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



#include <iostream>

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/spectrum-model-ism2400MHz-res1MHz.h>
#include <ns3/spectrum-model-300kHz-300GHz-log.h>
#include <ns3/wifi-spectrum-value-helper.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/waveform-generator.h>
#include <ns3/spectrum-analyzer.h>
#include <ns3/log.h>
#include <string>
#include <ns3/friis-spectrum-propagation-loss.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/mobility-module.h>
#include <ns3/spectrum-helper.h>
#include <ns3/applications-module.h>
// #include <ns3/adhoc-aloha-noack-ideal-phy-helper.h>
#include <ns3/adhoc-slotted-aloha-noack-ideal-phy-helper.h>
// #include "ns3/aloha-noack-net-device.h"
#include <ns3/slotted-aloha-noack-net-device.h>
#include <ns3/random-variable-stream.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TestAdhocOfdmAloha");

static bool g_verbose = false;

void
PhyTxStartTrace (std::string context, Ptr<const Packet> p)
{
  if (g_verbose)
    {
      std::cout << context << " PHY TX START p: " << p << std::endl;
    }
}


void
PhyTxEndTrace (std::string context, Ptr<const Packet> p)
{
  if (g_verbose)
    {
      std::cout << context << " PHY TX END p: " << p << std::endl;
    }
}

void
PhyRxStartTrace (std::string context, Ptr<const Packet> p)
{
  if (g_verbose)
    {
      std::cout << context << " PHY RX START p:" << p << std::endl;
    }
}

void
PhyRxEndOkTrace (std::string context, Ptr<const Packet> p)
{
  if (g_verbose)
    {
      std::cout << context << " PHY RX END OK p:" << p << std::endl;
    }
}

void
PhyRxEndErrorTrace (std::string context, Ptr<const Packet> p)
{
  if (g_verbose)
    {
      std::cout << context << " PHY RX END ERROR p:" << p << std::endl;
    }
}


void
ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  uint64_t bytes = 0;
  while ((packet = socket->Recv ()))
    {
      bytes += packet->GetSize ();
    }
  if (g_verbose)
    {
      std::cout << "SOCKET received " << bytes << " bytes" << std::endl;
    }
}

Ptr<Socket>
SetupPacketReceive (Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::PacketSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  sink->Bind ();
  sink->SetRecvCallback (MakeCallback (&ReceivePacket));
  return sink;
}

