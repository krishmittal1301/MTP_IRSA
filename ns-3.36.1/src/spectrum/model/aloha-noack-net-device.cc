/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 CTTC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/enum.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/channel.h"
#include "ns3/trace-source-accessor.h"
#include "aloha-noack-mac-header.h"
#include "aloha-noack-net-device.h"
#include "ns3/llc-snap-header.h"
#include "timestamp-tag.h"
#include "fstream"
#include "ns3/distribution-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AlohaNoackNetDevice");

/**
 * \brief Output stream operator
 * \param os output stream
 * \param state the state to print
 * \return an output stream
 */
std::ostream &
operator<< (std::ostream &os, AlohaNoackNetDevice::State state)
{
  switch (state)
    {
    case AlohaNoackNetDevice::IDLE:
      os << "IDLE";
      break;
    case AlohaNoackNetDevice::TX:
      os << "TX";
      break;
    case AlohaNoackNetDevice::RX:
      os << "RX";
      break;
    }
  return os;
}

NS_OBJECT_ENSURE_REGISTERED (AlohaNoackNetDevice);

TypeId
AlohaNoackNetDevice::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::AlohaNoackNetDevice")
          .SetParent<NetDevice> ()
          .SetGroupName ("Spectrum")
          .AddConstructor<AlohaNoackNetDevice> ()
          .AddAttribute ("Address", "The MAC address of this device.",
                         Mac48AddressValue (Mac48Address ("12:34:56:78:90:12")),
                         MakeMac48AddressAccessor (&AlohaNoackNetDevice::m_address),
                         MakeMac48AddressChecker ())
          .AddAttribute ("Queue", "packets being transmitted get queued here", PointerValue (),
                         MakePointerAccessor (&AlohaNoackNetDevice::m_queue),
                         MakePointerChecker<Queue<Packet>> ())
          .AddAttribute (
              "Mtu", "The Maximum Transmission Unit", UintegerValue (1500),
              MakeUintegerAccessor (&AlohaNoackNetDevice::SetMtu, &AlohaNoackNetDevice::GetMtu),
              MakeUintegerChecker<uint16_t> (1, 65535))
          .AddAttribute (
              "Phy", "The PHY layer attached to this device.", PointerValue (),
              MakePointerAccessor (&AlohaNoackNetDevice::GetPhy, &AlohaNoackNetDevice::SetPhy),
              MakePointerChecker<Object> ())
          .AddTraceSource ("MacTx",
                           "Trace source indicating a packet has arrived "
                           "for transmission by this device",
                           MakeTraceSourceAccessor (&AlohaNoackNetDevice::m_macTxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacTxDrop",
                           "Trace source indicating a packet has been dropped "
                           "by the device before transmission",
                           MakeTraceSourceAccessor (&AlohaNoackNetDevice::m_macTxDropTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacPromiscRx",
                           "A packet has been received by this device, has been "
                           "passed up from the physical layer "
                           "and is being forwarded up the local protocol stack.  "
                           "This is a promiscuous trace,",
                           MakeTraceSourceAccessor (&AlohaNoackNetDevice::m_macPromiscRxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacRx",
                           "A packet has been received by this device, "
                           "has been passed up from the physical layer "
                           "and is being forwarded up the local protocol stack.  "
                           "This is a non-promiscuous trace,",
                           MakeTraceSourceAccessor (&AlohaNoackNetDevice::m_macRxTrace),
                           "ns3::Packet::TracedCallback");
  return tid;
}

//Added for MTP
std::ofstream AlohaNoackNetDevice::m_delayLog ("delayLog.txt");
std::ofstream AlohaNoackNetDevice::m_macDelayLog ("MacDelayLog.txt");
std::ofstream AlohaNoackNetDevice::m_incomingLog ("incomingLog.txt");
std::ofstream AlohaNoackNetDevice::m_lossLog ("lossLog.txt");

AlohaNoackNetDevice::AlohaNoackNetDevice () : m_state (IDLE)
{
  m_pktCounter = 0;
  m_pktSent = 0;
  m_pktReceived = 0;
  NS_LOG_FUNCTION (this);
}

AlohaNoackNetDevice::~AlohaNoackNetDevice ()
{
  NS_LOG_FUNCTION (this);
  m_queue = 0;
}

void
AlohaNoackNetDevice::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_queue = 0;
  m_node = 0;
  m_channel = 0;
  m_currentPkt = 0;
  m_phy = 0;
  m_phyMacTxStartCallback = MakeNullCallback<bool, Ptr<Packet>> ();
  NetDevice::DoDispose ();
}

void
AlohaNoackNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (index);
  m_ifIndex = index;
}

