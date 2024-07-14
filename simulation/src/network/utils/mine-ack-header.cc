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

#include "mine-ack-header.h"

namespace ns3 {

MineAckHeader::Mode MineAckHeader::mode = NONE;

MineAckHeader::MineAckHeader ()
  : m_sequence(0)
{

}

//in-network notification
void
MineAckHeader::SetSequence (uint16_t sequence)
{
  m_sequence = sequence;
}

uint16_t
MineAckHeader::GetSequence (void) const
{
  return m_sequence;
}

uint32_t MineAckHeader::GetStaticSize(){
	if(mode == NOTIFICATION)
	{
		return 2;
	}
	return 0;
}


void
MineAckHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  if(mode == NOTIFICATION)
  {
	  i.WriteHtonU16 (m_sequence);
  }
}

uint32_t
MineAckHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  if(mode == NOTIFICATION)
  {
	  m_sequence = i.ReadNtohU16 ();
  }
  return GetStaticSize ();
}

} // namespace ns3
