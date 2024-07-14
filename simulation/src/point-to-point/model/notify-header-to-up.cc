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
#include "notify-header-to-up.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NotifyHeaderToUp");

NS_OBJECT_ENSURE_REGISTERED (NotifyHeaderToUp);

NotifyHeaderToUp::NotifyHeaderToUp (uint16_t notifyleft, uint16_t notifyright)
  : m_notifyleft(notifyleft), m_notifyright(notifyright)
{
}

NotifyHeaderToUp::NotifyHeaderToUp ()
  : m_notifyleft(0), m_notifyright(0)
{
}

NotifyHeaderToUp::~NotifyHeaderToUp ()
{}

TypeId
NotifyHeaderToUp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NotifyHeaderToUp")
    .SetParent<Header> ()
    .AddConstructor<NotifyHeaderToUp> ()
    ;
  return tid;
}


TypeId
NotifyHeaderToUp::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}


void 
NotifyHeaderToUp::Print (std::ostream &os) const
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
NotifyHeaderToUp::GetSerializedSize (void) const
{
  return 4;
}

//useful
void
NotifyHeaderToUp::Serialize (Buffer::Iterator start) const
{
  start.WriteU16 (m_notifyleft);
  start.WriteU16 (m_notifyright);
}

uint32_t
NotifyHeaderToUp::Deserialize (Buffer::Iterator start)
{
  m_notifyleft = start.ReadU16 ();
  m_notifyright = start.ReadU16 ();
  return GetSerializedSize ();
}

void
NotifyHeaderToUp::SetNotifyLeft (uint16_t notifyleft)
{
  m_notifyleft = notifyleft;
}

uint16_t
NotifyHeaderToUp::GetNotifyLeft () const
{
  return m_notifyleft;
}

void
NotifyHeaderToUp::SetNotifyRight (uint16_t notifyright)
{
  m_notifyright = notifyright;
}

uint16_t
NotifyHeaderToUp::GetNotifyRight () const
{
  return m_notifyright;
}

} // namespace ns3