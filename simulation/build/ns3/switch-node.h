#ifndef SWITCH_NODE_H
#define SWITCH_NODE_H

#include <unordered_map>
#include <ns3/node.h>
#include "qbb-net-device.h"
#include "switch-mmu.h"
#include "pint.h"
#include "ns3/packet.h"
#include "ns3/queue.h"
#include "ns3/object-factory.h"
#include "ns3/notify-header.h"
#include "ns3/ptr.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/empty-header.h"
#include <unordered_map>

namespace ns3 {

class Packet;

class SwitchNode : public Node{
	static const uint32_t pCnt = 257;	// Number of ports used
	static const uint32_t qCnt = 8;	// Number of queues/priorities used

	uint32_t m_ecmpSeed;
	std::unordered_map<uint32_t, std::vector<int> > m_rtTable; // map from ip address (u32) to possible ECMP port (index of dev)
	
	// monitor of PFC
	uint32_t m_bytes[pCnt][pCnt][qCnt]; // m_bytes[inDev][outDev][qidx] is the bytes from inDev enqueued for outDev at qidx
	
	uint64_t m_txBytes[pCnt]; // counter of tx bytes

	uint32_t m_lastPktSize[pCnt];
	uint64_t m_lastPktTs[pCnt]; // ns
	double m_u[pCnt];

	static const uint32_t resume_threshold = 37888; //105000
	static const uint32_t pause_threshold = 40888; //110000

protected:
    uint32_t lost_seq_udp;
	uint32_t lost_seq_ack;
	uint32_t lost_seq_nack;

	bool m_ecnEnabled;
	uint32_t m_ccMode;
	uint32_t m_RecMode;
	uint64_t m_maxRtt;
	uint32_t m_ackHighPrio; // set high priority for ACK/NACK
///*
  	uint16_t m_counter[257]; // down expected seq
  	uint16_t m_seqnumber[257]; // up seqNo
  	uint16_t m_uplatestRxSeqNo[257]; // up lastest seq
	uint16_t m_downlatestRxSeqNo[257]; //down lastest seq
	uint16_t m_downPendingAck[257]; //down pending ack,可以用flag区分
	uint32_t start_state[257];
	uint16_t m_ackNo[257];
	uint16_t m_curr_state[257];
	
	Ptr<Queue> m_copyqueue;
	std::vector<Ptr<Queue> > m_copyqueues;
	std::vector<Ptr<Queue> > m_reorderingqueues;
	std::unordered_map<uint16_t, std::vector<uint16_t>> m_reTxReqs;
	uint32_t m_copysize[257];
	uint32_t m_reorderingsize[257];
	
	enum CopyRecirculateTxMachineState
 	{
   		Copy_READY,   /**< The transmitter is ready to begin transmission of a packet */
   		Copy_BUSY     /**< The transmitter is busy transmitting a packet */
 	};
	CopyRecirculateTxMachineState m_CopyrecirculatetxMachineState[257];
	//std::vector<CopyRecirculateTxMachineState> m_CopyrecirculatetxMachineState;
  	//CopyRecirculateTxMachineState m_CopyrecirculatetxMachineState;

	enum ReorderingRecirculateTxMachineState
 	{
   		Reordering_READY,   /**< The transmitter is ready to begin transmission of a packet */
   		Reordering_BUSY     /**< The transmitter is busy transmitting a packet */
 	};
	ReorderingRecirculateTxMachineState m_ReorderingrecirculatetxMachineState[257];
	//std::vector<ReorderingRecirculateTxMachineState> m_ReorderingrecirculatetxMachineState;
  	//ReorderingRecirculateTxMachineState m_ReorderingrecirculatetxMachineState;

	struct FiveTuple{
    	uint16_t sport;
		uint16_t dport;
    	uint32_t sip;
		uint32_t dip;
		uint8_t protocol;
	}Fivetuple;

	struct DataFilterValue{
		FiveTuple filter_flowid;
		uint32_t filter_seq;
		uint64_t filter_time;
		uint16_t filter_round;
	}DataFiltervalue;

	struct NACKFilterValue{
		FiveTuple filter_flowid;
		uint32_t filter_seq;
		uint64_t filter_time;
		uint16_t filter_round;
		uint8_t vaild;
	}NACKFiltervalue;

	struct LookUpTableEntry{
    	uint16_t sport;
		uint16_t dport;
    	uint32_t sip;
		uint32_t dip;
		uint8_t protocol;
		uint32_t seq;
		uint16_t pg;
		uint16_t round;
		uint16_t flag;
		uint32_t size;
	}Tableentry;

	//Datafilter
	std::unordered_map<uint16_t, DataFilterValue> DataFilter;

	//NACKfilter
	std::unordered_map<uint16_t, NACKFilterValue> NACKFilter;

