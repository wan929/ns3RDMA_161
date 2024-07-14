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

#include "mine-udp-header.h"

namespace ns3 {

MineUdpHeader::Mode MineUdpHeader::mode = NONE;

MineUdpHeader::MineUdpHeader ()
  : m_sequence(0), m_round(0)
{

}

//in-network notification
void
MineUdpHeader::SetSequence (uint16_t sequence)
{
  m_sequence = sequence;
}

uint16_t
MineUdpHeader::GetSequence (void) const
{
  return m_sequence;
}

void
MineUdpHeader::SetRound (uint16_t round)
{
  m_round = round;
}

uint16_t
MineUdpHeader::GetRound (void) const
{
  return m_round;
}

uint32_t MineUdpHeader::GetStaticSize(){
	if(mode == NOTIFICATION)
	{
		return 4;
	}
	return 0;
}

void
MineUdpHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  if(mode == NOTIFICATION)
  {
	  i.WriteHtonU16 (m_sequence);
  	i.WriteHtonU16 (m_round);
  }
}

uint32_t
MineUdpHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  if(mode == NOTIFICATION)
  {
	  m_sequence = i.ReadNtohU16 ();
    m_round = i.ReadNtohU16 ();
  }
  return GetStaticSize ();
}

} // namespace ns3
