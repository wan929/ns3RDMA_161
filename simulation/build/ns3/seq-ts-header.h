/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef SEQ_TS_HEADER_H
#define SEQ_TS_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"
#include "ns3/int-header.h"
#include "ns3/mine-udp-header.h"

namespace ns3 {
/**
 * \ingroup udpclientserver
 * \class SeqTsHeader
 * \brief Packet header for Udp client/server application
 * The header is made of a 32bits sequence number followed by
 * a 64bits time stamp.
 */
class SeqTsHeader : public Header
{
public:
  enum {
	    FLAG_FIRST = 0,
      FLAG_MIDDLE = 1,
      FLAG_LAST = 2,
      FLAG_PENDINGACK = 8,
      FLAG_COPYRECIRCULATE = 9,
      FLAG_REORDERINGRECIRCULATE = 10,
      FLAG_NOTDATAFILTER = 11
  };

  SeqTsHeader ();

  /**
   * \param seq the sequence number
   */
  void SetSeq (uint32_t seq);
  /**
   * \return the sequence number
   */
  uint32_t GetSeq (void) const;
  /**
   * \return the time stamp
   */
  Time GetTs (void) const;

  void SetPG (uint16_t pg);
  uint16_t GetPG () const;

  void SetFirst ();
  void SetMiddle ();
  void SetLast ();
  void SetPendingAck();
  void SetCopyRecirculate();
  void SetReorderingRecirculate();
  void SetNotDataFilter();
  uint8_t GetFirst () const;
  uint8_t GetMiddle () const;
  uint8_t GetLast () const;
  uint8_t GetPendingAck() const;
  uint8_t GetCopyRecirculate() const;
  uint8_t GetReorderingRecirculate() const;
  uint8_t GetNotDataFilter() const;

//in-network notification
  void SetSequence (uint16_t sequence);
  uint16_t GetSequence (void) const;

  void SetRound (uint16_t round);
  uint16_t GetRound (void) const;

  void SetlatestRxSeqNo(uint16_t latestRxSeqNo);
  uint16_t GetlatestRxSeqNo() const;

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  static uint32_t GetHeaderSize(void);
private:
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  uint32_t m_seq;
  uint16_t m_pg;
  uint16_t m_flags; //first 0, middle 1, last 2
//in-network notification 
  uint16_t m_sequence;
  uint16_t m_round;
public:
  //MineUdpHeader muh;
  IntHeader ih;
};

} // namespace ns3

#endif /* SEQ_TS_HEADER_H */
