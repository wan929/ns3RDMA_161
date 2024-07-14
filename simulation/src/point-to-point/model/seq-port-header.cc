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
#include "seq-port-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SeqPortHeader");

NS_OBJECT_ENSURE_REGISTERED (SeqPortHeader);

SeqPortHeader::SeqPortHeader (uint16_t sequence, uint8_t pidx, uint64_t ts)
  : m_sequence(sequence), m_pidx(pidx), m_ts(ts)
{
}

SeqPortHeader::SeqPortHeader ()
  : m_sequence(0), m_pidx(0), m_ts(0)
{
}

SeqPortHeader::~SeqPortHeader ()
{}

TypeId
SeqPortHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SeqPortHeader")
    .SetParent<Header> ()
    .AddConstructor<SeqPortHeader> ()
    ;
  return tid;
}


TypeId
SeqPortHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}


void 
SeqPortHeader::Print (std::ostream &os) const
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
SeqPortHeader::GetSerializedSize (void) const
{
  return 11;
}

//useful
void
SeqPortHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU16 (m_sequence);
  start.WriteU8 (m_pidx);
  start.WriteU64 (m_ts);
}

uint32_t
SeqPortHeader::Deserialize (Buffer::Iterator start)
{
  m_sequence = start.ReadU16 ();
  m_pidx = start.ReadU8 ();
  m_ts = start.ReadU64 ();
  return GetSerializedSize ();
}

void
SeqPortHeader::SetSequence (uint16_t sequence)
{
  m_sequence = sequence;
}

uint16_t
SeqPortHeader::GetSequence () const
{
  return m_sequence;
}

void
SeqPortHeader::SetPidx (uint8_t pidx)
{
  m_pidx = pidx;
}

uint8_t
SeqPortHeader::GetPidx () const
{
  return m_pidx;
}

void 
SeqPortHeader::SetTs (uint64_t ts)
{
  m_ts =ts;
}

uint64_t 
SeqPortHeader::GetTs () const
{
  return m_ts;
}

/*
void
SeqPortHeader::SetRound (uint8_t round)
{
  m_round = round;
}

uint8_t
SeqPortHeader::GetRound () const
{
  return m_round;
}
*/
} // namespace ns3