uint32_t
AlohaNoackNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ifIndex;
}

bool
AlohaNoackNetDevice::SetMtu (uint16_t mtu)
{
  NS_LOG_FUNCTION (mtu);
  m_mtu = mtu;
  return true;
}

uint16_t
AlohaNoackNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}

void
AlohaNoackNetDevice::SetQueue (Ptr<Queue<Packet>> q)
{
  NS_LOG_FUNCTION (q);
  m_queue = q;
}

void
AlohaNoackNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this);
  m_address = Mac48Address::ConvertFrom (address);
}

Address
AlohaNoackNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION (this);
  return m_address;
}

bool
AlohaNoackNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

Address
AlohaNoackNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
AlohaNoackNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

Address
AlohaNoackNetDevice::GetMulticast (Ipv4Address addr) const
{
  NS_LOG_FUNCTION (addr);
  Mac48Address ad = Mac48Address::GetMulticast (addr);
  return ad;
}

Address
AlohaNoackNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (addr);
  Mac48Address ad = Mac48Address::GetMulticast (addr);
  return ad;
}

bool
AlohaNoackNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool
AlohaNoackNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

Ptr<Node>
AlohaNoackNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_node;
}

void
AlohaNoackNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (node);

  m_node = node;
}

void
AlohaNoackNetDevice::SetPhy (Ptr<Object> phy)
{
  NS_LOG_FUNCTION (this << phy);
  m_phy = phy;
}

Ptr<Object>
AlohaNoackNetDevice::GetPhy () const
{
  NS_LOG_FUNCTION (this);
  return m_phy;
}

void
AlohaNoackNetDevice::SetChannel (Ptr<Channel> c)
{
  NS_LOG_FUNCTION (this << c);
  m_channel = c;
}

Ptr<Channel>
AlohaNoackNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION (this);
  return m_channel;
}

bool
AlohaNoackNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

bool
AlohaNoackNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return m_linkUp;
}

void
AlohaNoackNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (&callback);
  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}

void
AlohaNoackNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION (&cb);
  m_rxCallback = cb;
}

void
AlohaNoackNetDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (&cb);
  m_promiscRxCallback = cb;
}

bool
AlohaNoackNetDevice::SupportsSendFrom () const
{
  NS_LOG_FUNCTION (this);
  return true;
}

bool
AlohaNoackNetDevice::Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (packet << dest << protocolNumber);
  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool
