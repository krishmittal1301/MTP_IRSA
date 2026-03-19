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
 * Author: Krish Mittal <B22214@students.iitmandi.ac.in>
 * Author: Siddharth Amlavad <B22177@students.iitmandi.ac.in>
 * Made the slotted aloha for MTP purpose
 */

#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/enum.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/channel.h"
#include "ns3/trace-source-accessor.h"
#include "slotted-aloha-noack-mac-header.h"
#include "slotted-aloha-noack-net-device.h"
#include "ns3/llc-snap-header.h"
#include "timestamp-tag.h"
#include "repetations-tag.h"
#include "fstream"
#include "ns3/distribution-tag.h"
#include "ns3/random-variable-stream.h"
#include "half-duplex-ideal-phy.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SlottedAlohaNoackNetDevice");

/**
 * \brief Output stream operator
 * \param os output stream
 * \param state the state to print
 * \return an output stream
 */
std::ostream &
operator<< (std::ostream &os, SlottedAlohaNoackNetDevice::State state)
{
  switch (state)
    {
    case SlottedAlohaNoackNetDevice::IDLE:
      os << "IDLE";
      break;
    case SlottedAlohaNoackNetDevice::TX:
      os << "TX";
      break;
    case SlottedAlohaNoackNetDevice::RX:
      os << "RX";
      break;
    }
  return os;
}

NS_OBJECT_ENSURE_REGISTERED (SlottedAlohaNoackNetDevice);

