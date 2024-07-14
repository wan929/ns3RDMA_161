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

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "seq-ts-header.h"

NS_LOG_COMPONENT_DEFINE ("SeqTsHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SeqTsHeader);

SeqTsHeader::SeqTsHeader ()
  : m_seq (0), m_pg(0), m_flags(0), m_sequence(0), m_round(0)
  //: m_seq (0), m_pg(0), m_opcode(0)
{
	if (IntHeader::mode == 1)
		ih.ts = Simulator::Now().GetTimeStep();
}

void
SeqTsHeader::SetSeq (uint32_t seq)
{
  m_seq = seq;
}
uint32_t
SeqTsHeader::GetSeq (void) const
{
  return m_seq;
}

void
SeqTsHeader::SetPG (uint16_t pg)
{
	//m_pg_m_round = (m_pg_m_round & 0xff) | (pg << 8);
  m_pg = pg;
}

uint16_t
SeqTsHeader::GetPG (void) const
{
  //return (m_pg_m_round >> 8) & 0xff;
  return m_pg;
}

void SeqTsHeader::SetFirst (){
  m_flags |= 1 << FLAG_FIRST;
}
void SeqTsHeader::SetMiddle (){
  m_flags |= 1 << FLAG_MIDDLE;
}
void SeqTsHeader::SetLast (){
  m_flags |= 1 << FLAG_LAST;
}
void SeqTsHeader::SetPendingAck(){
  m_flags |= 1 << FLAG_PENDINGACK;
}
void SeqTsHeader::SetCopyRecirculate(){
  m_flags |= 1 << FLAG_COPYRECIRCULATE;
}
void SeqTsHeader::SetReorderingRecirculate(){
  m_flags |= 1 << FLAG_REORDERINGRECIRCULATE;
}
void SeqTsHeader::SetNotDataFilter(){
  m_flags |= 1 << FLAG_NOTDATAFILTER;
}

uint8_t SeqTsHeader::GetFirst () const{
  return (m_flags >> FLAG_FIRST) & 1;
}
uint8_t SeqTsHeader::GetMiddle () const{
  return (m_flags >> FLAG_MIDDLE) & 1;
}
uint8_t SeqTsHeader::GetLast () const{
  return (m_flags >> FLAG_LAST) & 1;
}
uint8_t SeqTsHeader::GetPendingAck() const{
  return (m_flags >> FLAG_PENDINGACK) & 1;
}
uint8_t SeqTsHeader::GetCopyRecirculate() const{
  return (m_flags >> FLAG_COPYRECIRCULATE) & 1;
}
uint8_t SeqTsHeader::GetReorderingRecirculate() const{
  return (m_flags >> FLAG_REORDERINGRECIRCULATE) & 1;
}
uint8_t SeqTsHeader::GetNotDataFilter() const{
  return (m_flags >> FLAG_NOTDATAFILTER) & 1;
}
//in-network notification

void
SeqTsHeader::SetSequence (uint16_t sequence)
{
  // if(MineUdpHeader::mode == 1) //notification
  // {
  //   muh.m_sequence = sequence;
  // }
  m_sequence = sequence;
}
uint16_t
SeqTsHeader::GetSequence (void) const
{
  // NS_ASSERT_MSG(MineUdpHeader::mode == 1, "SeqTsHeader cannot GetSequence when MineUdpHeader::mode != 1");
  // return muh.m_sequence;
  return m_sequence;
}

void
SeqTsHeader::SetRound (uint16_t round)
{
  // if(MineUdpHeader::mode == 1) //notification
  // {
  //   //m_pg_m_round = (m_pg_m_round & 0xff00) | round;
  //   muh.m_round = round;
  // }
  m_round = round;
}

uint16_t
SeqTsHeader::GetRound (void) const
{
  // NS_ASSERT_MSG(MineUdpHeader::mode == 1, "SeqTsHeader cannot GetRound when MineUdpHeader::mode != 1");
	// //return m_pg_m_round & 0xff;
  // return muh.m_round;
  return m_round;
}

Time
SeqTsHeader::GetTs (void) const
{
	NS_ASSERT_MSG(IntHeader::mode == 1, "SeqTsHeader cannot GetTs when IntHeader::mode != 1");
	return TimeStep (ih.ts);
}

TypeId
SeqTsHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SeqTsHeader")
    .SetParent<Header> ()
    .AddConstructor<SeqTsHeader> ()
  ;
  return tid;
}
TypeId
SeqTsHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void
SeqTsHeader::Print (std::ostream &os) const
{
	os << m_seq << " " << m_pg << " " << m_flags;
  //os << m_seq << " " << m_pg << " " << m_opcode;
}

uint32_t
SeqTsHeader::GetSerializedSize (void) const
{
	return GetHeaderSize();
}

uint32_t SeqTsHeader::GetHeaderSize(void){
//o
	//return 8 + IntHeader::GetStaticSize();

//n
	return 12 + IntHeader::GetStaticSize();

  //return 8 + MineUdpHeader::GetStaticSize() + IntHeader::GetStaticSize();
}

void
SeqTsHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteHtonU32 (m_seq);
  i.WriteHtonU16 (m_pg);
  i.WriteHtonU16 (m_flags);
//in-network notification
  i.WriteHtonU16 (m_sequence);
  i.WriteHtonU16 (m_round);
  //muh.Serialize(i);
  // write IntHeader
  ih.Serialize(i);
}
uint32_t
SeqTsHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_seq = i.ReadNtohU32 ();
  m_pg = i.ReadNtohU16 ();
  m_flags = i.ReadNtohU16 ();
//in-network notification
  m_sequence = i.ReadNtohU16 ();
  m_round = i.ReadNtohU16 ();
  //muh.Deserialize(i);
  // read IntHeader
  ih.Deserialize(i);
  return GetSerializedSize ();
}

} // namespace ns3