AlohaNoackNetDevice::SendFrom (Ptr<Packet> packet, const Address &src, const Address &dest,
                               uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (packet << src << dest << protocolNumber);
  // std::cout<<"send from called"<<Simulator::Now()<<std::endl;
  Ptr<Packet> packetCopy = packet->Copy (); // work on a copy
  LlcSnapHeader llc;
  llc.SetType (protocolNumber);
  packetCopy->AddHeader (llc);

  AlohaNoackMacHeader header;
  header.SetSource (Mac48Address::ConvertFrom (src));
  header.SetDestination (Mac48Address::ConvertFrom (dest));
  packetCopy->AddHeader (header);

  uint8_t distType;
  DistributionTag distTag;
  if (packet->PeekPacketTag (distTag))
    {
      distType = distTag.GetDistribution ();
      // std::cout << "Distribution Type = " << distType << std::endl;
    }
  m_incomingLog << "Src: " << header.GetSource () << "\t"
                << "DistributionType: " << static_cast<DistributionTag::DistType> (distType) << "\t"
                << "Time: " << Simulator::Now () << "\t"
                << "queue-length: " << m_queue->GetCurrentSize () << std::endl;

  m_macTxTrace (packet);

  bool sendOk = true;
  //
  // If the device is idle, transmission starts immediately. Otherwise,
  // the transmission will be started by NotifyTransmissionEnd
  //
  NS_LOG_LOGIC (this << " state=" << m_state);
  // if (m_state == IDLE)
  //   {
  //     if (m_queue->IsEmpty ())
  //       {
  //         NS_LOG_LOGIC ("new packet is head of queue, starting TX immediately");
  //         m_currentPkt = packet;
  //         StartTransmission ();
  //       }
  //     else
  //       {
  //         NS_LOG_LOGIC ("enqueueing new packet");
  //         if (m_queue->Enqueue (packet) == false)
  //           {
  //             m_macTxDropTrace (packet);
  //             sendOk = false;
  //           }
  //       }
  //   }
  // else
  //   {
  //     NS_LOG_LOGIC ("deferring TX, enqueueing new packet");
  //     NS_ASSERT (m_queue);
  //     if (m_queue->Enqueue (packet) == false)
  //       {
  //         m_macTxDropTrace (packet);
  //         sendOk = false;
  //       }
  //   }
  //
  NS_LOG_LOGIC ("deferring TX, enqueueing new packet");
  NS_ASSERT (m_queue);
  if (m_queue->IsEmpty ())
    {
      Time now = Simulator::Now ();
      Time slotDuration = MilliSeconds (4); // Slot Time
      Time nextSlot = Seconds (std::ceil (now.GetSeconds () / slotDuration.GetSeconds ()) *
                               slotDuration.GetSeconds ());
      if (nextSlot == now && m_state != IDLE)
        {
          nextSlot += slotDuration;
          // std::cout<<"Scheduled for"<<nextSlot<<std::endl;
        }
      else
        {
          Simulator::Schedule (nextSlot - now, &AlohaNoackNetDevice::StartTransmission, this);
          // std::cout<<"scheduled for"<<nextSlot<<std::endl;
        }
    }
  TimestampTag ts (Simulator::Now ());
  packetCopy->AddPacketTag (ts);
  m_pktSent++;
  if (m_queue->Enqueue (packetCopy) == false)
    {
      m_macTxDropTrace (packetCopy);
      sendOk = false;
    }
  return sendOk;
}

void
AlohaNoackNetDevice::SetGenericPhyTxStartCallback (GenericPhyTxStartCallback c)
{
  NS_LOG_FUNCTION (this);
  m_phyMacTxStartCallback = c;
}

void
AlohaNoackNetDevice::StartTransmission ()
{
  // std::cout<<"packet left: "<<m_queue->GetNPackets()<<std::endl;
  // std::cout<<"starting tansmission at"<<Simulator::Now()<<std::endl;
  NS_LOG_FUNCTION (this);

  // NS_ASSERT (m_currentPkt == 0);
  NS_ASSERT (m_state == IDLE);

  if (m_queue->IsEmpty () == false)
    {
      Ptr<Packet> p = m_queue->Dequeue ();

      TimestampTag ts;
      Time macdelay;
      DistributionTag distTag;
      uint8_t distType;

      if (p->PeekPacketTag (ts))
        {
          macdelay = Simulator::Now () - ts.GetTimestamp ();
          // std::cout << "End-to-end delay = " << delay.GetSeconds() << " s" << std::endl;
        }

      if (p->PeekPacketTag (distTag))
        {
          distType = distTag.GetDistribution ();
          // std::cout << "Distribution Type = " << distType << std::endl;
        }
      m_macDelayLog << "Time: " << Simulator::Now () << "\t"
                << "MacDelay: " << macdelay.GetSeconds () << "\t"
                << "queue-length: " << m_queue->GetCurrentSize () << "\t"
                << "DistType: "<<static_cast<DistributionTag::DistType> (distType)<<std::endl;
      NS_ASSERT (p);
      m_currentPkt = p;
      // std::cout<<"transmitting packet of size: "<<m_currentPkt->GetSize()<<std::endl;
      NS_LOG_LOGIC ("scheduling transmission now");
      if (m_phyMacTxStartCallback (m_currentPkt))
        {
          NS_LOG_WARN ("PHY refused to start TX");
        }
      else
        {
          m_state = TX;
        }
      if (m_queue->IsEmpty () == false)
        {
          Time now = Simulator::Now ();
          Time slotDuration = MilliSeconds (4); // Slot Time
          Time nextSlot = Seconds (std::ceil (now.GetSeconds () / slotDuration.GetSeconds () +
                                              NanoSeconds (1).GetSeconds ()) *
                                   slotDuration.GetSeconds ());
          Simulator::Schedule (nextSlot - now, &AlohaNoackNetDevice::StartTransmission, this);
          // std::cout<<"scheduled for"<<nextSlot<<std::endl;
        }
    }
}