TypeId
SlottedAlohaNoackNetDevice::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::SlottedAlohaNoackNetDevice")
          .SetParent<NetDevice> ()
          .SetGroupName ("Spectrum")
          .AddConstructor<SlottedAlohaNoackNetDevice> ()
          .AddAttribute ("Address", "The MAC address of this device.",
                         Mac48AddressValue (Mac48Address ("12:34:56:78:90:12")),
                         MakeMac48AddressAccessor (&SlottedAlohaNoackNetDevice::m_address),
                         MakeMac48AddressChecker ())
          .AddAttribute ("Queue", "packets being transmitted get queued here", PointerValue (),
                         MakePointerAccessor (&SlottedAlohaNoackNetDevice::m_queue),
                         MakePointerChecker<Queue<Packet>> ())
          .AddAttribute (
                        "Mtu", "The Maximum Transmission Unit", UintegerValue (1500),
                        MakeUintegerAccessor (&SlottedAlohaNoackNetDevice::SetMtu, &SlottedAlohaNoackNetDevice::GetMtu),
                        MakeUintegerChecker<uint16_t> (1, 65535))
          .AddAttribute (
                        "Phy", "The PHY layer attached to this device.", PointerValue (),
                        MakePointerAccessor (&SlottedAlohaNoackNetDevice::GetPhy, &SlottedAlohaNoackNetDevice::SetPhy),
                        MakePointerChecker<Object> ())
          //Added for MTP for setting dynamic slot duration
          .AddAttribute (
                        "DataRate",
                        "PHY data rate used to compute slot duration",
                        UintegerValue (20),
                        MakeUintegerAccessor (&SlottedAlohaNoackNetDevice::m_dataRate),
                        MakeUintegerChecker<uint32_t> ())
          .AddAttribute (
                        "GuardTime",
                        "Extra guard time added to each slot",
                        UintegerValue (10),
                        MakeUintegerAccessor (&SlottedAlohaNoackNetDevice::m_guardTime),
                        MakeUintegerChecker<uint32_t> ())
          .AddAttribute (
                        "DefaultPacketSize",
                        "Packet size used to compute slot duration (bytes)",
                        UintegerValue (125),
                        MakeUintegerAccessor (&SlottedAlohaNoackNetDevice::m_pktSize),
                        MakeUintegerChecker<uint32_t> ())
          .AddAttribute (
                        "DelayLogFileName",
                        "File name for logging delay statistics",
                        StringValue ("delayLog.txt"),
                        MakeStringAccessor (&SlottedAlohaNoackNetDevice::m_delayLog),
                        MakeStringChecker ()
          )
          .AddAttribute (
                        "LossLogFileName",
                        "File name for logging loss statistics",
                        StringValue ("lossLog.txt"),
                        MakeStringAccessor (&SlottedAlohaNoackNetDevice::m_lossLog),
                        MakeStringChecker ()
          )
          .AddAttribute (
                        "MacDelayLogFileName",  
                        "File name for logging MAC delay statistics",
                        StringValue ("MacDelayLog.txt"),
                        MakeStringAccessor (&SlottedAlohaNoackNetDevice::m_macDelayLog),
                        MakeStringChecker ()
          )
          .AddAttribute (
                        "ProbabilityOfSend",
                        "Probability of sending a packet in a slot",
                        DoubleValue (0.1),
                        MakeDoubleAccessor (&SlottedAlohaNoackNetDevice::m_probabilityOfSend),
                        MakeDoubleChecker<double> (0.0, 1.0)
          )

          .AddTraceSource ("MacTx",
                           "Trace source indicating a packet has arrived "
                           "for transmission by this device",
                           MakeTraceSourceAccessor (&SlottedAlohaNoackNetDevice::m_macTxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacTxDrop",
                           "Trace source indicating a packet has been dropped "
                           "by the device before transmission",
                           MakeTraceSourceAccessor (&SlottedAlohaNoackNetDevice::m_macTxDropTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacPromiscRx",
                           "A packet has been received by this device, has been "
                           "passed up from the physical layer "
                           "and is being forwarded up the local protocol stack.  "
                           "This is a promiscuous trace,",
                           MakeTraceSourceAccessor (&SlottedAlohaNoackNetDevice::m_macPromiscRxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacRx",
                           "A packet has been received by this device, "
                           "has been passed up from the physical layer "
                           "and is being forwarded up the local protocol stack.  "
                           "This is a non-promiscuous trace,",
                           MakeTraceSourceAccessor (&SlottedAlohaNoackNetDevice::m_macRxTrace),
                           "ns3::Packet::TracedCallback");
  return tid;
}

//Added for MTP
std::ofstream SlottedAlohaNoackNetDevice::m_incomingLog ("incomingLog.txt");



SlottedAlohaNoackNetDevice::SlottedAlohaNoackNetDevice ()
  : m_state (IDLE)
{
  m_pktCounter = 0;
  m_pktSent = 0;
  m_pktSentDist1 = 0;
  m_pktSentDist2 = 0;
  m_pktLost = 0;
  m_pktReceived = 0;
  m_successReceivedDist1 = 0;
  m_successReceivedDist2 = 0;
  m_repetationsTracker = -1;
  m_slotsTracker = {};
  // m_probabilityOfSend = 0.3;
  // m_totalDelay = 0.0;
  // m_delayCount = 0;
  m_macTotalDelay = 0.0;
  m_macDelayCount = 0;
  m_totalDelayDist1 = 0.0; m_delayCountDist1 = 0;
  m_totalDelayDist2 = 0.0; m_delayCountDist2 = 0;


  m_slotDuration = MicroSeconds (ComputeSlotDuration());
  NS_LOG_FUNCTION (this);

}

void
SlottedAlohaNoackNetDevice::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  // Now m_dataRate and m_pktSize have their "40Mbps" and "125" values!
  m_slotDuration = MicroSeconds (ComputeSlotDuration());

  // std::cout<<"[SlottedAlohaNoackNetDevice] Slot Duration set to: "<<m_slotDuration.GetMicroSeconds()<<" microseconds"<<std::endl;
  Ptr<HalfDuplexIdealPhy> phy = DynamicCast<HalfDuplexIdealPhy> (m_phy);
  m_probabilityOfSend = std::min(std::max(m_probabilityOfSend, 0.0), 1.0); // Clamp between 0 and 1
  // std::cout<<"[SlottedAlohaNoackNetDevice] Probability of Send set to: "<<m_probabilityOfSend<<std::endl;
  if (phy)
    {
      phy->SetSlotDuration (m_slotDuration);
    }
  else
    {
      NS_FATAL_ERROR ("SlottedAlohaNoackNetDevice: PHY is not HalfDuplexIdealPhy");
    }

  if (!m_macDelayLog.empty ())
    {
      m_macDelayLogFile.open (m_macDelayLog.c_str (), std::ios::out | std::ios::app);
    }
  
  if (!m_delayLog.empty ())
    {
      // m_delayLogFile.open (m_delayLog.c_str (), std::ios::out | std::ios::app);
      size_t dotPos = m_delayLog.find_last_of(".");
      std::string base = (dotPos == std::string::npos) ? m_delayLog : m_delayLog.substr(0, dotPos);
      std::string ext = (dotPos == std::string::npos) ? "" : m_delayLog.substr(dotPos);

      std::string dist1Name = base + "_dist1" + ext;
      std::string dist2Name = base + "_dist2" + ext;

      m_delayLogFileDist1.open (dist1Name.c_str (), std::ios::out | std::ios::app);
      m_delayLogFileDist2.open (dist2Name.c_str (), std::ios::out | std::ios::app);
    }

  if (!m_lossLog.empty ())
    {
      m_lossLogFile.open (m_lossLog.c_str (), std::ios::out | std::ios::app);
    }

  
  // Call the parent class's version
  NetDevice::DoInitialize ();
}


SlottedAlohaNoackNetDevice::~SlottedAlohaNoackNetDevice ()
{
  NS_LOG_FUNCTION (this);
  m_queue = 0;
}

uint32_t
SlottedAlohaNoackNetDevice::ComputeSlotDuration () const
{
  uint64_t bits = m_pktSize * 8;
  uint64_t txTime =(double(bits) / double(m_dataRate));

  // std::cout<<"Packet Size (bytes): "<< m_pktSize << "\n";
  // std::cout<<"Data Rate (kbps): "<< m_dataRate << "\n";
  // std::cout<<"Computed TxTime: "<< txTime << " microseconds\n";
  // std::cout<<"Computed GuardTime: "<< m_guardTime << " microseconds\n";
  return txTime + m_guardTime;
}

void
SlottedAlohaNoackNetDevice::DoDispose ()
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
SlottedAlohaNoackNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (index);
  m_ifIndex = index;
}

