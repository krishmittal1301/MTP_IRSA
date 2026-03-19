/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 CTTC
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

#include <ns3/object-factory.h>
#include <ns3/log.h>
#include <cmath>
#include <ns3/simulator.h>
#include <ns3/trace-source-accessor.h>
#include <ns3/packet-burst.h>
#include <ns3/callback.h>
#include <ns3/antenna-model.h>

#include "half-duplex-ideal-phy.h"
#include "half-duplex-ideal-phy-signal-parameters.h"
#include "spectrum-error-model.h"
#include "ns3/repetations-tag.h"
#include "slotted-aloha-noack-mac-header.h"
#include "ns3/timestamp-tag.h"
#include "ns3/distribution-tag.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HalfDuplexIdealPhy");

NS_OBJECT_ENSURE_REGISTERED (HalfDuplexIdealPhy);

HalfDuplexIdealPhy::HalfDuplexIdealPhy ()
  : m_mobility (0),
    m_netDevice (0),
    m_channel (0),
    m_txPsd (0),
    m_state (IDLE)
{
  m_interference.SetErrorModel (CreateObject<ShannonSpectrumErrorModel> ());
  m_slotDuration = MicroSeconds(60);
}

void
HalfDuplexIdealPhy::SetSlotDuration (Time slot)
{
  m_slotDuration = slot;
  std::cout<<"Slot duration set to: "<<m_slotDuration.GetMicroSeconds()<<" microseconds"<<std::endl;
}



HalfDuplexIdealPhy::~HalfDuplexIdealPhy ()
{
}

void
HalfDuplexIdealPhy::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_mobility = 0;
  m_netDevice = 0;
  m_channel = 0;
  m_txPsd = 0;
  m_rxPsd = 0;
  m_txPacket = 0;
  m_rxPacket = 0;
  m_phyMacTxEndCallback      = MakeNullCallback< void, Ptr<const Packet> > ();
  m_phyMacRxStartCallback    = MakeNullCallback< void > ();
  m_phyMacRxEndErrorCallback = MakeNullCallback< void > ();
  m_phyMacRxEndOkCallback    = MakeNullCallback< void, Ptr<Packet> >  ();
  SpectrumPhy::DoDispose ();
}

/**
 * \brief Output stream operator
 * \param os output stream
 * \param s the state to print
 * \return an output stream
 */
std::ostream& operator<< (std::ostream& os, HalfDuplexIdealPhy::State s)
{
  switch (s)
    {
    case HalfDuplexIdealPhy::IDLE:
      os << "IDLE";
      break;
    case HalfDuplexIdealPhy::RX:
      os << "RX";
      break;
    case HalfDuplexIdealPhy::TX:
      os << "TX";
      break;
    default:
      os << "UNKNOWN";
      break;
    }
  return os;
}