	//lookuptable
	std::unordered_map<uint16_t, std::vector<LookUpTableEntry>> lookuptable;
//*/
private:
	int GetOutDev(Ptr<const Packet>, CustomHeader &ch);
	void SendToDev(Ptr<Packet>p, CustomHeader &ch);
	static uint32_t EcmpHash(const uint8_t* key, size_t len, uint32_t seed);
	void CheckAndSendPfc(uint32_t inDev, uint32_t qIndex);
	void CheckAndSendResume(uint32_t inDev, uint32_t qIndex);
//TOR
	bool ToIsSwitch(uint32_t pidx);
	bool IsTorSwitch();
	int FilterNACKPacket(FiveTuple fivetuple, uint16_t p_round, uint32_t p_seq);
	void RecordNACKEseq(CustomHeader &ch);
	bool IsNACKEseq(CustomHeader &ch);
//filter
	void SetFlowIDNull(FiveTuple id);
	bool IsNullFlowID(FiveTuple id);
	bool IsEqualFlowID(FiveTuple id1, FiveTuple id2);
	int FilterPacket_SADRCV(FiveTuple fivetuple, uint32_t p_seq, uint16_t p_round, bool isloss); // SADR_CV
	int FilterPacket_SADRSV(FiveTuple fivetuple, uint32_t p_seq, uint16_t p_round, bool isloss);// SADR_SV
	int FilterPacket_SADRSLR(FiveTuple fivetuple, uint32_t p_seq, uint16_t p_round, bool isloss);// SADR_SV
	uint32_t MurmurHash3(const void* key, int len, uint32_t seed);
//in-network notification
	uint16_t CheckAndSendNotifyToSender(uint32_t inDev, uint16_t sequence, Ptr<Packet> p, CustomHeader &ch);
	void SendNotifyToSender(uint32_t m_index, uint16_t left, uint16_t right);
	Ptr<Packet> ProduceNotifyToSender(LookUpTableEntry tableentry);
	Ptr<Packet> ProduceAckToSender(LookUpTableEntry tableentry);
	Ptr<Packet> ProduceSameNotify(LookUpTableEntry tableentry);
	Ptr<Packet> ProduceNAckToSender(LookUpTableEntry tableentry);
	Ptr<Packet> ProduceSameNAck(LookUpTableEntry tableentry);
	Ptr<Packet> ProduceLastdata(LookUpTableEntry tableentry);
	Ptr<Packet> ProduceOOOdata(LookUpTableEntry tableentry);
	void SavePHeader(Ptr<Packet>p, CustomHeader &ch, uint16_t pidx, uint16_t sequence);
	Ptr<Packet> ReplicateDataPacket(Ptr<Packet> p, CustomHeader &ch);
//in-network retransmit
	void UpdateReTxReqs(uint32_t m_index, uint16_t left, uint16_t right);
	Ptr<Packet> CopyPacket(Ptr<Packet> p, CustomHeader &ch, uint16_t pidx1, uint16_t sequence1);
	void EnCopyQueue(Ptr<Packet> p, uint16_t port);
	void CopyDequeue(uint16_t port);
	bool CopyRecirculateTransmitStart(Ptr<Packet> p, uint16_t port, bool a);
	void CopyRecirculateTransmitComplete(uint16_t port);

	void RecirculateReceive(Ptr<Packet> p, uint16_t port);
	void EnReorderingQueue(Ptr<Packet> p, uint16_t port);
	void ReorderingDequeue(uint16_t port);
	bool ReorderingRecirculateTransmitStart(Ptr<Packet> p, uint16_t port);
	void ReorderingRecirculateTransmitComplete(uint16_t port);

	int CheckInOrderRecoverySeq(uint16_t p_seq, uint16_t ackNo);
	int CopyCheck(uint16_t p_seq, uint16_t upackNo, uint16_t port);
	int ReorderingCheck(uint16_t p_seq, uint16_t downackNo);
	bool SeqNoInReTxReq(uint16_t seqno, uint16_t port);
	void SetQueue (Ptr<Queue> queue);
	Ptr<Queue> GetQueue (void) const;
public:
	Ptr<SwitchMmu> m_mmu;
//config main
	uint16_t m_lossrate;
	void ConfigLossRate(uint16_t rate);
	uint16_t m_lookuptablesize;
	void ConfigLookupTableSize(uint16_t tablesize);
	uint32_t m_filtersize;
	void ConfigFilterSize(uint32_t filtersize);
	uint16_t m_filtertime;
	void ConfigFilterTime(uint16_t filtertime);
	uint16_t m_onlylastack;
	void ConfigOnlyLastAck(uint16_t onlylastack);
	uint32_t m_filterFBsize;
	void ConfigFilterFBSize(uint32_t filterFBsize);
	uint16_t m_roundseqsize;
	void ConfigRoundSeqSize(uint16_t roundseqsize);
	static uint16_t GetLookUpTableEntrySize();
	static uint16_t GetFilterValueEntrySize();

	static TypeId GetTypeId (void);
	SwitchNode();

	void SetEcmpSeed(uint32_t seed);
	void AddTableEntry(Ipv4Address &dstAddr, uint32_t intf_idx);
	void ClearTable();
	bool SwitchReceiveFromDevice(Ptr<NetDevice> device, Ptr<Packet> packet, CustomHeader &ch);
	void SwitchNotifyDequeue(uint32_t ifIndex, uint32_t qIndex, Ptr<Packet> p);
	uint16_t GeSeqnumber(uint16_t PortIndex);
	uint16_t GetPendingack(uint16_t PortIndex);

	// for approximate calc in PINT
	int logres_shift(int b, int l);
	int log2apprx(int x, int b, int m, int l); // given x of at most b bits, use most significant m bits of x, calc the result in l bits
	void AddHeader (Ptr<Packet> p, uint16_t protocolNumber);
	static uint16_t EtherToPpp (uint16_t protocol);

	void AddExtraHeader (Ptr<Packet> p, uint16_t sequence, uint16_t latestRxSeqNo);
 	void ProcessExtraHeader (Ptr<Packet> p, uint16_t& sequence_param, uint16_t& latestRxSeqNo_param);
};

} /* namespace ns3 */

#endif /* SWITCH_NODE_H */