int main (int argc, char** argv)
{
  CommandLine cmd (__FILE__);

  double meanInterArrival = 0.008;
  std::string rate = "20Mbps";

  uint32_t nNodes = 3;
  // double distance = 50.0;          // meters
  double simTime = 10.0;
  int packetSize = 125; // bytes
  int queueSize = 2000; // packets
  std::string delayLog = "delayLog.txt";
  std::string lossLog = "lossLog.txt";
  std::string macDelayLog = "MacDelayLog.txt";
  double probabilityOfSend = 0.01;
  uint32_t run = 1;
  RngSeedManager::SetSeed(12345); //Set seed for reproducibility


  //Adding for car's position alignment in 2D
  double roadLength = 300.0;
  double roadWidth = 6.0;
  double nCars = 50; // Density (cars per square meter)
  // double minDistance = 5.0; // Minimum distance between car centers (standard car length)
  // double minLengthDist = 7.0; // Your length requirement
  // double minWidthDist = 3.0;  // Center-to-center for 2 lanes

  cmd.AddValue ("verbose", "Print trace information if true", g_verbose);
  // cmd.AddValue ("nNodes", "Number of nodes", nNodes);
  // cmd.AddValue ("distance", "Distance between adjacent nodes (m)", distance);
  cmd.AddValue ("rate", "Data rate", rate);
  cmd.AddValue ("mean", "Mean inter-arrival time", meanInterArrival);
  cmd.AddValue ("simTime", "Simulation time (s)", simTime);
  cmd.AddValue ( "packetSize", "Size of application packets (bytes)", packetSize);
  cmd.AddValue ( "queueSize", "Size of the queue (packets)", queueSize);
  cmd.AddValue ( "delayLog", "File name for logging delay statistics", delayLog);
  cmd.AddValue ( "lossLog", "File name for logging loss statistics", lossLog);
  cmd.AddValue ( "macDelayLog", "File name for logging MAC delay statistics", macDelayLog);
  cmd.AddValue ( "noOfCars", "No of cars you want to set", nCars);
  cmd.AddValue ( "ProbabilityOfSend", "Probability of sending a packet in a slot", probabilityOfSend);
  cmd.AddValue ( "run", "Run number for RNG seeding", run);
  cmd.Parse (argc, argv);


  // double area = roadLength * roadWidth;
  // Ptr<ExponentialRandomVariable> rv = CreateObject<ExponentialRandomVariable>();
  // rv->SetAttribute("Mean", DoubleValue(lambda * area));

  // nNodes = std::max(1, int(std::round(rv->GetValue())));
  nNodes = nCars; // For fixed number of cars instead of Poisson distribution
  std::cout << "Generated " << nNodes << " nodes for the area." << std::endl;

  RngSeedManager::SetRun(run);


  NodeContainer c;
  c.Create (nNodes);


  //Added for 2D alignment of vehicles
  //First using poisson distribution we get no of vehicles.
  //Then We align those vehicles in 2D grid while ensuring that those vehicles must be certain distance far apart 
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  Ptr<ns3::UniformRandomVariable> xRV = CreateObject<ns3::UniformRandomVariable>();
  Ptr<ns3::UniformRandomVariable> yRV = CreateObject<ns3::UniformRandomVariable>();
  std::vector<Vector> placedPositions;

  for (uint32_t i = 0; i < nNodes; ++i) {
    bool placed = false;
    int attempts = 0;
    while (!placed && attempts < 1000) { // Safety exit
        double posX = xRV->GetValue(0, roadLength);
        double posY = yRV->GetValue(0, roadWidth);
        Vector newPos(posX, posY, 0.0);

        bool tooClose = false;
        for (const auto& existingPos : placedPositions) {
            double dx = std::abs(newPos.x - existingPos.x);
            double dy = std::abs(newPos.y - existingPos.y);        
            if (dx == 0  && dy == 0) {
                tooClose = true;
                break;
            }
        }

        if (!tooClose) {
            positionAlloc->Add(newPos);
            placedPositions.push_back(newPos);
            placed = true;
        }
        attempts++;
    }
  }

  MobilityHelper mobility;
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(c);


  // MobilityHelper mobility;
  // Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  
  // for (uint32_t i = 0; i < nNodes; ++i)
  // {
  //   positionAlloc->Add (Vector (i * distance, 0.0, 0.0));
  // }

  
  // mobility.SetPositionAllocator (positionAlloc);
  // mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  // mobility.Install (c);


  SpectrumChannelHelper channelHelper = SpectrumChannelHelper::Default ();

  channelHelper.AddPropagationLoss ("ns3::FriisPropagationLossModel",
                                  "Frequency", DoubleValue (2.4e9),
                                  "SystemLoss", DoubleValue (1e3)); // 2.4 GHz
  channelHelper.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  Ptr<SpectrumChannel> channel = channelHelper.Create ();
  channel->SetAttribute ("MaxLossDb", DoubleValue (119.61));

  WifiSpectrumValue5MhzFactory sf;

  double txPower = 0.1; // Watts
  uint32_t channelNumber = 1;
  Ptr<SpectrumValue> txPsd =  sf.CreateTxPowerSpectralDensity (txPower, channelNumber);

  // for the noise, we use the Power Spectral Density of thermal noise
  // at room temperature. The value of the PSD will be constant over the band of interest.
  const double k = 1.381e-23; //Boltzmann's constant
  const double T = 290; // temperature in Kelvin
  double noisePsdValue = k * T; // watts per hertz
  Ptr<SpectrumValue> noisePsd = sf.CreateConstant (noisePsdValue);

  AdhocSlottedAlohaNoackIdealPhyHelper deviceHelper;
  deviceHelper.SetChannel (channel);
  deviceHelper.SetTxPowerSpectralDensity (txPsd);
  deviceHelper.SetNoisePowerSpectralDensity (noisePsd);
  deviceHelper.SetPhyAttribute ("Rate", DataRateValue (DataRate (rate)));
  deviceHelper.SetQueueSize("ns3::DropTailQueue<Packet>",queueSize); // MTP

  /**
   * This is done to dynamically set the slot duration
   * Dependent on the Packet size and Data Rate
   */
  uint32_t rateInt = std::stoi(rate.substr(0,rate.find("Mbps")));
  deviceHelper.SetDeviceAttribute("DataRate", UintegerValue (rateInt)); // MTP
  deviceHelper.SetDeviceAttribute ("DefaultPacketSize", UintegerValue (packetSize));
  deviceHelper.SetDeviceAttribute ("GuardTime", UintegerValue (160)); //MTP
  deviceHelper.SetDeviceAttribute ("DelayLogFileName", StringValue (delayLog)); // MTP
  deviceHelper.SetDeviceAttribute ("LossLogFileName", StringValue (lossLog)); // MTP
  deviceHelper.SetDeviceAttribute ("MacDelayLogFileName", StringValue (macDelayLog)); // MTP
  deviceHelper.SetDeviceAttribute ("ProbabilityOfSend", DoubleValue (probabilityOfSend)); // MTP

  NetDeviceContainer devices = deviceHelper.Install (c);

  PacketSocketHelper packetSocket;
  packetSocket.Install (c);

  Ptr<UniformRandomVariable> startRv = CreateObject<UniformRandomVariable>();
  ApplicationContainer allApps;

  for (uint32_t i = 0; i < nNodes; ++i)
  {
    PacketSocketAddress socket;
    socket.SetSingleDevice (devices.Get (i)->GetIfIndex ());
    socket.SetPhysicalAddress (Mac48Address::GetBroadcast ());
    socket.SetProtocol (1);
    //on-off for exponential traffic for each node
    OnOffHelper onoff ("ns3::PacketSocketFactory", Address (socket));
    onoff.SetConstantRate (DataRate (rate));
    onoff.SetAttribute ("PacketSize", UintegerValue (packetSize));
    onoff.SetAttribute("InterArrivalTime",StringValue("ns3::ExponentialRandomVariable[Mean=" +std::to_string(meanInterArrival) + "]")); // MTP
    onoff.SetAttribute("DistributionType",UintegerValue(0)); // MTP Exponential

    //on-off for constant traffic for each node
    OnOffHelper onoff2 ("ns3::PacketSocketFactory", Address (socket));
    onoff2.SetConstantRate (DataRate (rate));
    onoff2.SetAttribute ("PacketSize", UintegerValue (packetSize));
    onoff2.SetAttribute("InterArrivalTime",StringValue("ns3::ConstantRandomVariable[Constant="+std::to_string(meanInterArrival) + "]")); // MTP
    onoff2.SetAttribute("DistributionType",UintegerValue(3)); // MTP

    double startOffset = startRv->GetValue(0, meanInterArrival);

    ApplicationContainer app = onoff.Install (c.Get (i));
    app.Start (Seconds (startOffset));
    app.Stop (Seconds (simTime));
    allApps.Add (app);

    ApplicationContainer app2 = onoff2.Install (c.Get (i));
    app2.Start (Seconds (startOffset));
    app2.Stop (Seconds (simTime));
    allApps.Add (app2);
  }

  std::vector<Ptr<Socket>> recvSinks;

  for (uint32_t i = 0; i < c.GetN (); ++i)
  {
    recvSinks.push_back (SetupPacketReceive (c.Get (i)));
  }


  Simulator::Stop (Seconds (5*simTime));

  Config::Connect ("/NodeList/*/DeviceList/*/Phy/TxStart", MakeCallback (&PhyTxStartTrace));
  Config::Connect ("/NodeList/*/DeviceList/*/Phy/TxEnd", MakeCallback (&PhyTxEndTrace));
  Config::Connect ("/NodeList/*/DeviceList/*/Phy/RxStart", MakeCallback (&PhyRxStartTrace));
  Config::Connect ("/NodeList/*/DeviceList/*/Phy/RxEndOk", MakeCallback (&PhyRxEndOkTrace));
  Config::Connect ("/NodeList/*/DeviceList/*/Phy/RxEndError", MakeCallback (&PhyRxEndErrorTrace));

  // deviceHelper.EnablePcapAll("adhoc-aloha-ideal-phy", true);
  Simulator::Run ();

  for (uint32_t i=0; i<devices.GetN(); ++i)
  {
    Ptr<SlottedAlohaNoackNetDevice> dev = DynamicCast<SlottedAlohaNoackNetDevice>(devices.Get(i));
    if (dev) dev->LogStatistics();
  }


  Simulator::Destroy ();

  return 0;
}
