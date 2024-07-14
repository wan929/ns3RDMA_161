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
#include "extra-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ExtraHeader");

NS_OBJECT_ENSURE_REGISTERED (ExtraHeader);

ExtraHeader::ExtraHeader (uint16_t sequence, uint16_t latestRxSeqNo)
  : m_sequence(sequence), m_latestRxSeqNo(latestRxSeqNo)
{
}

ExtraHeader::ExtraHeader ()
  : m_sequence(0), m_latestRxSeqNo(0)
{
}

ExtraHeader::~ExtraHeader ()
{}

TypeId
ExtraHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ExtraHeader")
    .SetParent<Header> ()
    .AddConstructor<ExtraHeader> ()
    ;
  return tid;
}


TypeId
ExtraHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}


void 
ExtraHeader::Print (std::ostream &os) const
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
ExtraHeader::GetSerializedSize (void) const
{
  return 4;
}

//useful
void
ExtraHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU16 (m_sequence);
  start.WriteU16 (m_latestRxSeqNo);
}

uint32_t
ExtraHeader::Deserialize (Buffer::Iterator start)
{
  m_sequence = start.ReadU16 ();
  m_latestRxSeqNo = start.ReadU16 ();
  return GetSerializedSize ();
}

void
ExtraHeader::SetSequence (uint16_t sequence)
{
  m_sequence = sequence;
}

uint16_t
ExtraHeader::GetSequence () const
{
  return m_sequence;
}

void 
ExtraHeader::SetlatestRxSeqNo (uint16_t latestRxSeqNo)
{
  m_latestRxSeqNo = latestRxSeqNo;
}

uint16_t 
ExtraHeader::GetlatestRxSeqNo () const
{
  return m_latestRxSeqNo;
}

} // namespace ns3