uint32_t
SlottedAlohaNoackNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ifIndex;
}

bool
SlottedAlohaNoackNetDevice::SetMtu (uint16_t mtu)
{
  NS_LOG_FUNCTION (mtu);
  m_mtu = mtu;
  return true;
}

uint16_t
SlottedAlohaNoackNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}

void
SlottedAlohaNoackNetDevice::SetQueue (Ptr<Queue<Packet>> q)
{
  NS_LOG_FUNCTION (q);
  m_queue = q;
}

void
SlottedAlohaNoackNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this);
  m_address = Mac48Address::ConvertFrom (address);
}

Address
SlottedAlohaNoackNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION (this);
  return m_address;
}

bool
SlottedAlohaNoackNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

Address
SlottedAlohaNoackNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
SlottedAlohaNoackNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

Address
SlottedAlohaNoackNetDevice::GetMulticast (Ipv4Address addr) const
{
  NS_LOG_FUNCTION (addr);
  Mac48Address ad = Mac48Address::GetMulticast (addr);
  return ad;
}

Address
SlottedAlohaNoackNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (addr);
  Mac48Address ad = Mac48Address::GetMulticast (addr);
  return ad;
}

bool
SlottedAlohaNoackNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool
SlottedAlohaNoackNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

Ptr<Node>
SlottedAlohaNoackNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_node;
}

void
SlottedAlohaNoackNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (node);

  m_node = node;
}

