/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 New York University
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
 * Author: Adrian S.-W. Tam <adrian.sw.tam@gmail.com>
 */

#include <stdint.h>
#include <iostream>
#include "lg-pause-header.h"
#include "ns3/buffer.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("LGPauseHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (LGPauseHeader);

LGPauseHeader::LGPauseHeader (uint16_t lgtype, uint8_t qindex)
  : m_lgtype(lgtype), m_qindex(qindex)
{
}

LGPauseHeader::LGPauseHeader ()
  : m_lgtype(0)
{}

LGPauseHeader::~LGPauseHeader ()
{}

void LGPauseHeader::SetType (uint16_t _lgtype)
{
  m_lgtype = _lgtype;
}

uint16_t LGPauseHeader::GetType () const
{
  return m_lgtype;
}

void LGPauseHeader::SetQIndex (uint8_t qindex)
{
  m_qindex = qindex;
}

uint8_t LGPauseHeader::GetQIndex () const
{
  return m_qindex;
}

TypeId
LGPauseHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LGPauseHeader")
    .SetParent<Header> ()
    .AddConstructor<LGPauseHeader> ()
    ;
  return tid;
}
TypeId
LGPauseHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void LGPauseHeader::Print (std::ostream &os) const
{
  os << "pause=" << m_lgtype <<", qidx=" << int(m_qindex);
}
uint32_t LGPauseHeader::GetSerializedSize (void)  const
{
  return 3;
}
void LGPauseHeader::Serialize (Buffer::Iterator start)  const
{
  start.WriteU16 (m_lgtype);
  start.WriteU8 (m_qindex);
}

uint32_t LGPauseHeader::Deserialize (Buffer::Iterator start)
{
  m_lgtype = start.ReadU16 ();
  m_qindex = start.ReadU8 ();
  return GetSerializedSize ();
}


}; // namespace ns3
