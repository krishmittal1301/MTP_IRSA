/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010
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

#ifndef SLOTTED_ALOHA_NOACK_NET_DEVICE_H
#define SLOTTED_ALOHA_NOACK_NET_DEVICE_H

#include <cstring>
#include <fstream>
#include <ns3/node.h>
#include <ns3/address.h>
#include <ns3/net-device.h>
#include <ns3/callback.h>
#include <ns3/packet.h>
#include <ns3/traced-callback.h>
#include <ns3/nstime.h>
#include <ns3/ptr.h>
#include <ns3/mac48-address.h>
#include <ns3/generic-phy.h>


namespace ns3 {


class SpectrumChannel;
class Channel;
class SpectrumErrorModel;
template <typename Item> class Queue;



/**
 * \ingroup spectrum
 *
 * This devices implements the following features:
 *  - layer 3 protocol multiplexing
 *  - MAC addressing
 *  - Aloha MAC:
 *    + packets transmitted as soon as possible
 *    + a new packet is queued if previous one is still being transmitted
 *    + no acknowledgements, hence no retransmissions
 *  - can support any PHY layer compatible with the API defined in generic-phy.h
 *
 */
class SlottedAlohaNoackNetDevice : public NetDevice
{
public:
  /**
   * State of the NetDevice
   */
  enum State
  {
    IDLE, //!< Idle state
    TX,   //!< Transmitting state
    RX    //!< Receiving state
  };

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  SlottedAlohaNoackNetDevice ();
  virtual ~SlottedAlohaNoackNetDevice ();

  virtual void DoInitialize (void) override;

  /**
   * set the queue which is going to be used by this device
   *
   * @param queue
   */
  virtual void SetQueue (Ptr<Queue<Packet> > queue);


  /*
  * Helps to choose how many times each packet will transmit
  * returning 2 with probability 0.6, 
  * returning 3 with probability 0.3,
  * returning 8 with probability 0.1
  */

  int ChooseRepetations (void);

  uint32_t  m_dataRate;
  uint32_t  m_guardTime;
  uint32_t  m_pktSize;


  /**
   * Compute the slot duration based on packet size, data rate and guard time
   *
   */
  uint32_t
  ComputeSlotDuration () const;

  /**
   * Whether to send or not in upcoming slots
   * Send with 0.5 probability 
   * Not send with 0.5 probability
   */

  bool SendorNot (void);

  /**
    * This method is used to capture the slot number of future slots as well.
    * It takes the number of repetations as an argument to capture the slot numbers for all repetations.
  */
  std::vector<int> GenerateReplicaSlots (int repetitions);

  /**
   * Notify the MAC that the PHY has finished a previously started transmission
   *
   */
  void NotifyTransmissionEnd (Ptr<const Packet>);

  /**
   * Notify the MAC that the PHY has started a reception
   *
   */
  void NotifyReceptionStart ();


  /**
   * Notify the MAC that the PHY finished a reception with an error
   *
   */
  void NotifyReceptionEndError ();

  /**
   * Notify the MAC that the PHY finished a reception successfully
   *
   * @param p the received packet
   */
  void NotifyReceptionEndOk (Ptr<Packet> p);


  /**
   * This class doesn't talk directly with the underlying channel (a
   * dedicated PHY class is expected to do it), however the NetDevice
   * specification features a GetChannel() method. This method here
   * is therefore provide to allow AlohaNoackNetDevice::GetChannel() to have
   * something meaningful to return.
   *
   * @param c the underlying channel
   */
  void SetChannel (Ptr<Channel> c);


  /**
   * set the callback used to instruct the lower layer to start a TX
   *
   * @param c
   */
  void SetGenericPhyTxStartCallback (GenericPhyTxStartCallback c);



  /**
   * Set the Phy object which is attached to this device.
   * This object is needed so that we can set/get attributes and
   * connect to trace sources of the PHY from the net device.
   *
   * @param phy the Phy object attached to the device.  Note that the
   * API between the PHY and the above (this NetDevice which also
   * implements the MAC) is implemented entirely by
   * callbacks, so we do not require that the PHY inherits by any
   * specific class.
   */
  void SetPhy (Ptr<Object> phy);

  /**
   * @return a reference to the PHY object embedded in this NetDevice.
   */
  Ptr<Object> GetPhy () const;