void
SlottedAlohaNoackNetDevice::SetPhy (Ptr<Object> phy)
{
  NS_LOG_FUNCTION (this << phy);
  m_phy = phy;
}

Ptr<Object>
SlottedAlohaNoackNetDevice::GetPhy () const
{
  NS_LOG_FUNCTION (this);
  return m_phy;
}

void
SlottedAlohaNoackNetDevice::SetChannel (Ptr<Channel> c)
{
  NS_LOG_FUNCTION (this << c);
  m_channel = c;
}

Ptr<Channel>
SlottedAlohaNoackNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION (this);
  return m_channel;
}

bool
SlottedAlohaNoackNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

bool
SlottedAlohaNoackNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return m_linkUp;
}

int 
SlottedAlohaNoackNetDevice::ChooseRepetations (void)
{
  std::vector<double> cum = {0.5, 0.78, 1};      // probs = {0.5, 0.28, 0.22};
  std::vector<int> repetations = {2,3,8};
  
  std::cout<<"Repetations are chosen based on probabilities: "<<std::endl;
  
  for(size_t i=0; i<repetations.size(); i++)
  {    
    std::cout<<"Repetations: "<<repetations[i]<<" with cumulative probability: "<<cum[i]<<std::endl;
  }  

  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
  double rnd = uv->GetValue(0.0,1.0);

  if (rnd<=0.6)
  {
    return 2;
  }

  else if (rnd<=0.9)
  {
    return 3;
  }

  return 8;
}

bool
SlottedAlohaNoackNetDevice::SendorNot (void)
{
  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
  double rnd = uv->GetValue(0.0,1.0);
  if (rnd < m_probabilityOfSend)
  {
    return false;
  }

  return true;
}


void
SlottedAlohaNoackNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (&callback);
  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}

void
SlottedAlohaNoackNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION (&cb);
  m_rxCallback = cb;
}

void
SlottedAlohaNoackNetDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (&cb);
  m_promiscRxCallback = cb;
}

bool
SlottedAlohaNoackNetDevice::SupportsSendFrom () const
{
  NS_LOG_FUNCTION (this);
  return true;
}

bool
SlottedAlohaNoackNetDevice::Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (packet << dest << protocolNumber);
  return SendFrom (packet, m_address, dest, protocolNumber);
}

// int received_packetss = 0;

