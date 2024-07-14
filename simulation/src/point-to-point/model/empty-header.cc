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
#include "empty-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EmptyHeader");

NS_OBJECT_ENSURE_REGISTERED (EmptyHeader);

EmptyHeader::EmptyHeader ()
{
}

EmptyHeader::~EmptyHeader ()
{
}

TypeId
EmptyHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EmptyHeader")
    .SetParent<Header> ()
    .AddConstructor<EmptyHeader> ()
  ;
  return tid;
}


TypeId
EmptyHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}


void 
EmptyHeader::Print (std::ostream &os) const
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
EmptyHeader::GetSerializedSize (void) const
{
  return 4;
}

//useful
void
EmptyHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteU16 (m_seq);
  start.WriteU16 (m_pendingackflags);
}

uint32_t
EmptyHeader::Deserialize (Buffer::Iterator start)
{
  m_seq = start.ReadU16 ();
  m_pendingackflags = start.ReadU16 ();
  return GetSerializedSize ();
}

void
EmptyHeader::SetSeq (uint16_t seq)
{
  m_seq = seq;
}

uint16_t
EmptyHeader::GetSeq (void)
{
  return m_seq;
}

void EmptyHeader::SetPendingAckFlags (uint16_t pendingackflags)
{
  m_pendingackflags = pendingackflags;
}
uint16_t EmptyHeader::GetPendingAckFlags (void){
  return m_pendingackflags;
}
} // namespace ns3