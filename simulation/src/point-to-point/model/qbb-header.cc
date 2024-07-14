#include <stdint.h>
#include <iostream>
#include "qbb-header.h"
#include "ns3/buffer.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE("qbbHeader");

namespace ns3 {

	NS_OBJECT_ENSURE_REGISTERED(qbbHeader);

	qbbHeader::qbbHeader(uint16_t pg)
		//: m_pg(pg), sport(0), dport(0), flags(0), m_seq(0)  //0
		//: m_pg(pg), sport(0), dport(0), flags(0), m_seq(0), m_sequence(0)  //1
		: m_pg(pg), sport(0), dport(0), flags(0), m_seq(0), m_sequence(0), m_latestRxSeqNo(0)  //2
	{
	}

	qbbHeader::qbbHeader()
		//: m_pg(0), sport(0), dport(0), flags(0), m_seq(0)
		//: m_pg(0), sport(0), dport(0), flags(0), m_seq(0), m_sequence(0)  //1
		: m_pg(0), sport(0), dport(0), flags(0), m_seq(0), m_sequence(0), m_latestRxSeqNo(0)  //2
	{} 

	qbbHeader::~qbbHeader()
	{}

	void qbbHeader::SetPG(uint16_t pg) //change, pg_round
	{
		//m_pg = (pg >> 8) & 0xff;
		m_pg = pg;
	}

	void qbbHeader::SetSeq(uint32_t seq)
	{
		m_seq = seq;
	}

	void qbbHeader::SetSport(uint32_t _sport){
		sport = _sport;
	}
	void qbbHeader::SetDport(uint32_t _dport){
		dport = _dport;
	}

	void qbbHeader::SetTs(uint64_t ts){
		NS_ASSERT_MSG(IntHeader::mode == 1, "qbbHeader cannot SetTs when IntHeader::mode != 1");
		ih.ts = ts;
	}
	void qbbHeader::SetCnp(){
		flags |= 1 << FLAG_CNP;
	}
	void qbbHeader::SetLast(){
		flags |= 1 << FLAG_LAST;
	}
	void qbbHeader::SetPendingAck()
	{
		flags |= 1 << FLAG_PENDINGACK;
	}
	void qbbHeader::SetCopyRecirculate(){
  		flags |= 1 << FLAG_COPYRECIRCULATE;
	}
	void qbbHeader::SetReorderingRecirculate(){
  		flags |= 1 << FLAG_REORDERINGRECIRCULATE;
	}
	void qbbHeader::SetSwitchNACK(){
		flags |= 1 << FLAG_SWITCHNACK;
	}
	void qbbHeader::SetSequence(uint16_t sequence){
		// if(MineAckHeader::mode == 1) //notification
		// {
		// 	mah.m_sequence = sequence;
		// }
		m_sequence = sequence;
	}
	void qbbHeader::SetlatestRxSeqNo(uint16_t latestRxSeqNo){
		m_latestRxSeqNo = latestRxSeqNo;
	}
	void qbbHeader::SetIntHeader(const IntHeader &_ih){
		//std::cout << _ih.nhop << std::endl;
		ih = _ih;
	}

	uint16_t qbbHeader::GetPG() const
	{
		return m_pg;
	}

	uint32_t qbbHeader::GetSeq() const
	{
		return m_seq;
	}

	uint16_t qbbHeader::GetSport() const{
		return sport;
	}
	uint16_t qbbHeader::GetDport() const{
		return dport;
	}

	uint64_t qbbHeader::GetTs() const {
		NS_ASSERT_MSG(IntHeader::mode == 1, "qbbHeader cannot GetTs when IntHeader::mode != 1");
		return ih.ts;
	}
	uint8_t qbbHeader::GetCnp() const{
		return (flags >> FLAG_CNP) & 1;
	}
	uint8_t qbbHeader::GetLast() const{
		return (flags >> FLAG_LAST) & 1;
	}
	uint8_t qbbHeader::GetPendingAck() const{
		return (flags >> FLAG_PENDINGACK) & 1;
	}
	uint8_t qbbHeader::GetCopyRecirculate() const{
  		return (flags >> FLAG_COPYRECIRCULATE) & 1;
	}
	uint8_t qbbHeader::GetReorderingRecirculate() const{
  		return (flags >> FLAG_REORDERINGRECIRCULATE) & 1;
	}
	uint8_t qbbHeader::GetSwitchNACK() const{
  		return (flags >> FLAG_SWITCHNACK) & 1;
	}
	uint16_t qbbHeader::GetSequence() const{
		// NS_ASSERT_MSG(MineAckHeader::mode == 1, "SeqTsHeader cannot GetSequence when MineAckHeader::mode != 1");
		// return mah.m_sequence;
		return m_sequence;
	}
	uint16_t qbbHeader::GetlatestRxSeqNo() const{
		return m_latestRxSeqNo;
	}


	TypeId
		qbbHeader::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::qbbHeader")
			.SetParent<Header>()
			.AddConstructor<qbbHeader>()
			;
		return tid;
	}
	TypeId
		qbbHeader::GetInstanceTypeId(void) const
	{
		return GetTypeId();
	}
	void qbbHeader::Print(std::ostream &os) const
	{
		os << "qbb:" << "pg=" << m_pg << ",seq=" << m_seq;
	}
	uint32_t qbbHeader::GetSerializedSize(void)  const
	{
		//return GetBaseSize() + MineAckHeader::GetStaticSize() + IntHeader::GetStaticSize();
		return GetBaseSize() + IntHeader::GetStaticSize();
	}
	uint32_t qbbHeader::GetBaseSize() {
		qbbHeader tmp;
		//0
		//return sizeof(tmp.sport) + sizeof(tmp.dport) + sizeof(tmp.flags) + sizeof(tmp.m_pg) + sizeof(tmp.m_seq);

		//1
		//return sizeof(tmp.sport) + sizeof(tmp.dport) + sizeof(tmp.flags) + sizeof(tmp.m_pg) + sizeof(tmp.m_seq) + sizeof(tmp.m_sequence);
		
		//2
		return sizeof(tmp.sport) + sizeof(tmp.dport) + sizeof(tmp.flags) + sizeof(tmp.m_pg) + sizeof(tmp.m_seq) + sizeof(tmp.m_sequence) + sizeof(tmp.m_latestRxSeqNo);
	}
	void qbbHeader::Serialize(Buffer::Iterator start)  const
	{
		Buffer::Iterator i = start;
		i.WriteU16(sport);
		i.WriteU16(dport);
		i.WriteU16(flags);
		i.WriteU16(m_pg);
		i.WriteU32(m_seq);
		i.WriteU16(m_sequence);
		i.WriteU16(m_latestRxSeqNo);
		//mah.Serialize(i);
		// write IntHeader
		ih.Serialize(i);
	}

	uint32_t qbbHeader::Deserialize(Buffer::Iterator start)
	{
		Buffer::Iterator i = start;
		sport = i.ReadU16();
		dport = i.ReadU16();
		flags = i.ReadU16();
		m_pg = i.ReadU16();
		m_seq = i.ReadU32();
		m_sequence = i.ReadU16();
		m_latestRxSeqNo = i.ReadU16();
		//mah.Deserialize(i);
		// read IntHeader
		ih.Deserialize(i);
		return GetSerializedSize();
	}
}; // namespace ns3