bool
SlottedAlohaNoackNetDevice::SendFrom (Ptr<Packet> packet, const Address &src, const Address &dest,
                               uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (packet << src << dest << protocolNumber);
  // std::cout<<"send from called"<<Simulator::Now()<<std::endl;
  Ptr<Packet> packetCopy = packet->Copy (); // work on a copy
  LlcSnapHeader llc;
  llc.SetType (protocolNumber);
  packetCopy->AddHeader (llc);

  SlottedAlohaNoackMacHeader header;
  header.SetSource (Mac48Address::ConvertFrom (src));
  header.SetDestination (Mac48Address::ConvertFrom (dest));
  packetCopy->AddHeader (header);

  // uint8_t distType;
  DistributionTag distTag;
  // if (packet->PeekPacketTag (distTag))
  //   {
  //     distType = distTag.GetDistribution ();
  //     // std::cout << "Distribution Type = " << distType << std::endl;
  //   }
  // m_incomingLog << "Src: " << header.GetSource () << "\t"
  //               << "DistributionType: " << static_cast<DistributionTag::DistType> (distType) << "\t"
  //               << "Time: " << Simulator::Now () << "\t"
  //               << "queue-length: " << m_queue->GetCurrentSize () << std::endl;

  m_macTxTrace (packet);
  // received_packetss++;
  // std::cout<<"Packets received so far from packet generation: "<<received_packetss<<std::endl;

  bool sendOk = true;
  //
  // If the device is idle, transmission starts immediately. Otherwise,
  // the transmission will be started by NotifyTransmissionEnd
  //
  NS_LOG_LOGIC (this << " state=" << m_state);

  NS_LOG_LOGIC ("deferring TX, enqueueing new packet");
  
  int repetations = ChooseRepetations();
  // std::cout<<"Chose repetations: "<<repetations<<std::endl
  repetationsTag repTag (repetations, std::vector<int>{});
  packetCopy->AddPacketTag (repTag);

  NS_ASSERT (m_queue);
  if (m_queue->IsEmpty ())
    {
      Time now = Simulator::Now ();
      Time nextSlot = Seconds (std::ceil (now.GetSeconds () / m_slotDuration.GetSeconds ()) *
                               m_slotDuration.GetSeconds ());
      if (nextSlot == now && m_state != IDLE)
        {
          nextSlot += m_slotDuration;
          // std::cout<<"Scheduled for"<<nextSlot<<std::endl;
        }
      else
        {
          Simulator::Schedule (nextSlot - now, &SlottedAlohaNoackNetDevice::StartTransmission, this);
          // std::cout<<"scheduled for"<<nextSlot<<std::endl;
        }
    }
  TimestampTag ts (Simulator::Now ());
  packetCopy->AddPacketTag (ts);
  m_pktSent++;
  if (packetCopy->PeekPacketTag (distTag))
    {
      const uint8_t distType = distTag.GetDistribution ();
      if (distType == 0)
        {
          m_pktSentDist1++;
        }
      else if (distType == 3)
        {
          m_pktSentDist2++;
        }
    }
  if (m_queue->Enqueue (packetCopy) == false)
    {
      m_macTxDropTrace (packetCopy);
      m_pktLost += 1;
      sendOk = false;
    }
  return sendOk;
}

void
SlottedAlohaNoackNetDevice::SetGenericPhyTxStartCallback (GenericPhyTxStartCallback c)
{
  NS_LOG_FUNCTION (this);
  m_phyMacTxStartCallback = c;
}

int send_packets = 0;

