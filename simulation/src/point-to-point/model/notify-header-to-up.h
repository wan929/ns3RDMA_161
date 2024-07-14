/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 University of Washington
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
 */

#ifndef Notify_HEADER_To_Up_H
#define Notify_HEADER_To_Up_H

#include "ns3/header.h"

namespace ns3 {

/**
 * \ingroup point-to-point
 * \brief Packet header for PPP
 *
 * This class can be used to add a header to PPP packet.  Currently we do not
 * implement any of the state machine in \RFC{1661}, we just encapsulate the
 * inbound packet send it on.  The goal here is not really to implement the
 * point-to-point protocol, but to encapsulate our packets in a known protocol
 * so packet sniffers can parse them.
 *
 * if PPP is transmitted over a serial link, it will typically be framed in
 * some way derivative of IBM SDLC (HDLC) with all that that entails.
 * Thankfully, we don't have to deal with all of that -- we can use our own
 * protocol for getting bits across the serial link which we call an ns3 
 * Packet.  What we do have to worry about is being able to capture PPP frames
 * which are understandable by Wireshark.  All this means is that we need to
 * teach the PcapWriter about the appropriate data link type (DLT_PPP = 9),
 * and we need to add a PPP header to each packet.  Since we are not using
 * framed PPP, this just means prepending the sixteen bit PPP protocol number
 * to the packet.  The ns-3 way to do this is via a class that inherits from
 * class Header.
 */
class NotifyHeaderToUp : public Header
{
public:

  NotifyHeaderToUp (uint16_t notifyleft, uint16_t notifyright);
  NotifyHeaderToUp();
  virtual ~NotifyHeaderToUp ();


  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;

  void SetNotifyLeft (uint16_t notifyleft);
  uint16_t GetNotifyLeft () const;

  void SetNotifyRight (uint16_t notifyright);
  uint16_t GetNotifyRight () const;

  void SetlatestRxSeqNo(uint16_t latestRxSeqNo);
  uint16_t GetlatestRxSeqNo() const;


private:

  /**
   * \brief The PPP protocol type of the payload packet
   */
  uint16_t m_notifyleft;
  uint16_t m_notifyright;
  uint16_t m_latestRxSeqNo;
};

} // namespace ns3


#endif /* PPP_HEADER_H */