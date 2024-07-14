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

#include <iostream>
#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "notify-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NotifyHeader");

NS_OBJECT_ENSURE_REGISTERED (NotifyHeader);

NotifyHeader::NotifyHeader (uint32_t notifyseq)
  //: sport(0), dport(0), m_notifyseq(notifyseq)
  : sport(0), dport(0), m_notifyseq(notifyseq), m_round(0), m_sequence(0)
{
}

NotifyHeader::NotifyHeader ()
  //: sport(0), dport(0), m_notifyseq(0)
  : sport(0), dport(0), m_notifyseq(0), m_round(0), m_sequence(0)
{}

NotifyHeader::~NotifyHeader ()
{}

TypeId
NotifyHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NotifyHeader")
    .SetParent<Header> ()
    .AddConstructor<NotifyHeader> ()
    ;
  return tid;
}


TypeId
NotifyHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}


void 
NotifyHeader::Print (std::ostream &os) const
{
  /*
  std::string proto;

  switch(m_protocol)
    {
    case 0x0021: // IPv4
      proto = "IP (0x0021)";
      break;
    case 0x0057: // IPv6 
      proto = "IPv6 (0x0057)";
      break;
    default:
      NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
    }
  os << "Point-to-Point Protocol: " << proto; 
  */
}


uint32_t
NotifyHeader::GetSerializedSize (void) const
{
  return 13;
}

//useful
void
NotifyHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU16 (sport);
	start.WriteU16 (dport);
  start.WriteU32 (m_notifyseq);
  start.WriteU16 (m_round);
  start.WriteU8 (m_pg);
  start.WriteU16 (m_sequence);
}

uint32_t
NotifyHeader::Deserialize (Buffer::Iterator start)
{
  sport = start.ReadU16();
	dport = start.ReadU16();
  m_notifyseq = start.ReadU32 ();
  m_round = start.ReadU16 ();
  m_pg = start.ReadU8 ();
  m_sequence = start.ReadU16 ();
  return GetSerializedSize ();
}

void
NotifyHeader::SetNotifySeq(uint32_t notifyseq)
{
  m_notifyseq = notifyseq;
}

uint32_t
NotifyHeader::GetNotifySeq () const
{
  return m_notifyseq;
}

void NotifyHeader::SetSport(uint32_t _sport){
	sport = _sport;
}
void NotifyHeader::SetDport(uint32_t _dport){
	dport = _dport;
}

uint16_t NotifyHeader::GetSport() const{
	return sport;
}
uint16_t NotifyHeader::GetDport() const{
	return dport;
}

void 
NotifyHeader::SetLossRound (uint16_t loss_round)
{
  m_round = loss_round;
}

uint16_t 
NotifyHeader::GetLossRound () const
{
  return m_round;
}

void 
NotifyHeader::SetPG (uint8_t pg)
{
  m_pg = pg;
}

uint8_t
NotifyHeader::GetPG () const
{
  return m_pg;
}

void NotifyHeader::SetSequence (uint16_t sequence)
{
  m_sequence = sequence;
}

uint16_t 
NotifyHeader::GetSequence () const
{
  return m_sequence;
}
} // namespace ns3