void
SlottedAlohaNoackNetDevice::StartTransmission ()
{
  // std::cout<<"packet left: "<<m_queue->GetNPackets()<<std::endl;
  NS_LOG_FUNCTION (this);
  
  // NS_ASSERT (m_currentPkt == 0);
  NS_ASSERT (m_state == IDLE);
  
  if(!SendorNot())
  {
    Time now = Simulator::Now ();
    Time nextSlot = Seconds (std::ceil (now.GetSeconds () / m_slotDuration.GetSeconds () +
    NanoSeconds (1).GetSeconds ()) *
    m_slotDuration.GetSeconds ());
    // std::cout<<(nextSlot-now).GetMicroSeconds()<<std::endl;
    Simulator::Schedule (nextSlot - now, &SlottedAlohaNoackNetDevice::StartTransmission, this);
  }
  
  else
  {
    
    // std::cout<<"starting tansmission at"<<Simulator::Now()<<" queue length: "<<m_queue->GetCurrentSize()<<std::endl;
    if(m_repetationsTracker == 0 && m_queue->IsEmpty() == false)
    {
      Ptr<Packet> p = m_queue->Dequeue ();
      if(m_queue->IsEmpty())
      {
        m_repetationsTracker = -1;
      }
    }
    
    // std::cout<<"starting tansmission at2"<<Simulator::Now()<<" queue length: "<<m_queue->GetCurrentSize()<<std::endl;
    if (m_queue->IsEmpty () == false)
    {
        if(m_repetationsTracker == 0 || m_repetationsTracker == -1)   // First time sending the packet
        {
          repetationsTag rpt;
          m_slotsTracker = {};
          m_queue->Peek ()->PeekPacketTag (rpt);
          m_repetationsTracker = rpt.GetData().first;
        }
              
        Ptr<Packet> p = m_queue->Peek ()->Copy();
        repetationsTag rpt;
        p->RemovePacketTag (rpt);
        m_slotsTracker.push_back(Simulator::Now().GetMicroSeconds()/m_slotDuration.GetMicroSeconds());
        p->AddPacketTag(repetationsTag(m_repetationsTracker, m_slotsTracker));
        m_repetationsTracker--;

        TimestampTag ts;
        Time macdelay;
        DistributionTag distTag;
        // uint8_t distType;
  
        if (p->PeekPacketTag (ts))
          {
            macdelay = Simulator::Now () - ts.GetTimestamp ();
            // std::cout << "End-to-end delay = " << delay.GetSeconds() << " s" << std::endl;
          }
  
        // if (p->PeekPacketTag (distTag))
        //   {
        //     distType = distTag.GetDistribution ();
        //     // std::cout << "Distribution Type = " << distType << std::endl;
        //   }
        m_macTotalDelay += macdelay.GetSeconds ();
        m_macDelayCount++;
        // m_macDelayLogFile << "Time: " << Simulator::Now () << "\t"
        //               << "MacDelay: " << macdelay.GetSeconds () << "\t"
        //               << "queue-length: " << m_queue->GetCurrentSize () << "\t"
        //               << "DistType: "<<static_cast<DistributionTag::DistType> (distType)<<std::endl;
        NS_ASSERT (p);
        m_currentPkt = p;
        // std::cout<<"transmitting packet at slot: "<<Simulator::Now().GetMicroSeconds()/m_slotDuration.GetMicroSeconds()<<std::endl;
        NS_LOG_LOGIC ("scheduling transmission now");
        // send_packets++;
        // std::cout<<"Send packet: "<<send_packets<<std::endl;
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
            Time nextSlot = Seconds (std::ceil (now.GetSeconds () / m_slotDuration.GetSeconds () +
                                                NanoSeconds (1).GetSeconds ()) *
                                     m_slotDuration.GetSeconds ());
            Simulator::Schedule (nextSlot - now, &SlottedAlohaNoackNetDevice::StartTransmission, this);
            // std::cout<<"scheduled for"<<nextSlot<<std::endl;
          }
      }
  }

}

void
SlottedAlohaNoackNetDevice::NotifyTransmissionEnd (Ptr<const Packet>)
{
  // std::cout<<"packet left: "<<m_queue->GetNPackets()<<std::endl;
  // std::cout<<"notify trans end: "<<Simulator::Now()<<std::endl;
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_state == TX, "TX end notified while state != TX");
  m_state = IDLE;
  NS_ASSERT (m_queue);
}

void
SlottedAlohaNoackNetDevice::NotifyReceptionStart ()
{
  // std::cout<<"notify reception start: "<<Simulator::Now()<<std::endl;
  NS_LOG_FUNCTION (this);
}

void
SlottedAlohaNoackNetDevice::NotifyReceptionEndError ()
{
  NS_LOG_FUNCTION (this);
}