void
AlohaNoackNetDevice::NotifyTransmissionEnd (Ptr<const Packet>)
{
  // std::cout<<"packet left: "<<m_queue->GetNPackets()<<std::endl;
  // std::cout<<"notify trans end: "<<Simulator::Now()<<std::endl;
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_state == TX, "TX end notified while state != TX");
  m_state = IDLE;
  NS_ASSERT (m_queue);
  // if (m_queue->IsEmpty () == false)
  //   {
  //     Ptr<Packet> p = m_queue->Dequeue ();
  //     NS_ASSERT (p);
  //     m_currentPkt = p;
  //     NS_LOG_LOGIC ("scheduling transmission now");
  //     Simulator::ScheduleNow (&AlohaNoackNetDevice::StartTransmission, this);
  //   }
}

void
AlohaNoackNetDevice::NotifyReceptionStart ()
{
  // std::cout<<"notify reception start: "<<Simulator::Now()<<std::endl;
  NS_LOG_FUNCTION (this);
}

void
AlohaNoackNetDevice::NotifyReceptionEndError ()
{
  NS_LOG_FUNCTION (this);
}

void
AlohaNoackNetDevice::NotifyReceptionEndOk (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  AlohaNoackMacHeader header;
  Ptr<Packet> originalPacket = packet->Copy ();
  TimestampTag ts;
  DistributionTag distTag;
  Time delay;
  uint8_t distType;
  uint32_t packetSize = packet->GetSize ();
  // std::cout<<"notify reception end ok: "<<Simulator::Now()<<" Packet Size: "<<packetSize<<std::endl;
  if (packet->PeekPacketTag (ts))
    {
      delay = Simulator::Now () - ts.GetTimestamp ();
      // std::cout << "End-to-end delay = " << delay.GetSeconds() << " s" << std::endl;
    }
  if (packet->PeekPacketTag (distTag))
    {
      distType = distTag.GetDistribution ();
      // std::cout << "Distribution Type = " << distType << std::endl;
    }

  packet->RemoveHeader (header);
  NS_LOG_LOGIC ("packet " << header.GetSource () << " --> " << header.GetDestination ()
                          << " (here: " << m_address << ")");

  //Added By for MTP
  Mac48Address src = header.GetSource ();
  Mac48Address dst = header.GetDestination ();

  LlcSnapHeader llc;
  packet->RemoveHeader (llc);

  PacketType packetType;
  if (header.GetDestination ().IsBroadcast ())
    {
      packetType = PACKET_BROADCAST;
    }
  else if (header.GetDestination ().IsGroup ())
    {
      packetType = PACKET_MULTICAST;
    }
  else if (header.GetDestination () == m_address)
    {
      packetType = PACKET_HOST;
    }
  else
    {
      packetType = PACKET_OTHERHOST;
    }

  NS_LOG_LOGIC ("packet type = " << packetType);

  // m_macPromiscRxTrace (originalPacket);
  if (!m_promiscRxCallback.IsNull ())
    {
      m_promiscRxCallback (this, packet->Copy (), llc.GetType (), header.GetSource (),
                           header.GetDestination (), packetType);
    }

  if (packetType != PACKET_OTHERHOST)
    {
      // std::cout<<"notify reception end: "<<Simulator::Now()<<std::endl;
      //Added By for MTP
      m_pktReceived++;
      m_pktCounter++;
      m_delayLog << m_pktCounter << "\t" << "Src: " << src << "\t" << "Dst: " << dst << "\t"
                 << "Delay: " << delay.GetSeconds () << "\t" << "PacketSize: " << packetSize << "\t"
                 << "DistributionType: " << static_cast<DistributionTag::DistType> (distType)
                 << "\t" << "Time: " << Simulator::Now () << std::endl;
      NS_LOG_LOGIC ( m_pktCounter << "\t" << "Src: " << src << "\t" << "Dst: "<< dst << "\t" <<"Delay: "<< delay.GetSeconds() <<"\t"<<"PacketSize: "<< packetSize << "\t" << "DistributionType: " << static_cast<DistributionTag::DistType>(distType) << "\t" << "Time: " << Simulator::Now() <<std::endl);

      m_macRxTrace (originalPacket);
      m_rxCallback (this, packet, llc.GetType (), header.GetSource ());
    }
}

void
AlohaNoackNetDevice::LogStatistics ()
{
  m_lossLog << "Sent: " << m_pktSent << " Received: " << m_pktReceived << std::endl;
}

} // namespace ns3
