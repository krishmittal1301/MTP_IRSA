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
#include "ns3/propagation-delay-model.h"
#include "ns3/spectrum-propagation-loss-model.h"
#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/config.h"
#include "ns3/simulator.h"
#include "ns3/names.h"
#include "ns3/antenna-model.h"
#include "ns3/spectrum-channel.h"
#include "ns3/half-duplex-ideal-phy.h"
#include "ns3/mac48-address.h"
#include "ns3/aloha-noack-net-device.h"
#include "ns3/slotted-aloha-noack-net-device.h"
#include "adhoc-slotted-aloha-noack-ideal-phy-helper.h"
#include "ns3/trace-helper.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("AdhocSlottedAlohaNoackIdealPhyHelper");

AdhocSlottedAlohaNoackIdealPhyHelper::AdhocSlottedAlohaNoackIdealPhyHelper ()
{
  m_phy.SetTypeId ("ns3::HalfDuplexIdealPhy");
  m_device.SetTypeId ("ns3::SlottedAlohaNoackNetDevice");
  m_antenna.SetTypeId ("ns3::IsotropicAntennaModel");
}

AdhocSlottedAlohaNoackIdealPhyHelper::~AdhocSlottedAlohaNoackIdealPhyHelper ()
{
}

//added for setting queue size for the MTP project 
void 
AdhocSlottedAlohaNoackIdealPhyHelper::SetQueueSize (std::string type, uint32_t size) 
{
  m_queue.SetTypeId (type);
  m_queue.Set ("MaxSize", QueueSizeValue (QueueSize (std::to_string(size) + "p")));
}


void
AdhocSlottedAlohaNoackIdealPhyHelper::SetChannel (Ptr<SpectrumChannel> channel)
{
  m_channel = channel;
}

void
AdhocSlottedAlohaNoackIdealPhyHelper::SetChannel (std::string channelName)
{
  Ptr<SpectrumChannel> channel = Names::Find<SpectrumChannel> (channelName);
  m_channel = channel;
}

void
AdhocSlottedAlohaNoackIdealPhyHelper::SetTxPowerSpectralDensity (Ptr<SpectrumValue> txPsd)
{
  NS_LOG_FUNCTION (this << txPsd);
  m_txPsd = txPsd;
}

void
AdhocSlottedAlohaNoackIdealPhyHelper::SetNoisePowerSpectralDensity (Ptr<SpectrumValue> noisePsd)
{
  NS_LOG_FUNCTION (this << noisePsd);
  m_noisePsd = noisePsd;
}

void
AdhocSlottedAlohaNoackIdealPhyHelper::SetPhyAttribute (std::string name, const AttributeValue &v)
{
  m_phy.Set (name, v);
}


void
AdhocSlottedAlohaNoackIdealPhyHelper::SetDeviceAttribute (std::string name, const AttributeValue &v)
{
  m_device.Set (name, v);
}

void
AdhocSlottedAlohaNoackIdealPhyHelper::SetAntenna (std::string type,
                                           std::string n0, const AttributeValue &v0,
                                           std::string n1, const AttributeValue &v1,
                                           std::string n2, const AttributeValue &v2,
                                           std::string n3, const AttributeValue &v3,
                                           std::string n4, const AttributeValue &v4,
                                           std::string n5, const AttributeValue &v5,
                                           std::string n6, const AttributeValue &v6,
                                           std::string n7, const AttributeValue &v7)
{
  ObjectFactory factory;
  factory.SetTypeId (type);
  factory.Set (n0, v0);
  factory.Set (n1, v1);
  factory.Set (n2, v2);
  factory.Set (n3, v3);
  factory.Set (n4, v4);
  factory.Set (n5, v5);
  factory.Set (n6, v6);
  factory.Set (n7, v7);
  m_antenna = factory;
}