void
SlottedAlohaNoackNetDevice::NotifyReceptionEndOk (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  SlottedAlohaNoackMacHeader header;
  Ptr<Packet> originalPacket = packet->Copy ();
  TimestampTag ts;
  DistributionTag distTag;
  Time delay;
  uint8_t distType = 99;
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


      // repetationsTag rptTag;
      // if (packet->PeekPacketTag(rptTag))  // Check if the packet has your custom tag
      // {
      //     std::pair<int, std::vector<int>> data = rptTag.GetData();
      //     int repetitions = data.first;         // number of repetitions left
      //     std::vector<int> slots = data.second; // slots in which repetitions were sent

      //     std::cout << "Repetitions left: " << repetitions << std::endl;
      //     std::cout << "Slots where repetitions occurred: ";
      //     for (int s : slots)
      //     {
      //         std::cout << s << " ";
      //     }
      //     std::cout << std::endl;
      // }
      // else
      // {
      //     std::cout << "No repetationsTag found in packet!" << std::endl;
      // }



      m_pktReceived++;
      m_pktCounter++;
      // m_totalDelay += delay.GetSeconds();
      // m_delayCount += 1;

      if (distType == 0) // Check the value assigned in your OnOffHelper
        {
          // m_delayLogFileDist1 << Simulator::Now().GetSeconds() << "\t" << delay.GetSeconds() << std::endl;
          m_successReceivedDist1++;
          m_totalDelayDist1 += delay.GetSeconds();
          m_delayCountDist1++;
        }
      else if (distType == 3) 
        {
          // m_delayLogFileDist2 << Simulator::Now().GetSeconds() << "\t" << delay.GetSeconds() << std::endl;
          m_successReceivedDist2++;
          m_totalDelayDist2 += delay.GetSeconds();
          m_delayCountDist2++;
        }

      // m_delayLogFile << m_pktCounter << "\t" << "Src: " << src << "\t" << "Dst: " << dst << "\t"
      //            << "Delay: " << delay.GetSeconds () << "\t" << "PacketSize: " << packetSize << "\t"
      //            << "DistributionType: " << static_cast<DistributionTag::DistType> (distType)
      //            << "\t" << "Time: " << Simulator::Now () << std::endl;
      NS_LOG_LOGIC ( m_pktCounter << "\t" << "Src: " << src << "\t" << "Dst: "<< dst << "\t" <<"Delay: "<< delay.GetSeconds() <<"\t"<<"PacketSize: "<< packetSize << "\t" << "DistributionType: " << static_cast<DistributionTag::DistType>(distType) << "\t" << "Time: " << Simulator::Now() <<std::endl);

      m_macRxTrace (originalPacket);
      m_rxCallback (this, packet, llc.GetType (), header.GetSource ());
    }
}

void
SlottedAlohaNoackNetDevice::LogStatistics ()
{
  Ptr<HalfDuplexIdealPhy> phy = DynamicCast<HalfDuplexIdealPhy> (m_phy);
  HalfDuplexIdealPhy::UncancelledPacketStats uncancelledCount = {0, 0, 0, 0};
  
  if (phy)
    {
      uncancelledCount = phy->GetUncancelledPacketCount();
    }

  // m_pktLost = Queue Drops (already added by you)
  // uncancelledCount = Transmitted packets that RIC couldn't recover
  // uint32_t totalLoss = m_pktLost + uncancelledCount;
  m_lossLogFile << "Sent: " << m_pktSent
                << " SentDist1: " << m_pktSentDist1
                << " SentDist2: " << m_pktSentDist2
                << " notSent: " << m_pktLost
                << " TotalReceived: " << uncancelledCount.totalReceivedCount
                << " TotalReceivedDist1: " << uncancelledCount.totalReceivedDist1
                << " TotalReceivedDist2: " << uncancelledCount.totalReceivedDist2
                << " ReceivedSuccessfully: " << m_pktReceived
                << " ReceivedSuccessfullyDist1: " << m_successReceivedDist1
                << " ReceivedSuccessfullyDist2: " << m_successReceivedDist2
                << " lost: " << uncancelledCount.uncancelledCount << std::endl;
  // m_delayLogFile << "Average Delay: " << (m_totalDelay / m_delayCount) << " seconds over " << m_delayCount << " packets." << std::endl;
  m_macDelayLogFile << "Average MAC Delay: " << (m_macTotalDelay / m_macDelayCount) << " seconds over " << m_macDelayCount << " packets." << std::endl;

  if (m_delayCountDist1 > 0)
    m_delayLogFileDist1 << "Final Average Delay (Dist1): " << (m_totalDelayDist1 / m_delayCountDist1) << " Over packet: " << m_delayCountDist1 << std::endl;
  
  if (m_delayCountDist2 > 0)
    m_delayLogFileDist2 << "Final Average Delay (Dist2): " << (m_totalDelayDist2 / m_delayCountDist2) << " Over packet: " << m_delayCountDist2 << std::endl;
}

} // namespace ns3