TypeId
HalfDuplexIdealPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HalfDuplexIdealPhy")
    .SetParent<SpectrumPhy> ()
    .SetGroupName ("Spectrum")
    .AddConstructor<HalfDuplexIdealPhy> ()
    .AddAttribute ("Rate",
                   "The PHY rate used by this device",
                   DataRateValue (DataRate ("1Mbps")),
                   MakeDataRateAccessor (&HalfDuplexIdealPhy::SetRate,
                                         &HalfDuplexIdealPhy::GetRate),
                   MakeDataRateChecker ())
    .AddTraceSource ("TxStart",
                     "Trace fired when a new transmission is started",
                     MakeTraceSourceAccessor (&HalfDuplexIdealPhy::m_phyTxStartTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("TxEnd",
                     "Trace fired when a previously started transmission is finished",
                     MakeTraceSourceAccessor (&HalfDuplexIdealPhy::m_phyTxEndTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RxStart",
                     "Trace fired when the start of a signal is detected",
                     MakeTraceSourceAccessor (&HalfDuplexIdealPhy::m_phyRxStartTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RxAbort",
                     "Trace fired when a previously started RX is aborted before time",
                     MakeTraceSourceAccessor (&HalfDuplexIdealPhy::m_phyRxAbortTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RxEndOk",
                     "Trace fired when a previously started RX terminates successfully",
                     MakeTraceSourceAccessor (&HalfDuplexIdealPhy::m_phyRxEndOkTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RxEndError",
                     "Trace fired when a previously started RX terminates with an error (packet is corrupted)",
                     MakeTraceSourceAccessor (&HalfDuplexIdealPhy::m_phyRxEndErrorTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}



Ptr<NetDevice>
HalfDuplexIdealPhy::GetDevice () const
{
  NS_LOG_FUNCTION (this);
  return m_netDevice;
}


Ptr<MobilityModel>
HalfDuplexIdealPhy::GetMobility () const
{
  NS_LOG_FUNCTION (this);
  return m_mobility;
}


void
HalfDuplexIdealPhy::SetDevice (Ptr<NetDevice> d)
{
  NS_LOG_FUNCTION (this << d);
  m_netDevice = d;
}


void
HalfDuplexIdealPhy::SetMobility (Ptr<MobilityModel> m)
{
  NS_LOG_FUNCTION (this << m);
  m_mobility = m;
}


void
HalfDuplexIdealPhy::SetChannel (Ptr<SpectrumChannel> c)
{
  NS_LOG_FUNCTION (this << c);
  m_channel = c;
}

Ptr<const SpectrumModel>
HalfDuplexIdealPhy::GetRxSpectrumModel () const
{
  if (m_txPsd)
    {
      return m_txPsd->GetSpectrumModel ();
    }
  else
    {
      return 0;
    }
}

void
HalfDuplexIdealPhy::SetTxPowerSpectralDensity (Ptr<SpectrumValue> txPsd)
{
  NS_LOG_FUNCTION (this << txPsd);
  NS_ASSERT (txPsd);
  m_txPsd = txPsd;
  NS_LOG_INFO ( *txPsd << *m_txPsd);
}

void
HalfDuplexIdealPhy::SetNoisePowerSpectralDensity (Ptr<const SpectrumValue> noisePsd)
{
  NS_LOG_FUNCTION (this << noisePsd);
  NS_ASSERT (noisePsd);
  m_interference.SetNoisePowerSpectralDensity (noisePsd);
}

void
HalfDuplexIdealPhy::SetRate (DataRate rate)
{
  NS_LOG_FUNCTION (this << rate);
  m_rate = rate;
}

DataRate
HalfDuplexIdealPhy::GetRate () const
{
  NS_LOG_FUNCTION (this);
  return m_rate;
}


void
HalfDuplexIdealPhy::SetGenericPhyTxEndCallback (GenericPhyTxEndCallback c)
{
  NS_LOG_FUNCTION (this);
  m_phyMacTxEndCallback = c;
}

void
HalfDuplexIdealPhy::SetGenericPhyRxStartCallback (GenericPhyRxStartCallback c)
{
  NS_LOG_FUNCTION (this);
  m_phyMacRxStartCallback = c;
}


void
HalfDuplexIdealPhy::SetGenericPhyRxEndErrorCallback (GenericPhyRxEndErrorCallback c)
{
  NS_LOG_FUNCTION (this);
  m_phyMacRxEndErrorCallback = c;
}


void
HalfDuplexIdealPhy::SetGenericPhyRxEndOkCallback (GenericPhyRxEndOkCallback c)
{
  NS_LOG_FUNCTION (this);
  m_phyMacRxEndOkCallback = c;
}

Ptr<Object>
HalfDuplexIdealPhy::GetAntenna () const
{
  NS_LOG_FUNCTION (this);
  return m_antenna;
}

void
HalfDuplexIdealPhy::SetAntenna (Ptr<AntennaModel> a)
{
  NS_LOG_FUNCTION (this << a);
  m_antenna = a;
}

void
HalfDuplexIdealPhy::ChangeState (State newState)
{
  NS_LOG_LOGIC (this << " state: " << m_state << " -> " << newState);
  m_state = newState;
}

void 
HalfDuplexIdealPhy::RecursiveInterferenceCancellation(Ptr<Packet> p)
{
  //sending packet to mac layer
  
  ns3::Mac48Address src;
  SlottedAlohaNoackMacHeader header;
  TimestampTag ts;
  Time tt;
  if(p->PeekPacketTag(ts)){
    tt = ts.GetTimestamp();
  }
  
  if (p->PeekHeader(header))
  {
    src = header.GetSource();
  }
  else
  {
    // std::cout << "Warning: packet has no SlottedAlohaNoackMacHeader!" << std::endl;
    NS_ASSERT_MSG(false, "Packet has no SlottedAlohaNoackMacHeader");
    return;
  }

  bool send_mac_layer = true;
  if(m_packetTracker.find(src) != m_packetTracker.end()){
    if(m_packetTracker[src].find(tt) != m_packetTracker[src].end()){
      send_mac_layer = false;
    }
  }
  else{
    m_packetTracker[src] = std::map<Time,uint64_t>();
  }

  if(send_mac_layer){
    if (!m_phyMacRxStartCallback.IsNull())
      {
        m_phyMacRxStartCallback ();
      }
    if (!m_phyMacRxEndOkCallback.IsNull())
      { 
        m_phyMacRxEndOkCallback (p);
        m_packetTracker[src][tt] = 1;
      }
  }


  std::vector<int> previousslots;
  repetationsTag rpt;
  if (p->PeekPacketTag(rpt))
  {
    previousslots = rpt.GetData().second;
  }
  else
  {
    // std::cout << "Warning: packet has no repetationsTag!" << std::endl;
    NS_ASSERT_MSG(false, "Packet has no repetationsTag");
    return; 
  }
  


  uint64_t currentSlot = Simulator::Now().GetMicroSeconds()/m_slotDuration.GetMicroSeconds();
  m_slotPacketMap.erase(currentSlot);

  for(auto slot: previousslots)
  {
    if(m_slotPacketMap.find(slot) != m_slotPacketMap.end())
    {
      std::vector<Ptr<Packet>> temp;
      for(auto pkt:  m_slotPacketMap[slot])
      {
        SlottedAlohaNoackMacHeader hdr;
        ns3::Mac48Address src1;
        
        if (pkt->PeekHeader(hdr))
        {
          src1 = hdr.GetSource();
        }
        else
        {
          // std::cout << "Warning: packet has no SlottedAlohaNoackMacHeader!" << std::endl;
          NS_ASSERT_MSG(false, "Packet has no SlottedAlohaNoackMacHeader");
          return;
        }

        if(src1 == src)
        {
          //same source
          // std::cout<<"Cancelling packet at slot: "<<slot<<" from source: "<<src1<<std::endl;
          //removing this packet from map
        }
        else
        {
          temp.push_back(pkt);
        }
      }
      m_slotPacketMap[slot] = temp;
      //if vector is empty then removing the key from map
      if(m_slotPacketMap[slot].size() == 1)
      {
        RecursiveInterferenceCancellation( m_slotPacketMap[slot][0]);
      }
    }
  }
}


void
HalfDuplexIdealPhy::RICCaller()
{
  int currSlot = Simulator::Now().GetMicroSeconds()/m_slotDuration.GetMicroSeconds();
  // std::cout<<"RIC Caller at slot: "<<currSlot<<std::endl;
  if(m_slotPacketMap.find(currSlot-1) != m_slotPacketMap.end() && m_slotPacketMap[currSlot-1].size() == 1){
    RecursiveInterferenceCancellation(m_slotPacketMap[currSlot-1][0]);
  }
}


bool
HalfDuplexIdealPhy::StartTx (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  NS_LOG_LOGIC (this << "state: " << m_state);

  m_phyTxStartTrace (p);

  switch (m_state)
    {
    case RX:
      AbortRx ();
    // fall through

    case IDLE:
      {
        m_txPacket = p;
        ChangeState (TX);
        Ptr<HalfDuplexIdealPhySignalParameters> txParams = Create<HalfDuplexIdealPhySignalParameters> ();
        Time txTimeSeconds = m_rate.CalculateBytesTxTime (p->GetSize ());
        txParams->duration = txTimeSeconds;
        txParams->txPhy = GetObject<SpectrumPhy> ();
        txParams->txAntenna = m_antenna;
        txParams->psd = m_txPsd;
        txParams->data = m_txPacket;

        NS_LOG_LOGIC (this << " tx power: " << 10 * std::log10 (Integral (*(txParams->psd))) + 30 << " dBm");
        m_channel->StartTx (txParams);
        Simulator::Schedule (txTimeSeconds, &HalfDuplexIdealPhy::EndTx, this);
      }
      break;

    case TX:

      return true;
      break;
    }
  return false;
}


void
HalfDuplexIdealPhy::EndTx ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " state: " << m_state);

  NS_ASSERT (m_state == TX);

  m_phyTxEndTrace (m_txPacket);

  if (!m_phyMacTxEndCallback.IsNull ())
    {
      m_phyMacTxEndCallback (m_txPacket);
    }

  m_txPacket = 0;
  ChangeState (IDLE);
}


// void
// HalfDuplexIdealPhy::StartRx (Ptr<SpectrumSignalParameters> spectrumParams)
// {
//   NS_LOG_FUNCTION (this << spectrumParams);
//   NS_LOG_LOGIC (this << " state: " << m_state);
//   NS_LOG_LOGIC (this << " rx power: " << 10 * std::log10 (Integral (*(spectrumParams->psd))) + 30 << " dBm");

//   // interference will happen regardless of the state of the receiver
//   m_interference.AddSignal (spectrumParams->psd, spectrumParams->duration);

//   // the device might start RX only if the signal is of a type understood by this device
//   // this corresponds in real devices to preamble detection
//   Ptr<HalfDuplexIdealPhySignalParameters> rxParams = DynamicCast<HalfDuplexIdealPhySignalParameters> (spectrumParams);
//   if (rxParams != 0)
//     {
//       // signal is of known type
//       switch (m_state)
//         {
//         case TX:
//           // the PHY will not notice this incoming signal
//           // std::cout<<"not received packet as it is transmitting"<<std::endl;
//           break;

//         case RX:
//           // std::cout<<"not received packet as it is already receiving"<<std::endl;
//           // we should check if we should re-sync on a new incoming signal and discard the old one
//           // (somebody calls this the "capture" effect)
//           // criteria considered to do might include the following:
//           //  1) signal strength (e.g., as returned by rxPsd.Norm ())
//           //  2) how much time has passed since previous RX attempt started
//           // if re-sync (capture) is done, then we should call AbortRx ()
//           break;

//         case IDLE:
//           // preamble detection and synchronization is supposed to be always successful.

//           Ptr<Packet> p = rxParams->data;
//           // std::cout<<"start receiving packet at slot: "<<Simulator::Now().GetMilliSeconds()/4<<std::endl;
          
//           // p->PeekPacketTag();
//           m_phyRxStartTrace (p);
//           m_rxPacket = p;
//           m_rxPsd = rxParams->psd;
//           ChangeState (RX);
//           if (!m_phyMacRxStartCallback.IsNull ())
//             {
//               NS_LOG_LOGIC (this << " calling m_phyMacRxStartCallback");
//               m_phyMacRxStartCallback ();
//             }
//           else
//             {
//               NS_LOG_LOGIC (this << " m_phyMacRxStartCallback is NULL");
//             }
//           m_interference.StartRx (p, rxParams->psd);
//           NS_LOG_LOGIC (this << " scheduling EndRx with delay " << rxParams->duration);
//           m_endRxEventId = Simulator::Schedule (rxParams->duration, &HalfDuplexIdealPhy::EndRx, this);

//           break;

//         }
//     }
//   else // rxParams == 0
//     {
//       NS_LOG_LOGIC (this << " signal of unknown type");
//     }

//   NS_LOG_LOGIC (this << " state: " << m_state);
// }


int received_packets = 0;

void
HalfDuplexIdealPhy::StartRx (Ptr<SpectrumSignalParameters> spectrumParams)
{
  NS_LOG_FUNCTION (this << spectrumParams);
  NS_LOG_LOGIC (this << " state: " << m_state);
  NS_LOG_LOGIC (this << " rx power: " << 10 * std::log10 (Integral (*(spectrumParams->psd))) + 30 << " dBm");

  // interference will happen regardless of the state of the receiver
  m_interference.AddSignal (spectrumParams->psd, spectrumParams->duration);

  // the device might start RX only if the signal is of a type understood by this device
  // this corresponds in real devices to preamble detection
  Ptr<HalfDuplexIdealPhySignalParameters> rxParams = DynamicCast<HalfDuplexIdealPhySignalParameters> (spectrumParams);
  if (rxParams != 0)
    {
      // signal is of known type
      switch (m_state)
        {
        case TX:
          // the PHY will not notice this incoming signal
          // std::cout<<"not received packet as it is transmitting"<<std::endl;
          break;

        default:
          // preamble detection and synchronization is supposed to be always successful.
          Ptr<Packet> p = rxParams->data;
          received_packets++;
          // std::cout<<"Received packets so far: "<<received_packets<<std::endl;
          // std::cout<<"start receiving packet at slot: "<<Simulator::Now().GetMilliSeconds()/4<<std::endl;
          
          // p->PeekPacketTag();
          if (p == 0)
          {
            NS_LOG_ERROR("StartRx received null packet, ignoring");
            return;
          }
          SlottedAlohaNoackMacHeader header;
          TimestampTag ts;
          DistributionTag distTag;
          Mac48Address src;
          Time tt;
          uint8_t distType = 99;
                    
          
          if (p->PeekHeader(header) && p->PeekPacketTag(ts))
          {
            src = header.GetSource();
            tt = ts.GetTimestamp();
          }
          if (p->PeekPacketTag (distTag))
          {
            distType = distTag.GetDistribution ();
          }
          m_phyUniqueReceptions.insert(std::make_pair(src, tt));
          if (distType == 0)
          {
            m_phyUniqueReceptionsDist1.insert (std::make_pair (src, tt));
          }
          else if (distType == 3)
          {
            m_phyUniqueReceptionsDist2.insert (std::make_pair (src, tt));
          }

          if(m_slotPacketMap.find(Simulator::Now().GetMicroSeconds()/m_slotDuration.GetMicroSeconds()) != m_slotPacketMap.end())
          {
            m_slotPacketMap[Simulator::Now().GetMicroSeconds()/m_slotDuration.GetMicroSeconds()].push_back(p);
          }
          else
          {
            std::vector<Ptr<Packet>> temp;
            temp.push_back(p);
            m_slotPacketMap[Simulator::Now().GetMicroSeconds()/m_slotDuration.GetMicroSeconds()] = temp;
          }
          Time now = Simulator::Now ();
          Time nextSlot = Seconds (std::ceil (now.GetSeconds () / m_slotDuration.GetMicroSeconds()) * m_slotDuration.GetMicroSeconds());

          // std::cout<<"Scheduling RIC Caller at slot: "<<nextSlot<<" Current slot: "<<now<<std::endl;
          Simulator::Schedule(MicroSeconds((now.GetMicroSeconds()/m_slotDuration.GetMicroSeconds() + 1)*m_slotDuration.GetMicroSeconds()) - now, &HalfDuplexIdealPhy::RICCaller, this);
          break;
        }
    }
  else // rxParams == 0
    {
      NS_LOG_LOGIC (this << " signal of unknown type");
    }

  NS_LOG_LOGIC (this << " state: " << m_state);
}




void
HalfDuplexIdealPhy::AbortRx ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << "state: " << m_state);

  NS_ASSERT (m_state == RX);
  m_interference.AbortRx ();
  m_phyRxAbortTrace (m_rxPacket);
  m_endRxEventId.Cancel ();
  m_rxPacket = 0;
  ChangeState (IDLE);
}


void
HalfDuplexIdealPhy::EndRx ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " state: " << m_state);
  // std::cout<<"finished receiving packet at slot: "<<Simulator::Now().GetMilliSeconds()/4<<std::endl;

  NS_ASSERT (m_state == RX);

  bool rxOk = m_interference.EndRx ();

  if (rxOk)
    {
      m_phyRxEndOkTrace (m_rxPacket);
      if (!m_phyMacRxEndOkCallback.IsNull ())
        {
          NS_LOG_LOGIC (this << " calling m_phyMacRxEndOkCallback");
          m_phyMacRxEndOkCallback (m_rxPacket);
        }
      else
        {
          NS_LOG_LOGIC (this << " m_phyMacRxEndOkCallback is NULL");
        }
    }
  else
    {
      m_phyRxEndErrorTrace (m_rxPacket);
      if (!m_phyMacRxEndErrorCallback.IsNull ())
        {
          NS_LOG_LOGIC (this << " calling m_phyMacRxEndErrorCallback");
          m_phyMacRxEndErrorCallback ();
        }
      else
        {
          NS_LOG_LOGIC (this << " m_phyMacRxEndErrorCallback is NULL");
        }
    }

  ChangeState (IDLE);
  m_rxPacket = 0;
  m_rxPsd = 0;
}


HalfDuplexIdealPhy::UncancelledPacketStats
HalfDuplexIdealPhy::GetUncancelledPacketCount () const
{
std::set<std::string> uniqueUnrecovered;

  for (auto const& [slot, packetList] : m_slotPacketMap)
    {
      // We only care about slots that still have packets (unresolved collisions)
      for (auto const& p : packetList)
        {
          SlottedAlohaNoackMacHeader header;
          TimestampTag ts;
          
          if (p->PeekHeader(header) && p->PeekPacketTag(ts))
            {
              Mac48Address src = header.GetSource();
              Time tt = ts.GetTimestamp();

              // Check if this specific packet was EVER successfully received
              bool wasReceived = false;
              if (m_packetTracker.count(src) > 0)
                {
                  if (m_packetTracker.at(src).count(tt) > 0)
                    {
                      wasReceived = true;
                    }
                }

              // If it's still in the map and was never tracked as 'received'
              if (!wasReceived)
                {
                  // Create a unique string key for the set
                  std::stringstream ss;
                  ss << src << tt.GetNanoSeconds();
                  uniqueUnrecovered.insert(ss.str());
                }
            }
        }
    }
    HalfDuplexIdealPhy::UncancelledPacketStats stats;
    stats.uncancelledCount = uniqueUnrecovered.size ();
    stats.totalReceivedCount = m_phyUniqueReceptions.size ();
    stats.totalReceivedDist1 = m_phyUniqueReceptionsDist1.size ();
    stats.totalReceivedDist2 = m_phyUniqueReceptionsDist2.size ();
    return stats;
  }

} // namespace ns3