  // inherited from NetDevice
  virtual void SetIfIndex (const uint32_t index);
  virtual uint32_t GetIfIndex (void) const;
  virtual Ptr<Channel> GetChannel (void) const;
  virtual bool SetMtu (const uint16_t mtu);
  virtual uint16_t GetMtu (void) const;
  virtual void SetAddress (Address address);
  virtual Address GetAddress (void) const;
  virtual bool IsLinkUp (void) const;
  virtual void AddLinkChangeCallback (Callback<void> callback);
  virtual bool IsBroadcast (void) const;
  virtual Address GetBroadcast (void) const;
  virtual bool IsMulticast (void) const;
  virtual bool IsPointToPoint (void) const;
  virtual bool IsBridge (void) const;
  virtual bool Send (Ptr<Packet> packet, const Address& dest,
                     uint16_t protocolNumber);
  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest,
                         uint16_t protocolNumber);
  virtual Ptr<Node> GetNode (void) const;
  virtual void SetNode (Ptr<Node> node);
  virtual bool NeedsArp (void) const;
  virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);
  virtual Address GetMulticast (Ipv4Address addr) const;
  virtual Address GetMulticast (Ipv6Address addr) const;
  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);
  virtual bool SupportsSendFrom (void) const;
  virtual void LogStatistics (void);
  uint64_t m_pktCounter;
  uint64_t m_pktSent;
  uint64_t m_pktSentDist1;
  uint64_t m_pktSentDist2;
  uint64_t m_pktLost;
  uint64_t m_pktReceived;
  uint64_t m_successReceivedDist1;
  uint64_t m_successReceivedDist2;
  // For average delay logging
  // double m_totalDelay = 0.0;
  // uint64_t m_delayCount = 0;
  double m_probabilityOfSend = 0.3;
  double m_macTotalDelay = 0.0;
  uint64_t m_macDelayCount = 0;

  double m_totalDelayDist1;
  uint32_t m_delayCountDist1;
  double m_totalDelayDist2;
  uint32_t m_delayCountDist2;

  
  
  private:
  /**
   * Notification of Guard Interval end.
   */
  void NotifyGuardIntervalEnd ();
  virtual void DoDispose (void);
  std::string m_delayLog;
  static std::ofstream m_incomingLog;
  std::string m_lossLog;
  std::string m_macDelayLog;

  // std::ofstream m_delayLogFile;
  void WriteAverageDelayLog();
  std::ofstream m_macDelayLogFile;
  std::ofstream m_lossLogFile;
  std::ofstream m_delayLogFileDist1;
  std::ofstream m_delayLogFileDist2;


  /**
   * start the transmission of a packet by contacting the PHY layer
   */
  void StartTransmission ();


  std::vector<int> m_slotsTracker;    //!< To keep track of slots already used in a frame so that the next copy will have these slots in the repetations tag header
  int m_repetationsTracker;           //!< To keep track of how many copies of the packet have been sent so that we can stop when the number of repetations is reached and reinitilaze the slots tracker

  Time m_slotDuration;        //!< Slot duration

  Ptr<Queue<Packet> > m_queue; //!< packet queue
  
  TracedCallback<Ptr<const Packet> > m_macTxTrace;        //!< Tx trace
  TracedCallback<Ptr<const Packet> > m_macTxDropTrace;    //!< Tx Drop trace
  TracedCallback<Ptr<const Packet> > m_macPromiscRxTrace; //!< Promiscuous Rx trace
  TracedCallback<Ptr<const Packet> > m_macRxTrace;        //!< Rx trace

  Ptr<Node>    m_node;    //!< Node owning this NetDevice
  Ptr<Channel> m_channel; //!< Channel

  Mac48Address m_address; //!< MAC address

  NetDevice::ReceiveCallback m_rxCallback;                //!< Rx callback
  NetDevice::PromiscReceiveCallback m_promiscRxCallback;  //!< Promiscuous Rx callback

  GenericPhyTxStartCallback m_phyMacTxStartCallback;      //!< Tx Start callback

  /**
   * List of callbacks to fire if the link changes state (up or down).
   */
  TracedCallback<> m_linkChangeCallbacks;


  uint32_t m_ifIndex;     //!< Interface index
  mutable uint32_t m_mtu; //!< NetDevice MTU
  bool m_linkUp;          //!< true if the link is up

  State m_state;          //!< State of the NetDevice
  Ptr<Packet> m_currentPkt;  //!< Current packet
  Ptr<Object> m_phy;      //!< PHY object
};


} // namespace ns3

#endif /* ALOHA_NOACK_NET_DEVICE_H */