NetDeviceContainer
AdhocSlottedAlohaNoackIdealPhyHelper::Install (NodeContainer c) const
{
  NetDeviceContainer devices;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      Ptr<SlottedAlohaNoackNetDevice> dev = (m_device.Create ())->GetObject<SlottedAlohaNoackNetDevice> ();
      dev->SetAddress (Mac48Address::Allocate ());
      Ptr<Queue<Packet> > q = (m_queue.Create ())->GetObject<Queue<Packet> > ();
      dev->SetQueue (q);

      // note that we could have used a SpectrumPhyHelper here, but
      // given that it is straightforward to handle the configuration
      // in this helper here, we avoid asking the user to pass us a
      // SpectrumPhyHelper, so to spare him some typing.

      Ptr<HalfDuplexIdealPhy>  phy = (m_phy.Create ())->GetObject<HalfDuplexIdealPhy> ();
      NS_ASSERT (phy);

      dev->SetPhy (phy);

      NS_ASSERT (node);
      phy->SetMobility (node->GetObject<MobilityModel> ());

      NS_ASSERT (dev);
      phy->SetDevice (dev);

      NS_ASSERT_MSG (m_txPsd, "you forgot to call AdhocSlottedAlohaNoackIdealPhyHelper::SetTxPowerSpectralDensity ()");
      phy->SetTxPowerSpectralDensity (m_txPsd);

      NS_ASSERT_MSG (m_noisePsd, "you forgot to call AdhocSlottedAlohaNoackIdealPhyHelper::SetNoisePowerSpectralDensity ()");
      phy->SetNoisePowerSpectralDensity (m_noisePsd);

      NS_ASSERT_MSG (m_channel, "you forgot to call AdhocSlottedAlohaNoackIdealPhyHelper::SetChannel ()");
      phy->SetChannel (m_channel);
      dev->SetChannel (m_channel);
      m_channel->AddRx (phy);

      phy->SetGenericPhyTxEndCallback   (MakeCallback (&SlottedAlohaNoackNetDevice::NotifyTransmissionEnd, dev));
      phy->SetGenericPhyRxStartCallback (MakeCallback (&SlottedAlohaNoackNetDevice::NotifyReceptionStart, dev));
      phy->SetGenericPhyRxEndOkCallback (MakeCallback (&SlottedAlohaNoackNetDevice::NotifyReceptionEndOk, dev));
      dev->SetGenericPhyTxStartCallback (MakeCallback (&HalfDuplexIdealPhy::StartTx, phy));

      Ptr<AntennaModel> antenna = (m_antenna.Create ())->GetObject<AntennaModel> ();
      NS_ASSERT_MSG (antenna, "error in creating the AntennaModel object");
      phy->SetAntenna (antenna);

      node->AddDevice (dev);
      devices.Add (dev);
    }
  return devices;
}

void 
AdhocSlottedAlohaNoackIdealPhyHelper::EnablePcapInternal (std::string prefix, Ptr<NetDevice> nd, bool promiscuous, bool explicitFilename)
{
  //
  // All of the Pcap enable functions vector through here including the ones
  // that are wandering through all of devices on perhaps all of the nodes in
  // the system.  We can only deal with devices of type CsmaNetDevice.
  //
  Ptr<SlottedAlohaNoackNetDevice> device = nd->GetObject<SlottedAlohaNoackNetDevice> ();
  if (device == 0)
    {
      NS_LOG_INFO ("CsmaHelper::EnablePcapInternal(): Device " << device << " not of type ns3::SlottedAlohaNoackNetDevice");
      return;
    }

  PcapHelper pcapHelper;

  std::string filename;
  if (explicitFilename)
    {
      filename = prefix;
    }
  else
    {
      filename = pcapHelper.GetFilenameFromDevice (prefix, device);
    }

  Ptr<PcapFileWrapper> file = pcapHelper.CreateFile (filename, std::ios::out, PcapHelper::DLT_EN10MB);

  pcapHelper.HookDefaultSink<SlottedAlohaNoackNetDevice> (device, "MacTx", file);  
  pcapHelper.HookDefaultSink<SlottedAlohaNoackNetDevice> (device, "MacRx", file);  
  pcapHelper.HookDefaultSink<SlottedAlohaNoackNetDevice> (device, "MacPromiscRx", file);  
  pcapHelper.HookDefaultSink<SlottedAlohaNoackNetDevice> (device, "MacTxDrop", file);  
}


NetDeviceContainer
AdhocSlottedAlohaNoackIdealPhyHelper::Install (Ptr<Node> node) const
{
  return Install (NodeContainer (node));
}

NetDeviceContainer
AdhocSlottedAlohaNoackIdealPhyHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return Install (node);
}


} // namespace ns3
