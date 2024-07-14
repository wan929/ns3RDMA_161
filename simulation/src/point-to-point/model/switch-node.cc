#include "ns3/ipv4.h"
#include "ns3/packet.h"
#include "ns3/ipv4-header.h"
#include "ns3/pause-header.h"
#include "ns3/flow-id-tag.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/double.h"
#include "switch-node.h"
#include "qbb-net-device.h"
#include "ppp-header.h"
#include "ns3/int-header.h"
#include "ns3/mine-udp-header.h"
#include "ns3/notify-header.h"
#include <cmath>
#include "ns3/random-variable.h"
#include <stdio.h>
#include <iomanip>
#include <ns3/seq-ts-header.h>
#include "ns3/empty-header.h"
#include "qbb-header.h"
#include "ns3/seq-port-header.h"
#include "ns3/extra-header.h"
#include <unordered_map>
#include <iostream>
#include <fstream>
namespace ns3 {

TypeId SwitchNode::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SwitchNode")
    .SetParent<Node> ()
    .AddConstructor<SwitchNode> ()
	.AddAttribute("EcnEnabled",
			"Enable ECN marking.",
			BooleanValue(false),
			MakeBooleanAccessor(&SwitchNode::m_ecnEnabled),
			MakeBooleanChecker())
	.AddAttribute("CcMode",
			"CC mode.",
			UintegerValue(0),
			MakeUintegerAccessor(&SwitchNode::m_ccMode),
			MakeUintegerChecker<uint32_t>())
	.AddAttribute("RecMode",
			"Rec mode.",
			UintegerValue(0),
			MakeUintegerAccessor(&SwitchNode::m_RecMode),
			MakeUintegerChecker<uint32_t>())
	.AddAttribute("AckHighPrio",
			"Set high priority for ACK/NACK or not",
			UintegerValue(0),
			MakeUintegerAccessor(&SwitchNode::m_ackHighPrio),
			MakeUintegerChecker<uint32_t>())
	.AddAttribute("MaxRtt",
			"Max Rtt of the network",
			UintegerValue(9000),
			MakeUintegerAccessor(&SwitchNode::m_maxRtt),
			MakeUintegerChecker<uint32_t>())
///*
	.AddAttribute ("CopyQueue", 
            "A queue to use as the copy queue in the node.",
            PointerValue (),
            MakePointerAccessor (&SwitchNode::m_copyqueue),
            MakePointerChecker<Queue> ())
//*/
  ;
  return tid;
}

SwitchNode::SwitchNode(){
	m_ecmpSeed = m_id;
	m_node_type = 1;
	// m_CopyrecirculatetxMachineState = Copy_READY;
	// m_ReorderingrecirculatetxMachineState = Reordering_READY;
	m_lossrate = 0;
	m_lookuptablesize = 1000;
	m_filtersize = 400000;
	m_filtertime = 15;
	m_onlylastack = 0;
	m_filterFBsize = 10000;
	m_roundseqsize = 3;

    lost_seq_udp = 0;
	lost_seq_ack = 0;
	lost_seq_nack = 0;

  	for(uint16_t i = 0; i < 257; i++)
  	{
    	m_counter[i] = 0;
    	m_seqnumber[i] = 0;
    	m_uplatestRxSeqNo[i] = 0;
		m_downlatestRxSeqNo[i] = 0;
		m_downPendingAck[i] = 0;
		start_state[i] = 0;
		m_ackNo[i] = 0;
		m_curr_state[i] = 0; //0 resume 1 pause
		m_copysize[i] = 0;
		m_reorderingsize[i] = 0;

		m_CopyrecirculatetxMachineState[i] = Copy_READY;
		m_ReorderingrecirculatetxMachineState[i] = Reordering_READY;
  	}

 	m_copyqueue = CreateObject<DropTailQueue>();
	for (uint32_t i = 0; i < pCnt; i++)
	{
		m_copyqueues.push_back(CreateObject<DropTailQueue>());
		m_reorderingqueues.push_back(CreateObject<DropTailQueue>());
	}
	m_reTxReqs.reserve(257);
	DataFilter.reserve(400000); //10000
	NACKFilter.reserve(400000); //10000
	lookuptable.reserve(257); //256 port
	for (uint32_t i = 0; i < 257; i++)
	{
		lookuptable[i].resize(5000);
	}

    m_mmu = CreateObject<SwitchMmu>();
	for (uint32_t i = 0; i < pCnt; i++)
		for (uint32_t j = 0; j < pCnt; j++)
			for (uint32_t k = 0; k < qCnt; k++)
				m_bytes[i][j][k] = 0;
	for (uint32_t i = 0; i < pCnt; i++)
		m_txBytes[i] = 0;
	for (uint32_t i = 0; i < pCnt; i++)
		m_lastPktSize[i] = m_lastPktTs[i] = 0;
	for (uint32_t i = 0; i < pCnt; i++)
		m_u[i] = 0;
}

bool SwitchNode::ToIsSwitch(uint32_t pidx){
	Ptr<NetDevice> device = m_devices[pidx];
	if(device->GetChannel()->GetDevice(0) != device){
		Ptr<NetDevice> tmp = device->GetChannel()->GetDevice(0);
		if(tmp->GetNode()->GetNodeType() == 1){
			return true;
		}
	}
	else if(device->GetChannel()->GetDevice(1) != device){
		Ptr<NetDevice> tmp = device->GetChannel()->GetDevice (1);
		if(tmp->GetNode()->GetNodeType() == 1){
			return true;
		}
	}
	return false;
}

bool SwitchNode::IsTorSwitch() //320--339
{
	if(GetId() >= 320 && GetId() <= 339)
	//if(GetId() == 1 || GetId() == 5)
	{
		return true;
	}
	return false;
}

int SwitchNode::FilterNACKPacket(FiveTuple fivetuple, uint16_t p_round, uint32_t p_seq){
	uint32_t index = MurmurHash3(&fivetuple, 13, m_ecmpSeed) % 400000;
	//std::cout << "index " << index << std::endl;
	bool a = IsEqualFlowID(fivetuple, NACKFilter[index].filter_flowid);
	bool b = IsNullFlowID(NACKFilter[index].filter_flowid);
	if(!a){
		if((b)){ //empty entry
			NACKFilter[index].filter_flowid.sport = fivetuple.sport;
			NACKFilter[index].filter_flowid.dport = fivetuple.dport;
			NACKFilter[index].filter_flowid.sip = fivetuple.sip;
			NACKFilter[index].filter_flowid.dip = fivetuple.dip;
			NACKFilter[index].filter_flowid.protocol = fivetuple.protocol;
			NACKFilter[index].filter_seq = p_seq;
			NACKFilter[index].filter_time = Simulator::Now().GetTimeStep();
			NACKFilter[index].filter_round = p_round + 1; //new round
			NACKFilter[index].vaild = 0;
			return 0; //放行
		}else if((!b && Simulator::Now().GetTimeStep() - NACKFilter[index].filter_time > (m_filtertime * 1000))){ //old entry
			std::cout << "old entry " << std::dec << GetId() << " ";
			std::cout << std::hex << "0" << NACKFilter[index].filter_flowid.sip << " 0" << NACKFilter[index].filter_flowid.dip << " " ; 
			std::cout << std::dec << NACKFilter[index].filter_flowid.sport << " " << NACKFilter[index].filter_flowid.dport << " " << NACKFilter[index].filter_seq << " " << NACKFilter[index].filter_round << std::endl;

			NACKFilter[index].filter_flowid.sport = fivetuple.sport;
			NACKFilter[index].filter_flowid.dport = fivetuple.dport;
			NACKFilter[index].filter_flowid.sip = fivetuple.sip;
			NACKFilter[index].filter_flowid.dip = fivetuple.dip;
			NACKFilter[index].filter_flowid.protocol = fivetuple.protocol;
			NACKFilter[index].filter_seq = p_seq;
			NACKFilter[index].filter_time = Simulator::Now().GetTimeStep();
			NACKFilter[index].filter_round = p_round + 1; //new round
			NACKFilter[index].vaild = 0;

			std::cout << "new entry " << std::dec << GetId() << " ";
			std::cout << std::hex << "0" << NACKFilter[index].filter_flowid.sip << " 0" << NACKFilter[index].filter_flowid.dip << " " ; 
			std::cout << std::dec << NACKFilter[index].filter_flowid.sport << " " << NACKFilter[index].filter_flowid.dport << " " << NACKFilter[index].filter_seq << " " << NACKFilter[index].filter_round << std::endl;

			return 7; //放行
		}
		else{
			std::cout << "NACK hash conflict " << std::dec << GetId() << " ";
			std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ; 
			std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << std::endl;

			NACK_hash_conflict_number++;
			return 1; //哈希冲突，且不进行更新 //放行
		}
	}else{
		if(p_round < NACKFilter[index].filter_round){
			if(p_seq >= NACKFilter[index].filter_seq){
				return 2; //过滤
			}else{
				NACKFilter[index].filter_seq = p_seq;
				NACKFilter[index].filter_time = Simulator::Now().GetTimeStep();
				NACKFilter[index].filter_round++; //new round
				NACKFilter[index].vaild = 0;
				return 3; //放行
			}
		}else{
			if(p_seq >= NACKFilter[index].filter_seq){
				NACKFilter[index].filter_seq = p_seq;
				NACKFilter[index].filter_time = Simulator::Now().GetTimeStep();
				NACKFilter[index].filter_round++; //new round
				NACKFilter[index].vaild = 0;
				return 4; //放行
			}else{
				return 5; //不会出现
			}
		}
	}
	return 6;
}

void SwitchNode::RecordNACKEseq(CustomHeader &ch){
	FiveTuple fivetuple_nack;
	fivetuple_nack.sport = ch.ack.dport;
	fivetuple_nack.dport = ch.ack.sport;
	fivetuple_nack.sip = ch.dip;
	fivetuple_nack.dip = ch.sip;
	fivetuple_nack.protocol = 0x11;
	uint32_t index_nack = MurmurHash3(&fivetuple_nack, 13, m_ecmpSeed) % 400000;
	bool a = IsEqualFlowID(fivetuple_nack, NACKFilter[index_nack].filter_flowid);
	bool b = IsNullFlowID(NACKFilter[index_nack].filter_flowid);
	if(!a){
		if((b)){ //empty entry
			NACKFilter[index_nack].filter_flowid.sport = ch.ack.dport;
			NACKFilter[index_nack].filter_flowid.dport = ch.ack.sport;
			NACKFilter[index_nack].filter_flowid.sip = ch.dip;
			NACKFilter[index_nack].filter_flowid.dip = ch.sip;
			NACKFilter[index_nack].filter_flowid.protocol = 0x11;
			NACKFilter[index_nack].filter_seq = ch.ack.seq;
		}else{
			//hash conflict;
		}
	}else{
		//no proability;
	}
}

bool SwitchNode::IsNACKEseq(CustomHeader &ch){
	FiveTuple fivetuple_data;
	fivetuple_data.sport = ch.udp.sport;
	fivetuple_data.dport = ch.udp.dport;
	fivetuple_data.sip = ch.sip;
	fivetuple_data.dip = ch.dip;
	fivetuple_data.protocol = 0x11;
	uint32_t index_data = MurmurHash3(&fivetuple_data, 13, m_ecmpSeed) % 400000;
	bool a = IsEqualFlowID(fivetuple_data, NACKFilter[index_data].filter_flowid);
	bool b = IsNullFlowID(NACKFilter[index_data].filter_flowid);
	if(a){
		if(ch.udp.seq == NACKFilter[index_data].filter_seq){
			NACKFilter[index_data].filter_flowid.sport = 0;
			NACKFilter[index_data].filter_flowid.dport = 0;
			NACKFilter[index_data].filter_flowid.sip = 0;
			NACKFilter[index_data].filter_flowid.dip = 0;
			NACKFilter[index_data].filter_flowid.protocol = 0;
			NACKFilter[index_data].filter_seq = 0;
			return true;
		}
	}
	return false;
}

void SwitchNode::SetFlowIDNull(FiveTuple id)
{
	id.sport = 0;
	id.dport = 0;
	id.sip = 0;
	id.dip = 0;
	id.protocol = 0;
	return;
}

bool SwitchNode::IsNullFlowID(FiveTuple id)
{
	if(id.sport == 0 && id.dport == 0 && id.sip == 0 && id.dip == 0 && id.protocol == 0) //entry is empty
	{
		return true;
	}
	return false;
}

bool SwitchNode::IsEqualFlowID(FiveTuple id1, FiveTuple id2)
{

	if(id1.sport == id2.sport && id1.dport == id2.dport && id1.sip == id2.sip && id1.dip == id2.dip && id1.protocol == id2.protocol) //same flow
	{
		return true;
	}
	else{
		return false;
	}
}
//0b009601 0b004d01 10000 100 
//0b00bc01 0b002201 10000 100
int SwitchNode::FilterPacket_SADRCV(FiveTuple fivetuple, uint32_t p_seq, uint16_t p_round, bool isloss)
{
	uint32_t index = MurmurHash3(&fivetuple, 13, m_ecmpSeed) % m_filtersize;
	bool a = IsEqualFlowID(fivetuple, DataFilter[index].filter_flowid);
	bool b = IsNullFlowID(DataFilter[index].filter_flowid);
	if(isloss)
	{
		if(!a)
		{
			if((b) || (!b && Simulator::Now().GetTimeStep() - DataFilter[index].filter_time > (m_filtertime * 1000))) //15us
			{
				DataFilter[index].filter_flowid.sport = fivetuple.sport;
				DataFilter[index].filter_flowid.dport = fivetuple.dport;
				DataFilter[index].filter_flowid.sip = fivetuple.sip;
				DataFilter[index].filter_flowid.dip = fivetuple.dip;
				DataFilter[index].filter_flowid.protocol = fivetuple.protocol;
				DataFilter[index].filter_seq = p_seq;
				DataFilter[index].filter_time = Simulator::Now().GetTimeStep();
				DataFilter[index].filter_round = p_round;

				// if((fivetuple.sip == 0x0b009601 && fivetuple.dip == 0x0b004d01 && fivetuple.sport == 10000 && fivetuple.dport == 100) ||
				// 	(fivetuple.sip == 0x0b00bc01 && fivetuple.dip == 0x0b002201 && fivetuple.sport == 10000 && fivetuple.dport == 100))
				// {
				// 	std::cout << "新的丢掉的包序号更新 " << std::dec << GetId() << " " ;
				// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
				// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
				// }

				return 0; //新的丢掉的包序号更新
			}
			else{
				// if((fivetuple.sip == 0x0b009601 && fivetuple.dip == 0x0b004d01 && fivetuple.sport == 10000 && fivetuple.dport == 100) ||
				// 	(fivetuple.sip == 0x0b00bc01 && fivetuple.dip == 0x0b002201 && fivetuple.sport == 10000 && fivetuple.dport == 100))
				// {
				// 	std::cout << "哈希冲突且不进行更新 " << std::dec << GetId() << " " ;
				// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
				// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << std::endl;
				// }
				data_hash_conflict_number++;
				return 1; //哈希冲突，且不进行更新
			}
		}
		else{
			if(p_seq <= DataFilter[index].filter_seq)
			{
				// if((fivetuple.sip == 0x0b009601 && fivetuple.dip == 0x0b004d01 && fivetuple.sport == 10000 && fivetuple.dport == 100) ||
				// 	(fivetuple.sip == 0x0b00bc01 && fivetuple.dip == 0x0b002201 && fivetuple.sport == 10000 && fivetuple.dport == 100))
				// {
				// 	std::cout << "小于等于又丢了第二遍更新时间 " << std::dec << GetId() << " " ;
				// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
				// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
				// }
				DataFilter[index].filter_seq = p_seq;
				DataFilter[index].filter_time = Simulator::Now().GetTimeStep();
				DataFilter[index].filter_round = p_round;
				return 2; //又丢了第二遍，更新时间
			}
			else if(p_seq > DataFilter[index].filter_seq)
			{
				// if((fivetuple.sip == 0x0b009601 && fivetuple.dip == 0x0b004d01 && fivetuple.sport == 10000 && fivetuple.dport == 100) ||
				// 	(fivetuple.sip == 0x0b00bc01 && fivetuple.dip == 0x0b002201 && fivetuple.sport == 10000 && fivetuple.dport == 100))
				// {
				// 	std::cout << "比expectedseq大的包丢了不管 " << std::dec << GetId() << " " ;
				// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
				// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
				// }
				return 3; //比expectedseq大的包丢了，不管
			}
		}
	}
	else if(!isloss)
	{
		if(!a)
		{
			// if((fivetuple.sip == 0x0b009601 && fivetuple.dip == 0x0b004d01 && fivetuple.sport == 10000 && fivetuple.dport == 100) ||
			// 		(fivetuple.sip == 0x0b00bc01 && fivetuple.dip == 0x0b002201 && fivetuple.sport == 10000 && fivetuple.dport == 100))
			// {
			// 	std::cout << "没有丢包的流该包直接进入交换机buffer " << std::dec << GetId() << " " ;
			// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
			// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << DataFilter[index].filter_seq << std::endl;
			// }
			return 4; //没有丢包的流，该包直接进入交换机buffer
		}
		else{
			//new no round, useless..........................
			// if(Simulator::Now().GetTimeStep() - DataFilter[index].filter_time > 800000) //new
			// {
			// 	std::cout << "same entry time out " << std::dec << GetId() << " " ;
			// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
			// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;

			// 	DataFilter[index].filter_flowid.sport = 0;
			// 	DataFilter[index].filter_flowid.dport = 0;
			// 	DataFilter[index].filter_flowid.sip = 0;
			// 	DataFilter[index].filter_flowid.dip = 0;
			// 	DataFilter[index].filter_flowid.protocol = 0;
			// 	DataFilter[index].filter_seq = 0;
			// 	DataFilter[index].filter_time = 0;
			// 	DataFilter[index].filter_round = 0;
			// 	return 11;
			// }

			// if(p_seq == DataFilter[index].filter_seq){
			// 	if((fivetuple.sip == 0x0b009601 && fivetuple.dip == 0x0b004d01 && fivetuple.sport == 10000 && fivetuple.dport == 100) ||
			// 		(fivetuple.sip == 0x0b00bc01 && fivetuple.dip == 0x0b002201 && fivetuple.sport == 10000 && fivetuple.dport == 100))
			// 	{
			// 		std::cout << "期望的序号entry全部置为0 " << std::dec << GetId() << " " ;
			// 		std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
			// 		std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
			// 	}
			// 	DataFilter[index].filter_flowid.sport = 0;
			// 	DataFilter[index].filter_flowid.dport = 0;
			// 	DataFilter[index].filter_flowid.sip = 0;
			// 	DataFilter[index].filter_flowid.dip = 0;
			// 	DataFilter[index].filter_flowid.protocol = 0;
			// 	DataFilter[index].filter_seq = 0;
			// 	DataFilter[index].filter_time = 0;
			// 	DataFilter[index].filter_round = 0;
			// 	return 5; //expected seq，该entry全部置为0，该包直接进入交换机buffer
			// }else if(p_seq < DataFilter[index].filter_seq){
			// 	if((fivetuple.sip == 0x0b009601 && fivetuple.dip == 0x0b004d01 && fivetuple.sport == 10000 && fivetuple.dport == 100) ||
			// 		(fivetuple.sip == 0x0b00bc01 && fivetuple.dip == 0x0b002201 && fivetuple.sport == 10000 && fivetuple.dport == 100))
			// 	{
			// 		std::cout << "更小的序号 " << std::dec << GetId() << " " ;
			// 		std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
			// 		std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
			// 	}
			// 	return 7; //对应rdma7.30的第一种情况，该entry全部置为0，进入buffer
			// }else if(p_seq > DataFilter[index].filter_seq)
			// {
			// 	if((fivetuple.sip == 0x0b009601 && fivetuple.dip == 0x0b004d01 && fivetuple.sport == 10000 && fivetuple.dport == 100) ||
			// 		(fivetuple.sip == 0x0b00bc01 && fivetuple.dip == 0x0b002201 && fivetuple.sport == 10000 && fivetuple.dport == 100))
			// 	{
			// 		std::cout << "不期望的序号过滤 " << std::dec << GetId() << " " ;
			// 		std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
			// 		std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
			// 	}
			// 	return 6; //unexpected seq, filter and drop
			// }

			if(p_round > DataFilter[index].filter_round)
			{
				if(p_seq == DataFilter[index].filter_seq)
				{
					DataFilter[index].filter_flowid.sport = 0;
					DataFilter[index].filter_flowid.dport = 0;
					DataFilter[index].filter_flowid.sip = 0;
					DataFilter[index].filter_flowid.dip = 0;
					DataFilter[index].filter_flowid.protocol = 0;
					DataFilter[index].filter_seq = 0;
					DataFilter[index].filter_time = 0;
					DataFilter[index].filter_round = 0;
					return 5; //expected seq，该entry全部置为0，该包直接进入交换机buffer
				}
				else if(p_seq < DataFilter[index].filter_seq)
				{
					DataFilter[index].filter_flowid.sport = 0;
					DataFilter[index].filter_flowid.dport = 0;
					DataFilter[index].filter_flowid.sip = 0;
					DataFilter[index].filter_flowid.dip = 0;
					DataFilter[index].filter_flowid.protocol = 0;
					DataFilter[index].filter_seq = 0;
					DataFilter[index].filter_time = 0;
					DataFilter[index].filter_round = 0;
					return 7; //对应rdma7.30的第一种情况，进入buffer
				}
				else if(p_seq > DataFilter[index].filter_seq)
				{
					DataFilter[index].filter_flowid.sport = 0;
					DataFilter[index].filter_flowid.dport = 0;
					DataFilter[index].filter_flowid.sip = 0;
					DataFilter[index].filter_flowid.dip = 0;
					DataFilter[index].filter_flowid.protocol = 0;
					DataFilter[index].filter_seq = 0;
					DataFilter[index].filter_time = 0;
					DataFilter[index].filter_round = 0;
					return 10; //unexpected seq, filter and drop
				}
			}
			else
			{
				if(p_seq > DataFilter[index].filter_seq)
				{
					return 6; //unexpected seq, filter and drop
				}
			}
		}
	}
	return 8;
}
//0b006101 0b007201 10000 100
int SwitchNode::FilterPacket_SADRSV(FiveTuple fivetuple, uint32_t p_seq, uint16_t p_round, bool isloss){
	uint32_t index = MurmurHash3(&fivetuple, 13, m_ecmpSeed) % m_filtersize;
	bool a = IsEqualFlowID(fivetuple, DataFilter[index].filter_flowid);
	bool b = IsNullFlowID(DataFilter[index].filter_flowid);
	if(isloss)
	{
		if(!a){
			if((b) || (!b && Simulator::Now().GetTimeStep() - DataFilter[index].filter_time > (m_filtertime * 1000))){ //15us	
				DataFilter[index].filter_flowid.sport = fivetuple.sport;
				DataFilter[index].filter_flowid.dport = fivetuple.dport;
				DataFilter[index].filter_flowid.sip = fivetuple.sip;
				DataFilter[index].filter_flowid.dip = fivetuple.dip;
				DataFilter[index].filter_flowid.protocol = fivetuple.protocol;
				DataFilter[index].filter_seq = p_seq;
				DataFilter[index].filter_time = Simulator::Now().GetTimeStep();
				DataFilter[index].filter_round = p_round;
				// if(fivetuple.sip == 0x0b006101 && fivetuple.dip == 0x0b007201 && fivetuple.sport == 10000 && fivetuple.dport == 100)
				// {
				// 	std::cout << "新的丢掉的包序号更新 " << std::dec << GetId() << " " ;
				// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
				// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
				// }
				return 0; //新的丢掉的包序号更新
			}
			else{
				// if(fivetuple.sip == 0x0b005201 && fivetuple.dip == 0x0b011d01 && fivetuple.sport == 10000 && fivetuple.dport == 100)
				// {
				// 	std::cout << "哈希冲突且不进行更新 " << std::dec << GetId() << " " ;
				// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
				// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << std::endl;
				// }
				data_hash_conflict_number++;
				return 1; //哈希冲突，且不进行更新
			}
		}
		else{
			if(p_seq <= DataFilter[index].filter_seq){
				// if(fivetuple.sip == 0x0b006101 && fivetuple.dip == 0x0b007201 && fivetuple.sport == 10000 && fivetuple.dport == 100)
				// {
				// 	std::cout << "小于等于又丢了第二遍更新时间 " << std::dec << GetId() << " " ;
				// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
				// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
				// }
				DataFilter[index].filter_seq = p_seq;
				DataFilter[index].filter_time = Simulator::Now().GetTimeStep();
				DataFilter[index].filter_round = p_round;
				return 2; //又丢了第二遍，更新时间
			}
			else if(p_seq > DataFilter[index].filter_seq){
				// if(fivetuple.sip == 0x0b006101 && fivetuple.dip == 0x0b007201 && fivetuple.sport == 10000 && fivetuple.dport == 100)
				// {
				// 	std::cout << "比expectedseq大的包丢了不管 " << std::dec << GetId() << " " ;
				// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
				// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
				// }
				return 3; //比expectedseq大的包丢了，不管
			}
		}
	}
	else if(!isloss){
		if(!a){
			// if(fivetuple.sip == 0x0b005201 && fivetuple.dip == 0x0b011d01 && fivetuple.sport == 10000 && fivetuple.dport == 100)
			// {
			// 	// std::cout << "没有丢包的流该包直接进入交换机buffer " << std::dec << GetId() << " " ;
			// 	// std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
			// 	// std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << DataFilter[index].filter_seq << std::endl;
			// }
			return 4; //没有丢包的流，该包直接进入交换机buffer
		}
		else{
			if(Simulator::Now().GetTimeStep() - DataFilter[index].filter_time > 800000) //new
			{
				std::cout << "same entry time out " << std::dec << GetId() << " " ;
				std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
				std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;

				DataFilter[index].filter_flowid.sport = 0;
				DataFilter[index].filter_flowid.dport = 0;
				DataFilter[index].filter_flowid.sip = 0;
				DataFilter[index].filter_flowid.dip = 0;
				DataFilter[index].filter_flowid.protocol = 0;
				DataFilter[index].filter_seq = 0;
				DataFilter[index].filter_time = 0;
				DataFilter[index].filter_round = 0;
				return 11;
			}

			//if(p_round > DataFilter[index].filter_round){
				if(p_seq == DataFilter[index].filter_seq){
					// if(fivetuple.sip == 0x0b006101 && fivetuple.dip == 0x0b007201 && fivetuple.sport == 10000 && fivetuple.dport == 100)
					// {
					// 	std::cout << "期望的序号entry全部置为0 " << std::dec << GetId() << " " ;
					// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
					// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
					// }
					DataFilter[index].filter_flowid.sport = 0;
					DataFilter[index].filter_flowid.dport = 0;
					DataFilter[index].filter_flowid.sip = 0;
					DataFilter[index].filter_flowid.dip = 0;
					DataFilter[index].filter_flowid.protocol = 0;
					DataFilter[index].filter_seq = 0;
					DataFilter[index].filter_time = 0;
					DataFilter[index].filter_round = 0;
					return 5; //expected seq，该entry全部置为0，该包直接进入交换机buffer
				}else if(p_seq < DataFilter[index].filter_seq){
					// if(fivetuple.sip == 0x0b006101 && fivetuple.dip == 0x0b007201 && fivetuple.sport == 10000 && fivetuple.dport == 100)
					// {
					// 	std::cout << "更小的序号 " << std::dec << GetId() << " " ;
					// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
					// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
					// }
					// DataFilter[index].filter_flowid.sport = 0;
					// DataFilter[index].filter_flowid.dport = 0;
					// DataFilter[index].filter_flowid.sip = 0;
					// DataFilter[index].filter_flowid.dip = 0;
					// DataFilter[index].filter_flowid.protocol = 0;
					// DataFilter[index].filter_seq = 0;
					// DataFilter[index].filter_time = 0;
					// DataFilter[index].filter_round = 0;
					return 7; //对应rdma7.30的第一种情况，该entry全部置为0，进入buffer
				}else if(p_seq > DataFilter[index].filter_seq)
				{
					// if(fivetuple.sip == 0x0b006101 && fivetuple.dip == 0x0b007201 && fivetuple.sport == 10000 && fivetuple.dport == 100)
					// 	{
					// 		std::cout << "不期望的序号过滤 " << std::dec << GetId() << " " ;
					// 		std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
					// 		std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
					// 	}
					if(p_seq == DataFilter[index].filter_seq + 1000){ //subsequence
				   		//0b00e401 0b006e01 10000 100
						// if(fivetuple.sip == 0x0b005201 && fivetuple.dip == 0x0b011d01 && fivetuple.sport == 10000 && fivetuple.dport == 100)
						// {
						// 	std::cout << "subsequence " << std::dec << GetId() << " " ;
						// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
						// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << DataFilter[index].filter_seq << std::endl;
						// }
						return 9;
					}else{
						// if(fivetuple.sip == 0x0b005201 && fivetuple.dip == 0x0b011d01 && fivetuple.sport == 10000 && fivetuple.dport == 100)
						// {
						// 	// std::cout << "不期望的序号过滤 " << std::dec << GetId() << " " ;
						// 	// std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
						// 	// std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << DataFilter[index].filter_seq << std::endl;
						// }
						return 6; //unexpected seq, filter and drop
					}
				}
			// }else{
			// 	if(p_seq > DataFilter[index].filter_seq){
			// 		if(p_seq == DataFilter[index].filter_seq + 1000){ //subsequence
			// 	   		//0b00e401 0b006e01 10000 100
			// 			// if(fivetuple.sip == 0x0b005201 && fivetuple.dip == 0x0b011d01 && fivetuple.sport == 10000 && fivetuple.dport == 100)
			// 			// {
			// 			// 	std::cout << "subsequence " << std::dec << GetId() << " " ;
			// 			// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
			// 			// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << DataFilter[index].filter_seq << std::endl;
			// 			// }
			// 			return 9;
			// 		}else{
			// 			// if(fivetuple.sip == 0x0b005201 && fivetuple.dip == 0x0b011d01 && fivetuple.sport == 10000 && fivetuple.dport == 100)
			// 			// {
			// 			// 	// std::cout << "不期望的序号过滤 " << std::dec << GetId() << " " ;
			// 			// 	// std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
			// 			// 	// std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << DataFilter[index].filter_seq << std::endl;
			// 			// }
			// 			return 6; //unexpected seq, filter and drop
			// 		}
			// 	}
			// }
		}
	}
	//0b005201 0b011d01 10000 100
	// if(fivetuple.sip == 0x0b005201 && fivetuple.dip == 0x0b011d01 && fivetuple.sport == 10000 && fivetuple.dport == 100)
	// {
	// 	std::cout << "其他情况 " << std::dec << GetId() << " " ;
	// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
	// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << std::endl;
	// }
	return 8; //..
}

int SwitchNode::FilterPacket_SADRSLR(FiveTuple fivetuple, uint32_t p_seq, uint16_t p_round, bool isloss)// SADR_SLR
{
	uint32_t index = MurmurHash3(&fivetuple, 13, m_ecmpSeed) % m_filtersize;
	bool a = IsEqualFlowID(fivetuple, DataFilter[index].filter_flowid);
	bool b = IsNullFlowID(DataFilter[index].filter_flowid);
	if(isloss)
	{
		if(!a){
			if((b) || (!b && Simulator::Now().GetTimeStep() - DataFilter[index].filter_time > (m_filtertime * 1000))){ //15us	
				DataFilter[index].filter_flowid.sport = fivetuple.sport;
				DataFilter[index].filter_flowid.dport = fivetuple.dport;
				DataFilter[index].filter_flowid.sip = fivetuple.sip;
				DataFilter[index].filter_flowid.dip = fivetuple.dip;
				DataFilter[index].filter_flowid.protocol = fivetuple.protocol;
				DataFilter[index].filter_seq = p_seq;
				DataFilter[index].filter_time = Simulator::Now().GetTimeStep();
				DataFilter[index].filter_round = p_round;
				// if(fivetuple.sip == 0x0b006101 && fivetuple.dip == 0x0b007201 && fivetuple.sport == 10000 && fivetuple.dport == 100)
				// {
				// 	std::cout << "新的丢掉的包序号更新 " << std::dec << GetId() << " " ;
				// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
				// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
				// }
				return 0; //新的丢掉的包序号更新
			}
			else{
				// if(fivetuple.sip == 0x0b005201 && fivetuple.dip == 0x0b011d01 && fivetuple.sport == 10000 && fivetuple.dport == 100)
				// {
				// 	std::cout << "哈希冲突且不进行更新 " << std::dec << GetId() << " " ;
				// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
				// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << std::endl;
				// }
				data_hash_conflict_number++;
				return 1; //哈希冲突，且不进行更新
			}
		}
		else{
			if(p_seq <= DataFilter[index].filter_seq){
				// if(fivetuple.sip == 0x0b006101 && fivetuple.dip == 0x0b007201 && fivetuple.sport == 10000 && fivetuple.dport == 100)
				// {
				// 	std::cout << "小于等于又丢了第二遍更新时间 " << std::dec << GetId() << " " ;
				// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
				// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
				// }
				DataFilter[index].filter_seq = p_seq;
				DataFilter[index].filter_time = Simulator::Now().GetTimeStep();
				DataFilter[index].filter_round = p_round;
				return 2; //又丢了第二遍，更新时间
			}
			else if(p_seq > DataFilter[index].filter_seq){
				// if(fivetuple.sip == 0x0b006101 && fivetuple.dip == 0x0b007201 && fivetuple.sport == 10000 && fivetuple.dport == 100)
				// {
				// 	std::cout << "比expectedseq大的包丢了不管 " << std::dec << GetId() << " " ;
				// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
				// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
				// }
				return 3; //比expectedseq大的包丢了，不管
			}
		}
	}
	else if(!isloss){
		if(!a){
			// if(fivetuple.sip == 0x0b005201 && fivetuple.dip == 0x0b011d01 && fivetuple.sport == 10000 && fivetuple.dport == 100)
			// {
			// 	// std::cout << "没有丢包的流该包直接进入交换机buffer " << std::dec << GetId() << " " ;
			// 	// std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
			// 	// std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << DataFilter[index].filter_seq << std::endl;
			// }
			return 4; //没有丢包的流，该包直接进入交换机buffer
		}
		else{
			// if(Simulator::Now().GetTimeStep() - DataFilter[index].filter_time > 800000) //new
			// {
			// 	std::cout << "same entry time out " << std::dec << GetId() << " " ;
			// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
			// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;

			// 	DataFilter[index].filter_flowid.sport = 0;
			// 	DataFilter[index].filter_flowid.dport = 0;
			// 	DataFilter[index].filter_flowid.sip = 0;
			// 	DataFilter[index].filter_flowid.dip = 0;
			// 	DataFilter[index].filter_flowid.protocol = 0;
			// 	DataFilter[index].filter_seq = 0;
			// 	DataFilter[index].filter_time = 0;
			// 	DataFilter[index].filter_round = 0;
			// 	return 11;
			// }

			//if(p_round > DataFilter[index].filter_round){
				if(p_seq == DataFilter[index].filter_seq){
					// if(fivetuple.sip == 0x0b006101 && fivetuple.dip == 0x0b007201 && fivetuple.sport == 10000 && fivetuple.dport == 100)
					// {
					// 	std::cout << "期望的序号entry全部置为0 " << std::dec << GetId() << " " ;
					// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
					// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
					// }
					DataFilter[index].filter_flowid.sport = 0;
					DataFilter[index].filter_flowid.dport = 0;
					DataFilter[index].filter_flowid.sip = 0;
					DataFilter[index].filter_flowid.dip = 0;
					DataFilter[index].filter_flowid.protocol = 0;
					DataFilter[index].filter_seq = 0;
					DataFilter[index].filter_time = 0;
					DataFilter[index].filter_round = 0;
					return 5; //expected seq，该entry全部置为0，该包直接进入交换机buffer
				}else if(p_seq < DataFilter[index].filter_seq){
					// if(fivetuple.sip == 0x0b006101 && fivetuple.dip == 0x0b007201 && fivetuple.sport == 10000 && fivetuple.dport == 100)
					// {
					// 	std::cout << "更小的序号 " << std::dec << GetId() << " " ;
					// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
					// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << p_round << " " << DataFilter[index].filter_seq << " " << DataFilter[index].filter_round << std::endl;
					// }
					// DataFilter[index].filter_flowid.sport = 0;
					// DataFilter[index].filter_flowid.dport = 0;
					// DataFilter[index].filter_flowid.sip = 0;
					// DataFilter[index].filter_flowid.dip = 0;
					// DataFilter[index].filter_flowid.protocol = 0;
					// DataFilter[index].filter_seq = 0;
					// DataFilter[index].filter_time = 0;
					// DataFilter[index].filter_round = 0;
					return 7; //对应rdma7.30的第一种情况，该entry全部置为0，进入buffer
				}else if(p_seq > DataFilter[index].filter_seq)
				{
					// if(fivetuple.sip == 0x0b005201 && fivetuple.dip == 0x0b011d01 && fivetuple.sport == 10000 && fivetuple.dport == 100)
					// {
					// 	// std::cout << "不期望的序号过滤 " << std::dec << GetId() << " " ;
					// 	// std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
					// 	// std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << DataFilter[index].filter_seq << std::endl;
					// }
					return 6; //unexpected seq, filter and drop

				}
			// }else{
			// 	if(p_seq > DataFilter[index].filter_seq){
			// 		if(p_seq == DataFilter[index].filter_seq + 1000){ //subsequence
			// 	   		//0b00e401 0b006e01 10000 100
			// 			// if(fivetuple.sip == 0x0b005201 && fivetuple.dip == 0x0b011d01 && fivetuple.sport == 10000 && fivetuple.dport == 100)
			// 			// {
			// 			// 	std::cout << "subsequence " << std::dec << GetId() << " " ;
			// 			// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
			// 			// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << DataFilter[index].filter_seq << std::endl;
			// 			// }
			// 			return 9;
			// 		}else{
			// 			// if(fivetuple.sip == 0x0b005201 && fivetuple.dip == 0x0b011d01 && fivetuple.sport == 10000 && fivetuple.dport == 100)
			// 			// {
			// 			// 	// std::cout << "不期望的序号过滤 " << std::dec << GetId() << " " ;
			// 			// 	// std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
			// 			// 	// std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << " " << DataFilter[index].filter_seq << std::endl;
			// 			// }
			// 			return 6; //unexpected seq, filter and drop
			// 		}
			// 	}
			// }
		}
	}
	//0b005201 0b011d01 10000 100
	// if(fivetuple.sip == 0x0b005201 && fivetuple.dip == 0x0b011d01 && fivetuple.sport == 10000 && fivetuple.dport == 100)
	// {
	// 	std::cout << "其他情况 " << std::dec << GetId() << " " ;
	// 	std::cout << std::hex << "0" << fivetuple.sip << " 0" << fivetuple.dip << " " ;
	// 	std::cout << std::dec << fivetuple.sport << " " << fivetuple.dport << " " << p_seq << std::endl;
	// }
	return 8; //..
}

uint32_t SwitchNode::MurmurHash3(const void* key, int len, uint32_t seed) 
{
    // 将输入数据转换为字节数组
    const uint8_t* data = static_cast<const uint8_t*>(key);
    const int nblocks = len / 4;

    // 定义哈希值的初始值
    uint32_t h1 = seed;

    // 定义一些常数
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    // 处理4字节块
    const uint32_t* blocks = reinterpret_cast<const uint32_t*>(data + nblocks * 4);
    for (int i = -nblocks; i; i++) {
        uint32_t k1 = blocks[i];

        // 哈希计算
        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> 17);  // 左循环移位（ROTL32(k1, 15)）
        k1 *= c2;

        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> 19);  // 左循环移位（ROTL32(h1, 13)）
        h1 = h1 * 5 + 0xe6546b64;
    }

    // 处理剩余的字节
    const uint8_t* tail = data + nblocks * 4;
    uint32_t k1 = 0;

    switch (len & 3) {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0];
            	k1 *= c1;
            	k1 = (k1 << 15) | (k1 >> 17);  // 左循环移位（ROTL32(k1, 15)）
            	k1 *= c2;
            	h1 ^= k1;
    };

    // 最终处理
    h1 ^= len;
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
}

int SwitchNode::GetOutDev(Ptr<const Packet> p, CustomHeader &ch){
	// look up entries
	auto entry = m_rtTable.find(ch.dip);

	// no matching entry
	if (entry == m_rtTable.end())
		return -1;

	// entry found
	auto &nexthops = entry->second;

	// pick one next hop based on hash
	union {
		uint8_t u8[4+4+2+2];
		uint32_t u32[3];
	} buf;
	buf.u32[0] = ch.sip;
	buf.u32[1] = ch.dip;
	if (ch.l3Prot == 0x6)
		buf.u32[2] = ch.tcp.sport | ((uint32_t)ch.tcp.dport << 16);
	else if (ch.l3Prot == 0x11)
		buf.u32[2] = ch.udp.sport | ((uint32_t)ch.udp.dport << 16);
	else if (ch.l3Prot == 0xFC || ch.l3Prot == 0xFD)
		buf.u32[2] = ch.ack.sport | ((uint32_t)ch.ack.dport << 16);
	else if (ch.l3Prot == 0xFB)
		buf.u32[2] = ch.notify.sport | ((uint32_t)ch.notify.dport << 16);

	uint32_t idx = EcmpHash(buf.u8, 12, m_ecmpSeed) % nexthops.size();
	return nexthops[idx];
}

void SwitchNode::CheckAndSendPfc(uint32_t inDev, uint32_t qIndex){
	Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[inDev]);
	if (m_mmu->CheckShouldPause(inDev, qIndex)){
		pfc_times++;
		//std::cout << GetId() << " " << m_mmu->GetSharedUsed(inDev, qIndex) << " " << m_mmu->GetPfcThreshold(inDev) << std::endl;
		device->SendPfc(qIndex, 0);
		m_mmu->SetPause(inDev, qIndex);
	}
}
void SwitchNode::CheckAndSendResume(uint32_t inDev, uint32_t qIndex){
	Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[inDev]);
	if (m_mmu->CheckShouldResume(inDev, qIndex)){
		device->SendPfc(qIndex, 1);
		m_mmu->SetResume(inDev, qIndex);
	}
}

///*
//in-network notification
uint16_t SwitchNode::CheckAndSendNotifyToSender(uint32_t inDev, uint16_t sequence, Ptr<Packet> p, CustomHeader &ch)
{
	Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[inDev]);
	uint16_t counter_era = (m_counter[inDev] >> 15) & 0xff;
	uint16_t sequence_era = (sequence >> 15) & 0xff;
	uint16_t counter_value = m_counter[inDev] & 0x7fff;
	uint16_t sequence_value = sequence & 0x7fff;
	if(counter_era == sequence_era)
	{
		if(sequence_value == counter_value) //true receive
		{
			//std::cout << sequence << " " << m_counter[inDev] <<std::endl;
			//std::cout << "true receive " << this->count << " " << param << " " << next <<std::endl;
			m_counter[inDev]++;
			//std::cout << m_counter[inDev] << std::endl;
			return 1;
		}
		else if(sequence_value > counter_value) //just lost
		{
			//std::cout << Simulator::Now() << " " << "find lost " << GetId() << " " << inDev << " " << sequence << " " << m_counter[inDev] << std::endl;
			device->SendNotifyToUp(m_counter[inDev], sequence);
			m_counter[inDev] = sequence;
			m_counter[inDev]++;
			return 2;
		}
		else if(sequence_value < counter_value && sequence_value != 0)
		{
			std::cout << "sequence < m_counter[inDev] && sequence != 0 " << sequence_value << " " << counter_value << std::endl;
			return 3;
		}
	}
	
	else if(counter_era != sequence_era) //just lost different era
	{
		if(sequence_value < counter_value)
		{
			//std::cout << Simulator::Now() << " " << "find lost " << GetId() << " " << inDev << " " << sequence << " " << m_counter[inDev] << std::endl;
			device->SendNotifyToUp(m_counter[inDev], sequence);
			m_counter[inDev] = sequence;
			m_counter[inDev]++;
			return 4;
		}
		else
		{
			std::cout << "exception " << GetId() << " " << inDev << " " << ch.l3Prot << " " << m_counter[inDev] << " " << sequence << std::endl;
			return 5;
		}
	}
	return 6;
}

//in-network notification
void SwitchNode::SendNotifyToSender(uint32_t m_index, uint16_t left, uint16_t right)
{
	uint16_t counter_era = (left >> 15) & 0xff;
	uint16_t sequence_era = (right >> 15) & 0xff;
	uint16_t counter_value = left & 0x7fff;
	uint16_t sequence_value = right & 0x7fff;
	if(counter_era == sequence_era)
	{
		//std::cout << left << " " << right << std::endl;
		for(uint16_t i = left; i < right; i++)
		{
			//查找信息表的对应表象
			uint16_t index = i % m_lookuptablesize;
			LookUpTableEntry tableentry = lookuptable[m_index][index];

			if(tableentry.protocol == 0x11) //udp
			{
				// std::cout << "data find " << std::dec << GetId() << " " << m_index << " lostsequence ";
				// std::cout << std::dec << i << " tableid " << index << " lostleft " << left << " lostright " << right << " protocol " << unsigned(tableentry.protocol) << " ";
				// std::cout << std::hex << "0" << tableentry.sip << " 0" << tableentry.dip << " "; 
				// std::cout << std::dec << tableentry.sport << " " << tableentry.dport << " " << tableentry.seq << " " << i << " pg " << tableentry.pg << " round " << tableentry.round << " flag " << tableentry.flag << std::endl;

				if((tableentry.flag >> SeqTsHeader::FLAG_LAST) & 1)
				{
					last_udp_find_number++;
				}
				udp_find_number++;
				total_find_number++;
				if(m_onlylastack == 0){ //cv
					// 更新filter
					if(m_filtersize != 0){
						FiveTuple fivetuple;
						fivetuple.sport = tableentry.sport;
						fivetuple.dport = tableentry.dport;
						fivetuple.sip = tableentry.sip;
						fivetuple.dip = tableentry.dip;
						fivetuple.protocol = tableentry.protocol;
						int r = FilterPacket_SADRCV(fivetuple, tableentry.seq, tableentry.round, true);
					}
					//产生通知包给发送方
					//Ptr<Packet> np = ProduceNotifyToSender(tableentry);
					Ptr<Packet> np = ProduceNAckToSender(tableentry);
					CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					np->PeekHeader(chnp);
					uint32_t qIndex = 0;
					np->AddPacketTag(FlowIdTag(888));
					int idx1 = GetOutDev(np, chnp);
					m_devices[idx1]->SwitchSend(qIndex, np, chnp);
				}else if(m_onlylastack == 1){ //sv
					if((tableentry.flag >> SeqTsHeader::FLAG_LAST) & 1){ //last
						Ptr<Packet> lastp = ProduceLastdata(tableentry);
						CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
						lastp->PeekHeader(chnp);
						uint32_t qIndex = tableentry.pg;
						lastp->AddPacketTag(FlowIdTag(888));
						m_devices[m_index]->SwitchSend(qIndex, lastp, chnp);
					}else{ //更新data filter
						if(m_filtersize != 0 && (((tableentry.flag >> SeqTsHeader::FLAG_NOTDATAFILTER) & 1) != 1)){
							FiveTuple fivetuple;
							fivetuple.sport = tableentry.sport;
							fivetuple.dport = tableentry.dport;
							fivetuple.sip = tableentry.sip;
							fivetuple.dip = tableentry.dip;
							fivetuple.protocol = tableentry.protocol;
							int r = FilterPacket_SADRSV(fivetuple, tableentry.seq, tableentry.round, true);
						}
					}
				}
				else if(m_onlylastack == 2) //SLR
				{
					if(m_filtersize != 0){
						FiveTuple fivetuple;
						fivetuple.sport = tableentry.sport;
						fivetuple.dport = tableentry.dport;
						fivetuple.sip = tableentry.sip;
						fivetuple.dip = tableentry.dip;
						fivetuple.protocol = tableentry.protocol;
						int r = FilterPacket_SADRSLR(fivetuple, tableentry.seq, tableentry.round, true);
					}
					//产生通知包给发送方
					//Ptr<Packet> np = ProduceNotifyToSender(tableentry);
					Ptr<Packet> np = ProduceNAckToSender(tableentry);
					CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					np->PeekHeader(chnp);
					uint32_t qIndex = 0;
					np->AddPacketTag(FlowIdTag(888));
					int idx1 = GetOutDev(np, chnp);
					m_devices[idx1]->SwitchSend(qIndex, np, chnp);
				}
			}
			else if(tableentry.protocol == 0xFC) //ack
			{
				{
					// std::cout << "ack find " << std::dec << GetId() << " " << m_index << " lostsequence ";
					// std::cout << std::dec << i << " tableid " << index << " lostleft " << left << " lostright " << right << " protocol " << unsigned(tableentry.protocol) << " ";
					// std::cout << std::hex << "0" << tableentry.dip << " 0" << tableentry.sip << " "; 
					// std::cout << std::dec << tableentry.dport << " " << tableentry.sport << " " << tableentry.seq << " " << i << " pg " << tableentry.pg << " round " << tableentry.round << " flag " << tableentry.flag << std::endl;

					if((tableentry.flag >> qbbHeader::FLAG_LAST) & 1)
					{
						last_ack_find_number++;
					}
					ack_find_number++;
					total_find_number++;

					//产生ACK给发送方
					Ptr<Packet> ap = ProduceAckToSender(tableentry);
					CustomHeader chap(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					ap->PeekHeader(chap);
					uint32_t qIndex;
					if (m_ackHighPrio)
					{
						qIndex = 0;
					}
					else
					{
						qIndex = tableentry.pg;
					}
					ap->AddPacketTag(FlowIdTag(888)); //just this ack from egress
					m_devices[m_index]->SwitchSend(qIndex, ap, chap);
				}
			}
			else if(tableentry.protocol == 0xFD) //nack
			{
				// std::cout << "nack find " << std::dec << GetId() << " " << m_index << " lostsequence ";
				// std::cout << std::dec << i << " tableid " << index << " lostleft " << left << " lostright " << right << " protocol " << unsigned(tableentry.protocol) << " ";
				// std::cout << std::hex << "0" << tableentry.dip << " 0" << tableentry.sip << " "; 
				// std::cout << std::dec << tableentry.dport << " " << tableentry.sport << " " << tableentry.seq << " " << i << " pg " << tableentry.pg << " round " << tableentry.round << " flag " << tableentry.flag << std::endl;
				
				nack_find_number++;
				total_find_number++;
				
				//产生相同的通知包给发送方
				Ptr<Packet> np = ProduceSameNAck(tableentry);
				CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
				np->PeekHeader(chnp);
				uint32_t qIndex = 0;
				np->AddPacketTag(FlowIdTag(888));
				m_devices[m_index]->SwitchSend(qIndex, np, chnp);
			}
			else if(tableentry.protocol == 0xFB) //notify
			{
				std::cout << "notify find " << std::dec << GetId() << " " << m_index << " lostsequence ";
				std::cout << std::dec << i << " tableid " << index << " lostleft " << left << " lostright " << right << " protocol " << unsigned(tableentry.protocol) << " ";
				std::cout << std::hex << "0" << tableentry.dip << " 0" << tableentry.sip << " "; 
				std::cout << std::dec << tableentry.dport << " " << tableentry.sport << " " << tableentry.seq << " " << i << " pg " << tableentry.pg << " round " << tableentry.round << " flag " << tableentry.flag << std::endl;
				
				FB_find_number++;
				total_find_number++;
				
				//产生相同的通知包给发送方
				Ptr<Packet> np = ProduceSameNotify(tableentry);
				CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
				np->PeekHeader(chnp);
				uint32_t qIndex = 0;
				np->AddPacketTag(FlowIdTag(888));
				m_devices[m_index]->SwitchSend(qIndex, np, chnp);
				// np->AddPacketTag(FlowIdTag(m_index));
				// SendToDev(np, chnp);
			}
		}
	}

///*
	else if(counter_era != sequence_era)
	{
		//std::cout << "here " << left << " " << right << std::endl;
		if(counter_era == 0 && sequence_era == 1)
		{
			//std::cout << left << " " << right << std::endl;
			for(uint16_t i = left; i < right; i++)
			{
				uint16_t index = i % m_lookuptablesize;
				LookUpTableEntry tableentry = lookuptable[m_index][index];
				
				if(tableentry.protocol == 0x11) //udp
				{
					// std::cout << "data find " << std::dec << GetId() << " " << m_index << " lostsequence ";
					// std::cout << std::dec << i << " tableid " << index << " lostleft " << left << " lostright " << right << " protocol " << unsigned(tableentry.protocol) << " ";
					// std::cout << std::hex << "0" << tableentry.sip << " 0" << tableentry.dip << " "; 
					// std::cout << std::dec << tableentry.sport << " " << tableentry.dport << " " << tableentry.seq << " " << i << " pg " << tableentry.pg << " round " << tableentry.round << " flag " << tableentry.flag << std::endl;
					
					if((tableentry.flag >> SeqTsHeader::FLAG_LAST) & 1)
					{
						last_udp_find_number++;
					}
					udp_find_number++;
					total_find_number++;

					if(m_onlylastack == 0){ //cv
						// 更新filter
						if(m_filtersize != 0){
							FiveTuple fivetuple;
							fivetuple.sport = tableentry.sport;
							fivetuple.dport = tableentry.dport;
							fivetuple.sip = tableentry.sip;
							fivetuple.dip = tableentry.dip;
							fivetuple.protocol = tableentry.protocol;
							int r = FilterPacket_SADRCV(fivetuple, tableentry.seq, tableentry.round, true);
						}
						//产生通知包给发送方
						//Ptr<Packet> np = ProduceNotifyToSender(tableentry);
						Ptr<Packet> np = ProduceNAckToSender(tableentry);
						CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
						np->PeekHeader(chnp);
						uint32_t qIndex = 0;
						np->AddPacketTag(FlowIdTag(888));
						int idx1 = GetOutDev(np, chnp);
						m_devices[idx1]->SwitchSend(qIndex, np, chnp);
					}else if(m_onlylastack == 1){ //sv
						if((tableentry.flag >> SeqTsHeader::FLAG_LAST) & 1){ //last
							Ptr<Packet> lastp = ProduceLastdata(tableentry);
							CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
							lastp->PeekHeader(chnp);
							uint32_t qIndex = tableentry.pg;
							lastp->AddPacketTag(FlowIdTag(888));
							m_devices[m_index]->SwitchSend(qIndex, lastp, chnp);
						}else{ //更新data filter
							if(m_filtersize != 0 && (((tableentry.flag >> SeqTsHeader::FLAG_NOTDATAFILTER) & 1) != 1)){
								FiveTuple fivetuple;
								fivetuple.sport = tableentry.sport;
								fivetuple.dport = tableentry.dport;
								fivetuple.sip = tableentry.sip;
								fivetuple.dip = tableentry.dip;
								fivetuple.protocol = tableentry.protocol;
								int r = FilterPacket_SADRSV(fivetuple, tableentry.seq, tableentry.round, true);
							}
						}
					}else if(m_onlylastack == 2) //SLR
					{
						if(m_filtersize != 0){
							FiveTuple fivetuple;
							fivetuple.sport = tableentry.sport;
							fivetuple.dport = tableentry.dport;
							fivetuple.sip = tableentry.sip;
							fivetuple.dip = tableentry.dip;
							fivetuple.protocol = tableentry.protocol;
							int r = FilterPacket_SADRSLR(fivetuple, tableentry.seq, tableentry.round, true);
						}
						//产生通知包给发送方
						//Ptr<Packet> np = ProduceNotifyToSender(tableentry);
						Ptr<Packet> np = ProduceNAckToSender(tableentry);
						CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
						np->PeekHeader(chnp);
						uint32_t qIndex = 0;
						np->AddPacketTag(FlowIdTag(888));
						int idx1 = GetOutDev(np, chnp);
						m_devices[idx1]->SwitchSend(qIndex, np, chnp);
					}
				}
				else if(tableentry.protocol == 0xFC) //ack
				{
					{
						// std::cout << "ack find " << std::dec << GetId() << " " << m_index << " lostsequence ";
						// std::cout << std::dec << i << " tableid " << index << " lostleft " << left << " lostright " << right << " protocol " << unsigned(tableentry.protocol) << " ";
						// std::cout << std::hex << "0" << tableentry.dip << " 0" << tableentry.sip << " "; 
						// std::cout << std::dec << tableentry.dport << " " << tableentry.sport << " " << tableentry.seq << " " << i << " pg " << tableentry.pg << " round " << tableentry.round << " flag " << tableentry.flag << std::endl;

						if((tableentry.flag >> qbbHeader::FLAG_LAST) & 1)
						{
							last_ack_find_number++;
						}
						ack_find_number++;
						total_find_number++;

						//产生ACK给发送方
						Ptr<Packet> ap = ProduceAckToSender(tableentry);
						CustomHeader chap(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
						ap->PeekHeader(chap);
						uint32_t qIndex;
						if (m_ackHighPrio)
						{
							qIndex = 0;
						}
						else
						{
							qIndex = tableentry.pg;
						}
						ap->AddPacketTag(FlowIdTag(888)); //just this ack from egress
						m_devices[m_index]->SwitchSend(qIndex, ap, chap);
					}
				}
				else if(tableentry.protocol == 0xFD) //nack
				{
					// std::cout << "nack find " << std::dec << GetId() << " " << m_index << " lostsequence ";
					// std::cout << std::dec << i << " tableid " << index << " lostleft " << left << " lostright " << right << " protocol " << unsigned(tableentry.protocol) << " ";
					// std::cout << std::hex << "0" << tableentry.dip << " 0" << tableentry.sip << " "; 
					// std::cout << std::dec << tableentry.dport << " " << tableentry.sport << " " << tableentry.seq << " " << i << " pg " << tableentry.pg << " round " << tableentry.round << " flag " << tableentry.flag << std::endl;
				
					nack_find_number++;
					total_find_number++;
				
					//产生相同的通知包给发送方
					Ptr<Packet> np = ProduceSameNAck(tableentry);
					CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					np->PeekHeader(chnp);
					uint32_t qIndex = 0;
					np->AddPacketTag(FlowIdTag(888));
					m_devices[m_index]->SwitchSend(qIndex, np, chnp);
				}
				else if(tableentry.protocol == 0xFB) //notify
				{
					// std::cout << "notify find " << std::dec << GetId() << " " << m_index << " lostsequence ";
					// std::cout << std::dec << i << " tableid " << index << " lostleft " << left << " lostright " << right << " protocol " << unsigned(tableentry.protocol) << " ";
					// std::cout << std::hex << "0" << tableentry.dip << " 0" << tableentry.sip << " "; 
					// std::cout << std::dec << tableentry.dport << " " << tableentry.sport << " " << tableentry.seq << " " << i << " pg " << tableentry.pg << " round " << tableentry.round << " flag " << tableentry.flag << std::endl;
					
					FB_find_number++;
					total_find_number++;

					//产生相同的通知包给发送方
					Ptr<Packet> np = ProduceSameNotify(tableentry);
					CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					np->PeekHeader(chnp);
					uint32_t qIndex = 0;
					np->AddPacketTag(FlowIdTag(888));
					m_devices[m_index]->SwitchSend(qIndex, np, chnp);
					// np->AddPacketTag(FlowIdTag(m_index));
					// SendToDev(np, chnp);
				}
			}
		}
		else if(counter_era == 1 && sequence_era == 0)
		{
			for(uint32_t i = left; i <= 0xffff; i++)
			{
				uint16_t index = i % m_lookuptablesize;
				LookUpTableEntry tableentry = lookuptable[m_index][index];
				
				if(tableentry.protocol == 0x11) //udp
				{
					// std::cout << "data find " << std::dec << GetId() << " " << m_index << " lostsequence ";
					// std::cout << std::dec << i << " tableid " << index << " lostleft " << left << " lostright " << right << " protocol " << unsigned(tableentry.protocol) << " ";
					// std::cout << std::hex << "0" << tableentry.sip << " 0" << tableentry.dip << " "; 
					// std::cout << std::dec << tableentry.sport << " " << tableentry.dport << " " << tableentry.seq << " " << i << " pg " << tableentry.pg << " round " << tableentry.round << " flag " << tableentry.flag << std::endl;
					
					if((tableentry.flag >> SeqTsHeader::FLAG_LAST) & 1)
					{
						last_udp_find_number++;
					}
					udp_find_number++;
					total_find_number++;

					if(m_onlylastack == 0){ //cv
						// 更新filter
						if(m_filtersize != 0){
							FiveTuple fivetuple;
							fivetuple.sport = tableentry.sport;
							fivetuple.dport = tableentry.dport;
							fivetuple.sip = tableentry.sip;
							fivetuple.dip = tableentry.dip;
							fivetuple.protocol = tableentry.protocol;
							int r = FilterPacket_SADRCV(fivetuple, tableentry.seq, tableentry.round, true);
						}
						//产生通知包给发送方
						//Ptr<Packet> np = ProduceNotifyToSender(tableentry);
						Ptr<Packet> np = ProduceNAckToSender(tableentry);
						CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
						np->PeekHeader(chnp);
						uint32_t qIndex = 0;
						np->AddPacketTag(FlowIdTag(888));
						int idx1 = GetOutDev(np, chnp);
						m_devices[idx1]->SwitchSend(qIndex, np, chnp);
					}else if(m_onlylastack == 1){ //sv
						if((tableentry.flag >> SeqTsHeader::FLAG_LAST) & 1){ //last
							Ptr<Packet> lastp = ProduceLastdata(tableentry);
							CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
							lastp->PeekHeader(chnp);
							uint32_t qIndex = tableentry.pg;
							lastp->AddPacketTag(FlowIdTag(888));
							m_devices[m_index]->SwitchSend(qIndex, lastp, chnp);
						}else{ //更新data filter
							if(m_filtersize != 0 && (((tableentry.flag >> SeqTsHeader::FLAG_NOTDATAFILTER) & 1) != 1)){
								FiveTuple fivetuple;
								fivetuple.sport = tableentry.sport;
								fivetuple.dport = tableentry.dport;
								fivetuple.sip = tableentry.sip;
								fivetuple.dip = tableentry.dip;
								fivetuple.protocol = tableentry.protocol;
								int r = FilterPacket_SADRSV(fivetuple, tableentry.seq, tableentry.round, true);
							}
						}
					}else if(m_onlylastack == 2) //SLR
					{
						if(m_filtersize != 0){
							FiveTuple fivetuple;
							fivetuple.sport = tableentry.sport;
							fivetuple.dport = tableentry.dport;
							fivetuple.sip = tableentry.sip;
							fivetuple.dip = tableentry.dip;
							fivetuple.protocol = tableentry.protocol;
							int r = FilterPacket_SADRSLR(fivetuple, tableentry.seq, tableentry.round, true);
						}
						//产生通知包给发送方
						//Ptr<Packet> np = ProduceNotifyToSender(tableentry);
						Ptr<Packet> np = ProduceNAckToSender(tableentry);
						CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
						np->PeekHeader(chnp);
						uint32_t qIndex = 0;
						np->AddPacketTag(FlowIdTag(888));
						int idx1 = GetOutDev(np, chnp);
						m_devices[idx1]->SwitchSend(qIndex, np, chnp);
					}
				}
				else if(tableentry.protocol == 0xFC) //ack
				{
					{
						// std::cout << "ack find " << std::dec << GetId() << " " << m_index << " lostsequence ";
						// std::cout << std::dec << i << " tableid " << index << " lostleft " << left << " lostright " << right << " protocol " << unsigned(tableentry.protocol) << " ";
						// std::cout << std::hex << "0" << tableentry.dip << " 0" << tableentry.sip << " "; 
						// std::cout << std::dec << tableentry.dport << " " << tableentry.sport << " " << tableentry.seq << " " << i << " pg " << tableentry.pg << " round " << tableentry.round << " flag " << tableentry.flag << std::endl;

						if((tableentry.flag >> qbbHeader::FLAG_LAST) & 1)
						{
							last_ack_find_number++;
						}
						ack_find_number++;
						total_find_number++;

						//产生ACK给发送方
						Ptr<Packet> ap = ProduceAckToSender(tableentry);
						CustomHeader chap(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
						ap->PeekHeader(chap);
						uint32_t qIndex;
						if (m_ackHighPrio)
						{
							qIndex = 0;
						}
						else
						{
							qIndex = tableentry.pg;
						}
						ap->AddPacketTag(FlowIdTag(888)); //just this ack from egress
						m_devices[m_index]->SwitchSend(qIndex, ap, chap);
					}
				}
				else if(tableentry.protocol == 0xFD) //nack
				{
					// std::cout << "nack find " << std::dec << GetId() << " " << m_index << " lostsequence ";
					// std::cout << std::dec << i << " tableid " << index << " lostleft " << left << " lostright " << right << " protocol " << unsigned(tableentry.protocol) << " ";
					// std::cout << std::hex << "0" << tableentry.dip << " 0" << tableentry.sip << " "; 
					// std::cout << std::dec << tableentry.dport << " " << tableentry.sport << " " << tableentry.seq << " " << i << " pg " << tableentry.pg << " round " << tableentry.round << " flag " << tableentry.flag << std::endl;
				
					nack_find_number++;
					total_find_number++;
				
					//产生相同的通知包给发送方
					Ptr<Packet> np = ProduceSameNAck(tableentry);
					CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					np->PeekHeader(chnp);
					uint32_t qIndex = 0;
					np->AddPacketTag(FlowIdTag(888));
					m_devices[m_index]->SwitchSend(qIndex, np, chnp);
				}
				else if(tableentry.protocol == 0xFB) //notify
				{
					std::cout << "notify find " << std::dec << GetId() << " " << m_index << " lostsequence ";
					std::cout << std::dec << i << " tableid " << index << " lostleft " << left << " lostright " << right << " protocol " << unsigned(tableentry.protocol) << " ";
					std::cout << std::hex << "0" << tableentry.dip << " 0" << tableentry.sip << " "; 
					std::cout << std::dec << tableentry.dport << " " << tableentry.sport << " " << tableentry.seq << " " << i << " pg " << tableentry.pg << " round " << tableentry.round << " flag " << tableentry.flag << std::endl;
					
					FB_find_number++;
					total_find_number++;
					
					//产生相同的通知包给发送方
					Ptr<Packet> np = ProduceSameNotify(tableentry);
					CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					np->PeekHeader(chnp);
					uint32_t qIndex = 0;
					np->AddPacketTag(FlowIdTag(888));
					m_devices[m_index]->SwitchSend(qIndex, np, chnp);
					// np->AddPacketTag(FlowIdTag(m_index));
					// SendToDev(np, chnp);
				}
			}
			for(uint16_t i = 0; i < right; i++)
			{
				uint16_t index = i % m_lookuptablesize;
				LookUpTableEntry tableentry = lookuptable[m_index][index];
				
				if(tableentry.protocol == 0x11) //udp
				{
					// std::cout << "data find " << std::dec << GetId() << " " << m_index << " lostsequence ";
					// std::cout << std::dec << i << " tableid " << index << " lostleft " << left << " lostright " << right << " protocol " << unsigned(tableentry.protocol) << " ";
					// std::cout << std::hex << "0" << tableentry.sip << " 0" << tableentry.dip << " "; 
					// std::cout << std::dec << tableentry.sport << " " << tableentry.dport << " " << tableentry.seq << " " << i << " pg " << tableentry.pg << " round " << tableentry.round << " flag " << tableentry.flag << std::endl;
					
					if((tableentry.flag >> SeqTsHeader::FLAG_LAST) & 1)
					{
						last_udp_find_number++;
					}
					udp_find_number++;
					total_find_number++;

					if(m_onlylastack == 0){ //cv
						// 更新filter
						if(m_filtersize != 0){
							FiveTuple fivetuple;
							fivetuple.sport = tableentry.sport;
							fivetuple.dport = tableentry.dport;
							fivetuple.sip = tableentry.sip;
							fivetuple.dip = tableentry.dip;
							fivetuple.protocol = tableentry.protocol;
							int r = FilterPacket_SADRCV(fivetuple, tableentry.seq, tableentry.round, true);
						}
						//产生通知包给发送方
						//Ptr<Packet> np = ProduceNotifyToSender(tableentry);
						Ptr<Packet> np = ProduceNAckToSender(tableentry);
						CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
						np->PeekHeader(chnp);
						uint32_t qIndex = 0;
						np->AddPacketTag(FlowIdTag(888));
						int idx1 = GetOutDev(np, chnp);
						m_devices[idx1]->SwitchSend(qIndex, np, chnp);
					}else if(m_onlylastack == 1){ //sv
						if((tableentry.flag >> SeqTsHeader::FLAG_LAST) & 1){ //last
							Ptr<Packet> lastp = ProduceLastdata(tableentry);
							CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
							lastp->PeekHeader(chnp);
							uint32_t qIndex = tableentry.pg;
							lastp->AddPacketTag(FlowIdTag(888));
							m_devices[m_index]->SwitchSend(qIndex, lastp, chnp);
						}else{ //更新data filter
							if(m_filtersize != 0 && (((tableentry.flag >> SeqTsHeader::FLAG_NOTDATAFILTER) & 1) != 1)){
								FiveTuple fivetuple;
								fivetuple.sport = tableentry.sport;
								fivetuple.dport = tableentry.dport;
								fivetuple.sip = tableentry.sip;
								fivetuple.dip = tableentry.dip;
								fivetuple.protocol = tableentry.protocol;
								int r = FilterPacket_SADRSV(fivetuple, tableentry.seq, tableentry.round, true);
							}
						}
					}else if(m_onlylastack == 2) //SLR
					{
						if(m_filtersize != 0){
							FiveTuple fivetuple;
							fivetuple.sport = tableentry.sport;
							fivetuple.dport = tableentry.dport;
							fivetuple.sip = tableentry.sip;
							fivetuple.dip = tableentry.dip;
							fivetuple.protocol = tableentry.protocol;
							int r = FilterPacket_SADRSLR(fivetuple, tableentry.seq, tableentry.round, true);
						}
						//产生通知包给发送方
						//Ptr<Packet> np = ProduceNotifyToSender(tableentry);
						Ptr<Packet> np = ProduceNAckToSender(tableentry);
						CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
						np->PeekHeader(chnp);
						uint32_t qIndex = 0;
						np->AddPacketTag(FlowIdTag(888));
						int idx1 = GetOutDev(np, chnp);
						m_devices[idx1]->SwitchSend(qIndex, np, chnp);
					}
				}
				else if(tableentry.protocol == 0xFC) //ack
				{
					{
						// std::cout << "ack find " << std::dec << GetId() << " " << m_index << " lostsequence ";
						// std::cout << std::dec << i << " tableid " << index << " lostleft " << left << " lostright " << right << " protocol " << unsigned(tableentry.protocol) << " ";
						// std::cout << std::hex << "0" << tableentry.dip << " 0" << tableentry.sip << " "; 
						// std::cout << std::dec << tableentry.dport << " " << tableentry.sport << " " << tableentry.seq << " " << i << " pg " << tableentry.pg << " round " << tableentry.round << " flag " << tableentry.flag << std::endl;

						if((tableentry.flag >> qbbHeader::FLAG_LAST) & 1)
						{
							last_ack_find_number++;
						}
						ack_find_number++;
						total_find_number++;

						//产生ACK给发送方
						Ptr<Packet> ap = ProduceAckToSender(tableentry);
						CustomHeader chap(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
						ap->PeekHeader(chap);
						uint32_t qIndex;
						if (m_ackHighPrio)
						{
							qIndex = 0;
						}
						else
						{
							qIndex = tableentry.pg;
						}
						ap->AddPacketTag(FlowIdTag(888)); //just this ack from egress
						m_devices[m_index]->SwitchSend(qIndex, ap, chap);
					}
				}
				else if(tableentry.protocol == 0xFD) //nack
				{
					// std::cout << "nack find " << std::dec << GetId() << " " << m_index << " lostsequence ";
					// std::cout << std::dec << i << " tableid " << index << " lostleft " << left << " lostright " << right << " protocol " << unsigned(tableentry.protocol) << " ";
					// std::cout << std::hex << "0" << tableentry.dip << " 0" << tableentry.sip << " "; 
					// std::cout << std::dec << tableentry.dport << " " << tableentry.sport << " " << tableentry.seq << " " << i << " pg " << tableentry.pg << " round " << tableentry.round << " flag " << tableentry.flag << std::endl;
				
					nack_find_number++;
					total_find_number++;
				
					//产生相同的通知包给发送方
					Ptr<Packet> np = ProduceSameNAck(tableentry);
					CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					np->PeekHeader(chnp);
					uint32_t qIndex = 0;
					np->AddPacketTag(FlowIdTag(888));
					m_devices[m_index]->SwitchSend(qIndex, np, chnp);
				}
				else if(tableentry.protocol == 0xFB) //notify
				{
					std::cout << "notify find " << std::dec << GetId() << " " << m_index << " lostsequence ";
					std::cout << std::dec << i << " tableid " << index << " lostleft " << left << " lostright " << right << " protocol " << unsigned(tableentry.protocol) << " ";
					std::cout << std::hex << "0" << tableentry.dip << " 0" << tableentry.sip << " "; 
					std::cout << std::dec << tableentry.dport << " " << tableentry.sport << " " << tableentry.seq << " " << i << " pg " << tableentry.pg << " round " << tableentry.round << " flag " << tableentry.flag << std::endl;
					
					FB_find_number++;
					total_find_number++;

					//产生相同的通知包给发送方
					Ptr<Packet> np = ProduceSameNotify(tableentry);
					CustomHeader chnp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					np->PeekHeader(chnp);
					uint32_t qIndex = 0;
					np->AddPacketTag(FlowIdTag(888));
					m_devices[m_index]->SwitchSend(qIndex, np, chnp);
					// np->AddPacketTag(FlowIdTag(m_index));
					// SendToDev(np, chnp);
				}
			}
		}
	}
	//std::cout << "check " << m_counter[m_index] << std::endl;
//*/
}

//in-network notification
Ptr<Packet> SwitchNode::ProduceNotifyToSender(LookUpTableEntry tableentry)
{
	Ptr<Packet> notifypacket = Create<Packet>(0);
	NotifyHeader notifyh;
	notifyh.SetSport(tableentry.dport);
	notifyh.SetDport(tableentry.sport);
	notifyh.SetNotifySeq(tableentry.seq);
	notifyh.SetLossRound(tableentry.round);
	notifyh.SetPG(tableentry.pg);
	notifypacket->AddHeader(notifyh);
	Ipv4Header ipv4;
	ipv4.SetProtocol(0xFB);
	ipv4.SetSource(Ipv4Address(tableentry.dip));
	ipv4.SetDestination(Ipv4Address(tableentry.sip));
	ipv4.SetPayloadSize(notifypacket->GetSize());
	ipv4.SetTtl(64);
	ipv4.SetIdentification(UniformVariable(0, 65536).GetValue());
	notifypacket->AddHeader(ipv4);
	AddHeader(notifypacket, 0x0800);
	return notifypacket;
}

Ptr<Packet> SwitchNode::ProduceAckToSender(LookUpTableEntry tableentry)
{
	qbbHeader ackh;
	ackh.SetSeq(tableentry.seq);
	ackh.SetPG(tableentry.pg);
	ackh.SetSport(tableentry.sport);
	ackh.SetDport(tableentry.dport);
	if((tableentry.flag >> qbbHeader::FLAG_CNP) & 1) //CNP
	{
		ackh.SetCnp();
	}
	if((tableentry.flag >> qbbHeader::FLAG_LAST) & 1) //LAST
	{
		ackh.SetLast();
	}
	Ptr<Packet> ackpacket = Create<Packet>(std::max(60-14-20-(int)ackh.GetSerializedSize(), 0));
	ackpacket->AddHeader(ackh);
	Ipv4Header ipv4;
	ipv4.SetProtocol(tableentry.protocol);
	ipv4.SetSource(Ipv4Address(tableentry.sip));
	ipv4.SetDestination(Ipv4Address(tableentry.dip));
	ipv4.SetPayloadSize(ackpacket->GetSize());
	ipv4.SetTtl(64);
	ipv4.SetIdentification(UniformVariable(0, 65536).GetValue());
	ackpacket->AddHeader(ipv4);
	AddHeader(ackpacket, 0x0800);
	return ackpacket;
}

Ptr<Packet> SwitchNode::ProduceSameNotify(LookUpTableEntry tableentry)
{
	Ptr<Packet> notifypacket = Create<Packet>(0);
	NotifyHeader notifyh;
	notifyh.SetSport(tableentry.sport);
	notifyh.SetDport(tableentry.dport);
	notifyh.SetNotifySeq(tableentry.seq);
	notifyh.SetLossRound(tableentry.round);
	notifyh.SetPG(tableentry.pg);
	notifypacket->AddHeader(notifyh);
	Ipv4Header ipv4;
	ipv4.SetProtocol(0xFB);
	ipv4.SetSource(Ipv4Address(tableentry.sip));
	ipv4.SetDestination(Ipv4Address(tableentry.dip));
	ipv4.SetPayloadSize(notifypacket->GetSize());
	ipv4.SetTtl(64);
	ipv4.SetIdentification(UniformVariable(0, 65536).GetValue());
	notifypacket->AddHeader(ipv4);
	AddHeader(notifypacket, 0x0800);
	return notifypacket;
}

Ptr<Packet> SwitchNode::ProduceNAckToSender(LookUpTableEntry tableentry)
{
	Ptr<Packet> nackp = Create<Packet>(0);
	qbbHeader nackh;
	nackh.SetSport(tableentry.dport);
	nackh.SetDport(tableentry.sport);
	nackh.SetSeq(tableentry.seq);
	nackh.SetlatestRxSeqNo(tableentry.round);
	nackh.SetPG(tableentry.pg);
	nackh.SetSwitchNACK();
	nackp->AddHeader(nackh);
	Ipv4Header ipv4;
	ipv4.SetProtocol(0xFD);
	ipv4.SetSource(Ipv4Address(tableentry.dip));
	ipv4.SetDestination(Ipv4Address(tableentry.sip));
	ipv4.SetPayloadSize(nackp->GetSize());
	ipv4.SetTtl(64);
	ipv4.SetIdentification(UniformVariable(0, 65536).GetValue());
	nackp->AddHeader(ipv4);
	AddHeader(nackp, 0x0800);
	// std::cout << "Produce NACK " << std::dec << GetId() << " ";
	// std::cout << std::hex << "0" << tableentry.sip << " 0" << tableentry.dip << " " ;
	// std::cout << std::dec << tableentry.sport << " " << tableentry.dport << " " << tableentry.seq << " " << tableentry.round << std::endl;
	return nackp;
}

Ptr<Packet> SwitchNode::ProduceSameNAck(LookUpTableEntry tableentry)
{
	Ptr<Packet> nackp = Create<Packet>(0);
	qbbHeader nackh;
	nackh.SetSport(tableentry.sport);
	nackh.SetDport(tableentry.dport);
	nackh.SetSeq(tableentry.seq);
	nackh.SetlatestRxSeqNo(tableentry.round);
	nackh.SetPG(tableentry.pg);
	nackh.SetSwitchNACK();
	nackp->AddHeader(nackh);
	Ipv4Header ipv4;
	ipv4.SetProtocol(0xFD);
	ipv4.SetSource(Ipv4Address(tableentry.sip));
	ipv4.SetDestination(Ipv4Address(tableentry.dip));
	ipv4.SetPayloadSize(nackp->GetSize());
	ipv4.SetTtl(64);
	ipv4.SetIdentification(UniformVariable(0, 65536).GetValue());
	nackp->AddHeader(ipv4);
	AddHeader(nackp, 0x0800);
	// std::cout << "Produce Same NACK " << std::dec << GetId() << " ";
	// std::cout << std::hex << "0" << tableentry.dip << " 0" << tableentry.sip << " " ;
	// std::cout << std::dec << tableentry.dport << " " << tableentry.sport << " " << tableentry.seq << " " << tableentry.round << std::endl;
	return nackp;
}

Ptr<Packet> SwitchNode::ProduceLastdata(LookUpTableEntry tableentry){
	//Ptr<Packet> lastp = Create<Packet> (tableentry.size);
	Ptr<Packet> lastp = Create<Packet> (0);
	SeqTsHeader seqTs;
	seqTs.SetSeq (tableentry.seq + 1000);
	seqTs.SetPG (tableentry.pg);
	if((tableentry.flag >> SeqTsHeader::FLAG_LAST) & 1) //last p // just this
	{
		seqTs.SetLast();
	}
	seqTs.SetRound (tableentry.round);
	lastp->AddHeader(seqTs);
	UdpHeader udpHeader;
	udpHeader.SetDestinationPort (tableentry.dport);
	udpHeader.SetSourcePort (tableentry.sport);
	lastp->AddHeader (udpHeader);
	Ipv4Header ipHeader;
	ipHeader.SetSource (Ipv4Address(tableentry.sip));
	ipHeader.SetDestination (Ipv4Address(tableentry.dip));
	ipHeader.SetProtocol (0x11);
	ipHeader.SetPayloadSize (lastp->GetSize());
	ipHeader.SetTtl (64);
	ipHeader.SetTos (0);
	ipHeader.SetIdentification (0);
	lastp->AddHeader(ipHeader);
	PppHeader ppp;
	ppp.SetProtocol (0x0021);
	lastp->AddHeader (ppp);
	return lastp;
}

Ptr<Packet> SwitchNode::ProduceOOOdata(LookUpTableEntry tableentry){
	Ptr<Packet> lastp = Create<Packet> (0);
	SeqTsHeader seqTs;
	seqTs.SetSeq (tableentry.seq + 1000);
	seqTs.SetPG (tableentry.pg);
	// if((tableentry.flag >> SeqTsHeader::FLAG_LAST) & 1) //last p // just this
	// {
	// 	seqTs.SetLast();
	// }
	seqTs.SetRound (tableentry.round);
	lastp->AddHeader(seqTs);
	UdpHeader udpHeader;
	udpHeader.SetDestinationPort (tableentry.dport);
	udpHeader.SetSourcePort (tableentry.sport);
	lastp->AddHeader (udpHeader);
	Ipv4Header ipHeader;
	ipHeader.SetSource (Ipv4Address(tableentry.sip));
	ipHeader.SetDestination (Ipv4Address(tableentry.dip));
	ipHeader.SetProtocol (0x11);
	ipHeader.SetPayloadSize (lastp->GetSize());
	ipHeader.SetTtl (64);
	ipHeader.SetTos (0);
	ipHeader.SetIdentification (0);
	lastp->AddHeader(ipHeader);
	PppHeader ppp;
	ppp.SetProtocol (0x0021);
	lastp->AddHeader (ppp);
	return lastp;
}
//*/

//in-network retransmit
void SwitchNode::UpdateReTxReqs(uint32_t m_index, uint16_t left, uint16_t right)
{
	uint16_t counter_era = (left >> 15) & 0xff;
	uint16_t sequence_era = (right >> 15) & 0xff;
	uint16_t counter_value = left & 0x7fff;
	uint16_t sequence_value = right & 0x7fff;
	if(counter_era == sequence_era){
		for(uint16_t i = left; i < right; i++)
		m_reTxReqs[m_index].push_back(i);
	}
	else if(counter_era != sequence_era){
		if(counter_era == 0 && sequence_era == 1){
			for(uint16_t i = left; i < right; i++){
				m_reTxReqs[m_index].push_back(i);
			}
		}
		else if(counter_era == 1 && sequence_era == 0){
			for(uint32_t i = left; i <= 0xffff; i++){
				m_reTxReqs[m_index].push_back(i);
			}
			for(uint16_t i = 0; i < right; i++){
				m_reTxReqs[m_index].push_back(i);
			}
		}
	}
	
	for(uint16_t i : m_reTxReqs[m_index]){
		//std::cout << Simulator::Now() << " " << "receive loss notification and update ReTxReqs " << i << std::endl;
	}

	m_uplatestRxSeqNo[m_index] = right;
	start_state[m_index] = 1;
}

void SwitchNode::SendToDev(Ptr<Packet>p, CustomHeader &ch){
	int idx = GetOutDev(p, ch);
	if (idx >= 0){
		NS_ASSERT_MSG(m_devices[idx]->IsLinkUp(), "The routing table look up should return link that is up");
		// determine the qIndex
		uint32_t qIndex;
		if (ch.l3Prot == 0xFF || ch.l3Prot == 0xFE || ch.l3Prot == 0xFB || ch.l3Prot == 0xFA || (m_ackHighPrio && (ch.l3Prot == 0xFD || ch.l3Prot == 0xFC)) || (m_RecMode == 1 && (m_onlylastack == 0 || m_onlylastack == 2) && ch.l3Prot == 0xFD)){  //QCN or PFC or NACK, go highest priority
			qIndex = 0;
		}else{
			if(ch.l3Prot == 0x11) //udp
			{
				qIndex = ch.udp.pg;
			}
			else
			{
				qIndex = ch.l3Prot == 0x06 ? 1 : ch.udp.pg ; // if TCP, put to queue 1
			}
		}
		// admission control
		FlowIdTag t;
		p->PeekPacketTag(t);
		uint32_t inDev = t.GetFlowId();

//notification
		if(m_RecMode == 1){
			//if(GetId() == 3 && inDev == 1){ //指定链路检测序号
			if(ToIsSwitch(inDev)){
				if(ch.l3Prot == 0x11 || ch.l3Prot == 0xFC || ch.l3Prot == 0xFD || ch.l3Prot == 0xFB)//udp ack nack
				{
					if(ch.l3Prot == 0x11)
					{
						//std::cout << "udp " << GetId() << " " << inDev << " " << m_counter[inDev] << " " << ch.udp.sequence << std::endl;
						uint16_t x = CheckAndSendNotifyToSender(inDev, ch.udp.sequence, p, ch);
					}
					else if(ch.l3Prot == 0xFC || ch.l3Prot == 0xFD)
					{
						//std::cout << "ack " << GetId() << " " << inDev << " " << m_counter[inDev] << " " << ch.ack.sequence << std::endl;
						uint16_t x = CheckAndSendNotifyToSender(inDev, ch.ack.sequence, p, ch);
					}
					else if(ch.l3Prot == 0xFB)
					{
						//std::cout << "notify " << GetId() << " " << inDev << " " << m_counter[inDev] << " " << ch.notify.m_sequence << std::endl;
						uint16_t x = CheckAndSendNotifyToSender(inDev, ch.notify.m_sequence, p, ch);
					}
				}
			}

			if(m_onlylastack == 0) //cv
			{
				// if(m_filtersize != 0)
				// {
				// 	if(IsTorSwitch()){
				// 		if(!ToIsSwitch(inDev) && ch.l3Prot == 0x11){ //TOR of Sender, data
				// 			FiveTuple fivetuple1;
				// 			fivetuple1.sport = ch.udp.sport;
				// 			fivetuple1.dport = ch.udp.dport;
				// 			fivetuple1.sip = ch.sip;
				// 			fivetuple1.dip = ch.dip;
				// 			fivetuple1.protocol = 0xFD;
				// 			uint32_t index = MurmurHash3(&fivetuple1, 13, m_ecmpSeed) % m_filtersize;
				// 			if(IsNullFlowID(NACKFilter[index].filter_flowid) || (!IsEqualFlowID(fivetuple1, NACKFilter[index].filter_flowid))){ //NACKFilter中没有这个流, 表示这个流没有发生丢包
				// 				PppHeader ppph;
				// 				Ipv4Header iph;
				// 				UdpHeader udph;
				// 				SeqTsHeader sth;
				// 				p->RemoveHeader(ppph);
				// 				p->RemoveHeader(iph);
				// 				p->RemoveHeader(udph);
				// 				p->RemoveHeader(sth);
				// 				sth.SetRound(0); //0轮
				// 				p->AddHeader(sth);
				// 				p->AddHeader(udph);
				// 				p->AddHeader(iph);
				// 				p->AddHeader(ppph);
				// 			}else{ //NACKFilter中有这个流
				// 				if(ch.udp.seq == NACKFilter[index].filter_seq){
				// 					std::cout << "NACK new round starts vaild " << std::dec << GetId() << " " << inDev << " ";
				// 					std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
				// 					std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.round << std::endl;
				// 					NACKFilter[index].vaild = 1; //新一轮生效
				// 				}

				// 				if(NACKFilter[index].vaild == 0) //NACK新一轮还未生效，该data还是上一轮的
				// 				{
				// 					PppHeader ppph;
				// 					Ipv4Header iph;
				// 					UdpHeader udph;
				// 					SeqTsHeader sth;
				// 					p->RemoveHeader(ppph);
				// 					p->RemoveHeader(iph);
				// 					p->RemoveHeader(udph);
				// 					p->RemoveHeader(sth);
				// 					sth.SetRound(NACKFilter[index].filter_round - 1); //上一轮
				// 					p->AddHeader(sth);
				// 					p->AddHeader(udph);
				// 					p->AddHeader(iph);
				// 					p->AddHeader(ppph);
				// 					ch.udp.round = NACKFilter[index].filter_round - 1;
				// 				}else{
				// 					PppHeader ppph;
				// 					Ipv4Header iph;
				// 					UdpHeader udph;
				// 					SeqTsHeader sth;
				// 					p->RemoveHeader(ppph);
				// 					p->RemoveHeader(iph);
				// 					p->RemoveHeader(udph);
				// 					p->RemoveHeader(sth);
				// 					sth.SetRound(NACKFilter[index].filter_round); //当前NACK轮
				// 					p->AddHeader(sth);
				// 					p->AddHeader(udph);
				// 					p->AddHeader(iph);
				// 					p->AddHeader(ppph);
				// 					ch.udp.round = NACKFilter[index].filter_round;
				// 				}
				// 				//std::cout << "vaild " << unsigned(NACKFilter[index].vaild) << " data round " << ch.udp.round << " NACKFilter round " <<  NACKFilter[index].filter_round << std::endl;
				// 			}
				// 			// std::cout << "TOR data p " << std::dec << GetId() << " " << inDev << " ";
				// 			// std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
				// 			// std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.round << std::endl;
				// 		}else if(ch.l3Prot == 0xFD){ //NACK
				// 			PppHeader ppph;
				// 			Ipv4Header iph;
				// 			qbbHeader nackh;
				// 			p->RemoveHeader(ppph);
				// 			p->RemoveHeader(iph);
				// 			p->RemoveHeader(nackh);
				// 			p->AddHeader(nackh);
				// 			p->AddHeader(iph);
				// 			p->AddHeader(ppph);
				// 			if(nackh.GetSwitchNACK() == 0)
				// 			{
				// 				std::cout << "filter NACK from sever " << std::dec << GetId() << " " << inDev << " ";
				// 				std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
				// 				std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << std::endl;
				// 				return; //丢掉Sever的NACK
				// 			}
				// 			FiveTuple fivetuple2;
				// 			fivetuple2.sport = ch.ack.dport;
				// 			fivetuple2.dport = ch.ack.sport;
				// 			fivetuple2.sip = ch.dip;
				// 			fivetuple2.dip = ch.sip;
				// 			fivetuple2.protocol = 0xFD;
				// 			int nr = FilterNACKPacket(fivetuple2, ch.ack.latestRxSeqNo, ch.ack.seq);
				// 			if(nr == 2){
				// 				std::cout << "nackfilter " << std::dec << GetId() << " " << inDev << " ";
				// 				std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
				// 				std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << std::endl; 
				// 				filter_NACK_number++;
				// 				return; //drop
				// 			}
				// 		}
				// 	}
					if(m_filtersize != 0 && ch.l3Prot == 0x11) //udp filter
					{
						FiveTuple fivetuple;
						fivetuple.sport = ch.udp.sport;
						fivetuple.dport = ch.udp.dport;
						fivetuple.sip = ch.sip;
						fivetuple.dip = ch.dip;
						fivetuple.protocol = ch.l3Prot;
						//std::cout << ch.udp.round << std::endl;
						int r = FilterPacket_SADRCV(fivetuple, ch.udp.seq, ch.udp.round, false);
						//std::cout << "data_filter " << GetId() << " " << ch.udp.seq << " " << r << std::endl;
						if(r == 6)//filter
						{
							// if(ch.sip == 0x0b002701 && ch.dip == 0x0b00da01 && ch.udp.sport == 10001 && ch.udp.dport ==100)
							{
					 			// std::cout << "datafilter " << std::dec << GetId() << " ";
					 			// std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
					 			// std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.sequence << " pg " << ch.udp.pg << " round " << ch.udp.round << std::endl;
							}
							filter_data_number++;
							return;// drop
						}
					}
				//}
			}
			else if(m_onlylastack == 1) //sv
			{
				if(ch.l3Prot == 0xFD && IsTorSwitch() && ToIsSwitch(inDev)){ //TOR of Sender, NACK
					RecordNACKEseq(ch);
					// std::cout << "RecordNACKEseq " << std::dec << GetId() << " ";
					// std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
					// std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << std::endl;
				}else if(ch.l3Prot == 0x11 && IsTorSwitch() && !ToIsSwitch(inDev)){ //TOR of Sender, data
					bool isnackseq = IsNACKEseq(ch);
					if(isnackseq){ //replicate and let it far from data filter
						PppHeader ppph;
						Ipv4Header iph;
						UdpHeader udph;
						SeqTsHeader sth;
						p->RemoveHeader(ppph);
						p->RemoveHeader(iph);
						p->RemoveHeader(udph);
						p->RemoveHeader(sth);
						sth.SetNotDataFilter(); // twice 不参与data filter
						p->AddHeader(sth);
						p->AddHeader(udph);
						p->AddHeader(iph);
						p->AddHeader(ppph);

						Ptr<Packet> pc = ReplicateDataPacket(p, ch);
						CustomHeader chpc(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
						pc->PeekHeader(chpc);
						// std::cout << "ReplicateDataPacket " << std::dec << GetId() << " ";
						// std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
						// std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << std::endl;
						pc->AddPacketTag(FlowIdTag(888));
						m_devices[idx]->SwitchSend(qIndex, pc, chpc);

						Ptr<Packet> pc1 = ReplicateDataPacket(p, ch);
						CustomHeader chpc1(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
						pc1->PeekHeader(chpc1);
						// std::cout << "ReplicateDataPacket " << std::dec << GetId() << " ";
						// std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
						// std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << std::endl;
						pc1->AddPacketTag(FlowIdTag(888));
						m_devices[idx]->SwitchSend(qIndex, pc1, chpc1);
					}
				}

				if(m_filtersize != 0 && ch.l3Prot == 0x11) //udp filter
					{
						FiveTuple fivetuple;
						fivetuple.sport = ch.udp.sport;
						fivetuple.dport = ch.udp.dport;
						fivetuple.sip = ch.sip;
						fivetuple.dip = ch.dip;
						fivetuple.protocol = ch.l3Prot;
						int r = FilterPacket_SADRSV(fivetuple, ch.udp.seq, ch.udp.round, false);
						//std::cout << "data_filter " << GetId() << " " << ch.udp.seq << " " << r << std::endl;
						if(r == 6)//filter
						{
							// if(ch.sip == 0x0b005201 && ch.dip == 0x0b011d01 && ch.udp.sport == 10000 && ch.udp.dport ==100)
							// { 0b005201 0b011d01 10000 100
					 		// 	std::cout << "datafilter " << std::dec << GetId() << " ";
					 		// 	std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
					 		// 	std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << std::endl;
							// }
							filter_data_number++;
							return;// drop
						}
						else if(r == 9){ //subsequence
							PppHeader ppph;
							Ipv4Header iph;
							UdpHeader udph;
							SeqTsHeader sth;
							p->RemoveHeader(ppph);
							p->RemoveHeader(iph);
							p->RemoveHeader(udph);
							p->RemoveHeader(sth);
							sth.SetLast(); // 保护subsequence p
							p->AddHeader(sth);
							p->AddHeader(udph);
							p->AddHeader(iph);
							p->AddHeader(ppph);
						}
					}
			}
			else if(m_onlylastack == 2){
				if(m_filtersize != 0 && ch.l3Prot == 0x11) //udp filter
				{
					FiveTuple fivetuple;
					fivetuple.sport = ch.udp.sport;
					fivetuple.dport = ch.udp.dport;
					fivetuple.sip = ch.sip;
					fivetuple.dip = ch.dip;
					fivetuple.protocol = ch.l3Prot;
					//std::cout << ch.udp.round << std::endl;
					int r = FilterPacket_SADRSLR(fivetuple, ch.udp.seq, ch.udp.round, false);
					//std::cout << "data_filter " << GetId() << " " << ch.udp.seq << " " << r << std::endl;
					if(r == 6)//filter
					{
						// if(ch.sip == 0x0b002701 && ch.dip == 0x0b00da01 && ch.udp.sport == 10001 && ch.udp.dport ==100)
						{
					 		// std::cout << "datafilter " << std::dec << GetId() << " ";
					 		// std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
					 		// std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.sequence << " pg " << ch.udp.pg << " round " << ch.udp.round << std::endl;
						}
						filter_data_number++;
						return;// drop
					}
				}
			}
		}

//retransmit
		else if(m_RecMode == 2){
			// if(GetId() == 3 && inDev == 1){ //下游检测包序号
			// if((GetId() == 360 && inDev == 1) ||
	   	   	//    (GetId() == 373 && inDev == 2) || 
	        //    (GetId() == 345 && inDev == 5) ||
	        //    (GetId() == 355 && inDev == 6) ||
	        //    (GetId() == 333 && inDev == 3) ||
	        //    (GetId() == 328 && inDev == 4)){

			if(ToIsSwitch(inDev)){
				if(ch.l3Prot == 0x11 || ch.l3Prot == 0xFC || ch.l3Prot == 0xFD){ //udp ack nack
					if(ch.l3Prot == 0x11){
						if(((ch.udp.flags >> SeqTsHeader::FLAG_COPYRECIRCULATE) & 1) != 1){
							//std::cout << "udp " << GetId() << " " << inDev << " " << m_counter[inDev] << " " << ch.udp.sequence << std::endl;
							uint16_t x = CheckAndSendNotifyToSender(inDev, ch.udp.sequence, p, ch);
							m_downlatestRxSeqNo[inDev] = ch.udp.sequence; //更新m_downlatestRxSeqNo
							m_downPendingAck[inDev] = 1;
							// if(GetId() == 373 && inDev == 2){
							// 	std::cout << "down RxSeqNo " << m_downlatestRxSeqNo[inDev] << std::endl;
							// }
							// if(Simulator::Now().GetTimeStep() > 2005278207){
							// 	std::cout << Simulator::Now() << " " << "down receive normal data p and update downlatestRxSeqNo " << std::dec << GetId() << " " << inDev << " ";
							// 	std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
							// 	std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << m_downlatestRxSeqNo[inDev] << std::endl;
							// }
					
							//std::cout << Simulator::Now() << " " << "down receive data normal data p and update downlatestRxSeqNo " << GetId() << " " << inDev << " " << ch.udp.sport << " " << ch.udp.dport << " " << m_downlatestRxSeqNo[inDev] << " reorderingsize " << m_reorderingsize << std::endl;
							//提醒下游发送ack给上游及时更新lastestseq
							m_devices[inDev]->TriggerTransmit();
						}
						else{
							// if(Simulator::Now().GetTimeStep() > 2005278207){
							// 	std::cout << Simulator::Now() << " " << "down receive data retransmit data p " << std::dec << GetId() << " " << inDev << " ";
							// 	std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
							// 	std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.sequence << std::endl;
							// }//std::cout << Simulator::Now() << " " << "down receive data retransmit data p " << GetId() << " " << inDev << " " << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.sequence << " reorderingsize " << m_reorderingsize << std::endl;
						}

						//normal data p, retransmit data p
						int re = CheckInOrderRecoverySeq(m_ackNo[inDev], ch.udp.sequence);
						if(m_onlylastack == 1){
							re = 1;
						}	
						if(re == 1){
							//send
							//std::cout << Simulator::Now() << " " << "forward protected normal data p ackNo " << GetId() << " " << inDev << " " << ch.udp.sequence << " " << m_ackNo[inDev] << std::endl;
							m_ackNo[inDev]++;
						}else if(re == 2){
							//进reordering buffer
							//std::cout << Simulator::Now() << " " << "reordering protected normal data p ackNo "<< GetId() << " " << inDev << " " << ch.udp.sequence << " " << m_ackNo[inDev] << std::endl;
							//set reordering recirculate
							PppHeader ppph;
							Ipv4Header iph;
							UdpHeader udph;
							SeqTsHeader sth;
							p->RemoveHeader(ppph);
							p->RemoveHeader(iph);
							p->RemoveHeader(udph);
							p->RemoveHeader(sth);
							sth.SetReorderingRecirculate();
							p->AddHeader(sth);
							p->AddHeader(udph);
							p->AddHeader(iph);
							p->AddHeader(ppph);

							EnReorderingQueue(p, inDev);
							//中断
							return;
						}else if(re == 3){
							//de-duplication, drop
							//std::cout << Simulator::Now() << " " << "drop protected normal data p ackNo " << GetId() << " " << inDev << " " << ch.udp.sequence << " " << m_ackNo[inDev] << std::endl;
							return;
						}
					}
					else if(ch.l3Prot == 0xFC || ch.l3Prot == 0xFD){
						if(((ch.ack.flags >> qbbHeader::FLAG_COPYRECIRCULATE) & 1) != 1){
							uint16_t x = CheckAndSendNotifyToSender(inDev, ch.ack.sequence, p, ch);
							m_downlatestRxSeqNo[inDev] = ch.ack.sequence; //更新m_downlatestRxSeqNo
							m_downPendingAck[inDev] = 1;
							// if(GetId() == 373 && inDev == 2){
							// 	std::cout << "down RxSeqNo " << m_downlatestRxSeqNo[inDev] << std::endl;
							// }
							// if(Simulator::Now().GetTimeStep() > 2005278207){
							// 	std::cout << Simulator::Now() << " " << "down receive normal ack p and update downlatestRxSeqNo " << std::dec << GetId() << " " << inDev << " ";
							// 	std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
							// 	std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << m_downlatestRxSeqNo[inDev] << std::endl;
							// 	}//std::cout << Simulator::Now() << " " << "down receive normal ack p and update downlatestRxSeqNo " << GetId() << " " << inDev << " " << ch.ack.dport << " " << ch.ack.sport << " " << m_downlatestRxSeqNo[inDev] << " reorderingsize " << m_reorderingsize << std::endl;
							//提醒下游发送ack给上游及时更新lastestseq
							m_devices[inDev]->TriggerTransmit();
						}
						else{
							// if(Simulator::Now().GetTimeStep() > 2005278207){
							// 	std::cout << Simulator::Now() << " " << "down receive data retransmit ack p " << std::dec << GetId() << " " << inDev << " ";
							// 	std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
							// 	std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << ch.ack.sequence << std::endl;
							// }//std::cout << Simulator::Now() << " " << "down receive data retransmit ack p " << GetId() << " " << inDev << " " << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.sequence << " reorderingsize " << m_reorderingsize << std::endl;
						}
						//normal ack p, retransmit ack p
						int re = CheckInOrderRecoverySeq(m_ackNo[inDev], ch.ack.sequence);
						if(m_onlylastack == 1){
							re = 1;
						}
						if(re == 1){
							//send
							//std::cout << Simulator::Now() << " " << "forward protected normal ack p ackNo " << GetId() << " " << inDev << " " << ch.ack.sequence << " " << m_ackNo[inDev] << std::endl;
							m_ackNo[inDev]++;
						}else if(re == 2){
							//进reordering buffer
							//std::cout << Simulator::Now() << " " << "reordering protected normal ack p ackNo " << GetId() << " " << inDev << " " << ch.ack.sequence << " " << m_ackNo[inDev] << std::endl;
							//set reordering recirculate
							PppHeader ppph;
							Ipv4Header iph;
							qbbHeader ackh;
							p->RemoveHeader(ppph);
							p->RemoveHeader(iph);
							p->RemoveHeader(ackh);
							ackh.SetReorderingRecirculate();
							p->AddHeader(ackh);
							p->AddHeader(iph);
							p->AddHeader(ppph);

							EnReorderingQueue(p, inDev);
							//中断
							return;
						}else if(re == 3){
							//de-duplication, drop
							//std::cout << Simulator::Now() << " " << "drop protected normal ack p ackNo " << GetId() << " " << inDev << " " << ch.ack.sequence << " " << m_ackNo[inDev] << std::endl;
							return;
						}
					}
				}
			}
			// else if(GetId() == 2 && inDev == 2) //上游更新uplatestRxSeqNo
			// else if((GetId() == 340 && inDev == 1) ||
			//     (GetId() == 347 && inDev == 2) ||
			// 	(GetId() == 324 && inDev == 2) ||
			// 	(GetId() == 333 && inDev == 4) ||
			// 	(GetId() == 354 && inDev == 6) ||
			// 	(GetId() == 351 && inDev == 5))

			if(ToIsSwitch(inDev))
			{
				if(ch.l3Prot == 0x11 || ch.l3Prot == 0xFC || ch.l3Prot == 0xFD)//udp ack nack
				{
					if(ch.l3Prot == 0xFC || ch.l3Prot == 0xFD)
					{
						if(((ch.ack.flags >> qbbHeader::FLAG_COPYRECIRCULATE) & 1) != 1){
							start_state[inDev] = 1;
							//std::cout << "udp " << GetId() << " " << inDev << " " << m_counter[inDev] << " " << ch.udp.sequence << std::endl;
							if((ch.ack.flags >> qbbHeader::FLAG_PENDINGACK)) //should update
							{
								m_uplatestRxSeqNo[inDev] = ch.ack.latestRxSeqNo;
								// if(GetId() == 347 && inDev == 2){
								// 	std::cout << "update SeqNo ack " << m_uplatestRxSeqNo[inDev] << std::endl;
								// }
								uint8_t* buf2 = p->GetBuffer();
								buf2[PppHeader::GetStaticSize() + 20 + 5] &= (uint8_t(254) & 0xff);
								// if(Simulator::Now().GetTimeStep() > 2005278207){
								// 	std::cout << Simulator::Now() << " " << "up receive ack normal p and update uplatestRxSeqNo " << std::dec << GetId() << " " << inDev << " ";
								// 	std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
								// 	std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << m_uplatestRxSeqNo[inDev] << std::endl;
								// }//std::cout << Simulator::Now() << " " << "up receive ack normal p and update uplatestRxSeqNo " << m_uplatestRxSeqNo[inDev] << std::endl;
							}
						}else{
							uint8_t* buf2 = p->GetBuffer();
							buf2[PppHeader::GetStaticSize() + 20 + 5] &= (uint8_t(253) & 0xff);
						}
					}
					else if(ch.l3Prot == 0x11)
					{
						if(((ch.udp.flags >> SeqTsHeader::FLAG_COPYRECIRCULATE) & 1) != 1)
						{
							start_state[inDev] = 1;
							if((ch.udp.flags >> SeqTsHeader::FLAG_PENDINGACK)) //should update
							{
								m_uplatestRxSeqNo[inDev] = ch.udp.round;
								// if(GetId() == 347 && inDev == 2){
								// 	std::cout << "update SeqNo data " << m_uplatestRxSeqNo[inDev] << std::endl;
								// }
								uint8_t* buf2 = p->GetBuffer();
								//std::cout << unsigned(buf2[PppHeader::GetStaticSize() + 20 + 8 + 6]) << std::endl;
								buf2[PppHeader::GetStaticSize() + 20 + 8 + 6] &= (uint8_t(254) & 0xff);
								//std::cout << unsigned(buf2[PppHeader::GetStaticSize() + 20 + 8 + 6]) << std::endl;
								// if(Simulator::Now().GetTimeStep() > 2005278207){
								// 	std::cout << Simulator::Now() << " " << "up receive data normal p and update uplatestRxSeqNo " << std::dec << GetId() << " " << inDev << " ";
								// 	std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
								// 	std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << m_uplatestRxSeqNo[inDev] << std::endl;
								// }//std::cout << Simulator::Now() << " " << "up receive data normal p and update uplatestRxSeqNo " << m_uplatestRxSeqNo[inDev] << std::endl;
							}
						}else{
							uint8_t* buf2 = p->GetBuffer();
							buf2[PppHeader::GetStaticSize() + 20 + 8 + 6] &= (uint8_t(253) & 0xff);
						}
					}
				}
			}
		}

		if (qIndex != 0){ //not highest priority, and noe lowest priority
			if (m_mmu->CheckIngressAdmission(inDev, qIndex, p->GetSize()) && m_mmu->CheckEgressAdmission(idx, qIndex, p->GetSize())){			// Admission control
				m_mmu->UpdateIngressAdmission(inDev, qIndex, p->GetSize());
				m_mmu->UpdateEgressAdmission(idx, qIndex, p->GetSize());
			}else{
				std::cout << "congestion drop" << std::endl;
				return; // Drop
			}
			CheckAndSendPfc(inDev, qIndex);
		}
		m_bytes[inDev][idx][qIndex] += p->GetSize();

		m_devices[idx]->SwitchSend(qIndex, p, ch);
	}else
		return; // Drop
}

uint32_t SwitchNode::EcmpHash(const uint8_t* key, size_t len, uint32_t seed) { //Murmurhash
  uint32_t h = seed;
  if (len > 3) {
    const uint32_t* key_x4 = (const uint32_t*) key;
    size_t i = len >> 2;
    do {
      uint32_t k = *key_x4++;
      k *= 0xcc9e2d51;
      k = (k << 15) | (k >> 17);
      k *= 0x1b873593;
      h ^= k;
      h = (h << 13) | (h >> 19);
      h += (h << 2) + 0xe6546b64;
    } while (--i);
    key = (const uint8_t*) key_x4;
  }
  if (len & 3) {
    size_t i = len & 3;
    uint32_t k = 0;
    key = &key[i - 1];
    do {
      k <<= 8;
      k |= *key--;
    } while (--i);
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    h ^= k;
  }
  h ^= len;
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

void SwitchNode::SetEcmpSeed(uint32_t seed){
	m_ecmpSeed = seed;
}

void SwitchNode::AddTableEntry(Ipv4Address &dstAddr, uint32_t intf_idx){
	uint32_t dip = dstAddr.Get();
	m_rtTable[dip].push_back(intf_idx);
}

void SwitchNode::ClearTable(){
	m_rtTable.clear();
}

// This function can only be called in switch mode
bool SwitchNode::SwitchReceiveFromDevice(Ptr<NetDevice> device, Ptr<Packet> packet, CustomHeader &ch){
//n
	if(m_RecMode == 1){
		//if(GetId() == 3 && device->GetIfIndex() == 1){ //指定链路收到空数据包
		if(ToIsSwitch(device->GetIfIndex())){
			if(ch.l3Prot == 0xF9){ //empty data p
				FlowIdTag t;
				packet->PeekPacketTag(t);
				uint32_t inDev = t.GetFlowId();
				//std::cout << "empty data " << GetId() << " " << inDev << " " << m_counter[inDev] << " " << ch.empty.empty_seq << std::endl;
				uint16_t x = CheckAndSendNotifyToSender(inDev, ch.empty.empty_seq, packet, ch);
				return false; //drop empty
			}
		}
	} 
//r
	if(m_RecMode == 2){
		// if(GetId() == 3 && device->GetIfIndex() == 1){ //指定链路收到空数据包
		// if((GetId() == 360 && device->GetIfIndex() == 1) ||
	   	//    (GetId() == 373 && device->GetIfIndex() == 2) || 
	    //    (GetId() == 345 && device->GetIfIndex() == 5) ||
	    //    (GetId() == 355 && device->GetIfIndex() == 6) ||
	    //    (GetId() == 333 && device->GetIfIndex() == 3) ||
	    //    (GetId() == 328 && device->GetIfIndex() == 4)){
		if(ToIsSwitch(device->GetIfIndex())){
			if(ch.l3Prot == 0xF9){ //empty data p
				FlowIdTag t;
				packet->PeekPacketTag(t);
				uint32_t inDev = t.GetFlowId();
				uint16_t x = CheckAndSendNotifyToSender(inDev, ch.empty.empty_seq, packet, ch);
				m_downlatestRxSeqNo[inDev] = ch.empty.empty_seq; //更新m_downlatestRxSeqNo
				m_downPendingAck[inDev] = 1;
				//std::cout << Simulator::Now() << " " << "down receive data [empty] p and update downlatestRxSeqNo " << GetId() << " " << inDev << " " << m_downlatestRxSeqNo[inDev] << std::endl;
				m_devices[inDev]->TriggerTransmit();
				//empty data,
				int re = CheckInOrderRecoverySeq(m_ackNo[inDev], ch.empty.empty_seq);
				if(m_onlylastack == 1){
					re = 1;
				}
				if(re == 1){
					//std::cout << Simulator::Now() << " " << "forward protected [empty] data p ackNo " << GetId() << " " << inDev << " " << ch.empty.empty_seq << " " << m_ackNo[inDev] << std::endl;
					m_ackNo[inDev]++;
					return false; //drop empty
				}else if(re == 2){
					//直接进reordering buffer
					//std::cout << Simulator::Now() << " " << "reordering protected [empty] data p ackNo " << GetId() << " " << inDev << " " << ch.empty.empty_seq << " " << m_ackNo[inDev] << std::endl;
					EnReorderingQueue(packet, inDev);
					return false; //drop empty
				}else if(re == 3){
					//de-duplication, drop
					//std::cout << Simulator::Now() << " " << "drop protected [empty] data p ackNo " << GetId() << " " << inDev << " " << ch.empty.empty_seq << " " << m_ackNo[inDev] << std::endl;
					return false; //drop empty
				}
				//return false; //drop empty
			}
		}
		// else if(GetId() == 2 && device->GetIfIndex() == 2){ //指定链路收到空ack包
		// else if((GetId() == 340 && device->GetIfIndex() == 1) ||
		// 	    (GetId() == 347 && device->GetIfIndex() == 2) ||
		// 		(GetId() == 324 && device->GetIfIndex() == 2) ||
		// 		(GetId() == 333 && device->GetIfIndex() == 4) ||
		// 		(GetId() == 354 && device->GetIfIndex() == 6) ||
		// 		(GetId() == 351 && device->GetIfIndex() == 5)){

		if(ToIsSwitch(device->GetIfIndex())){
			if(ch.l3Prot == 0xF8){ //empty ack
				start_state[device->GetIfIndex()] = 1;
				if(ch.empty.pendingack == 1)
				{
					m_uplatestRxSeqNo[device->GetIfIndex()] = ch.empty.empty_seq;
					// if(GetId() == 347 && device->GetIfIndex() == 2){
					// 	std::cout << "update SeqNo empty " << m_uplatestRxSeqNo[device->GetIfIndex()] << std::endl;
					// }
					// if(Simulator::Now().GetTimeStep() > 2005278207){
					// 	std::cout << Simulator::Now() << " " << "up receive ack [empty] p and update uplatestRxSeqNo " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
					// 	std::cout << std::dec << m_uplatestRxSeqNo[device->GetIfIndex()] << std::endl;
					// }
				}
				return false;
			}
		}
	}

// all related sw
///*
	if(m_lossrate != 0) //has loss
	{
		if(device->GetChannel()->GetDevice(0) != device)
		{
			Ptr<NetDevice> tmp = device->GetChannel()->GetDevice (0);
			if(tmp->GetNode()->GetNodeType() == 1) //to is sw
			{
				if(ch.l3Prot == 0x11 || ch.l3Prot == 0xFC || ch.l3Prot == 0xFD || ch.l3Prot == 0xFB) //udp and ack and nack
				{
					uint16_t randomnum = rand() % m_lossrate;
					if(randomnum == 0)
					{
						total_lost_number++;
						if(ch.l3Prot == 0x11)
						{
							if(m_RecMode == 0 || m_RecMode == 2)
							{
								std::cout << "data lost " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
								std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
								std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.sequence << std::endl;
							}
							else if(m_RecMode == 1) //n
							{
								std::cout << "data lost " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
								std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
								std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.sequence << " round " << ch.udp.round << std::endl;
							}
							if((ch.udp.flags >> SeqTsHeader::FLAG_LAST) & 1){
								last_udp_lost_number++;
							}
							udp_lost_number++;
						}
						else if(ch.l3Prot == 0xFC)
						{
							if(m_RecMode == 0 || m_RecMode == 2)
							{
								std::cout << "ack lost " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
								std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
								std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << ch.ack.sequence << std::endl;
							}
							else if(m_RecMode == 1) //n
							{
								std::cout << "ack lost " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
								std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
								std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << ch.ack.sequence << std::endl;
							}
							if((ch.ack.flags >> qbbHeader::FLAG_LAST) & 1){
								last_ack_lost_number++;
							}
							ack_lost_number++;
						}
						else if(ch.l3Prot == 0xFD)
						{
							if(m_RecMode == 0 || m_RecMode == 2)
							{
								std::cout << "nack lost " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
								std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
								std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << ch.ack.sequence << std::endl;
							}
							else if(m_RecMode == 1)
							{
								std::cout << "nack lost " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
								std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
								std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << ch.ack.sequence << std::endl;
							}
							nack_lost_number++;
						}
						else if(ch.l3Prot == 0xFB)
						{
							if(m_RecMode == 0)
							{
								std::cout << "0xFB exception !" << std::endl;
							}
							else if(m_RecMode == 1)
							{
								std::cout << "notify lost " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
								std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
								std::cout << std::dec << ch.notify.dport << " " << ch.notify.sport << " " << ch.notify.m_notifyseq << " " << ch.notify.m_sequence << " " << unsigned(ch.notify.m_pg) << " " << ch.notify.m_round << std::endl;
							}
							FB_lost_number++;
						}
						return false; //drop
					}
				}
			}
		}
		else if(device->GetChannel()->GetDevice(1) != device)
		{
			Ptr<NetDevice> tmp = device->GetChannel()->GetDevice (1);
			if(tmp->GetNode()->GetNodeType() == 1) //to is sw
			{
				if(ch.l3Prot == 0x11 || ch.l3Prot == 0xFC || ch.l3Prot == 0xFD || ch.l3Prot == 0xFB) //udp and ack and nack
				{
					uint16_t randomnum = rand() % m_lossrate;
					if(randomnum == 0)
					{
						{
							total_lost_number++;
							if(ch.l3Prot == 0x11)
							{
								if(m_RecMode == 0 || m_RecMode == 2)
								{
									std::cout << "data lost " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
									std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
									std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.sequence << std::endl;
								}
								else if(m_RecMode == 1) //n
								{
									std::cout << "data lost " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
									std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
									std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.sequence << " round " << ch.udp.round << std::endl;
								}
								if((ch.udp.flags >> SeqTsHeader::FLAG_LAST) & 1){
									last_udp_lost_number++;
								}
								udp_lost_number++;
							}
							else if(ch.l3Prot == 0xFC)
							{
								if(m_RecMode == 0 || m_RecMode == 2)
								{
									std::cout << "ack lost " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
									std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
									std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << ch.ack.sequence << std::endl;
								}
								else if(m_RecMode == 1) //n
								{
									std::cout << "ack lost " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
									std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
									std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << ch.ack.sequence << std::endl;
								}
								if((ch.ack.flags >> qbbHeader::FLAG_LAST) & 1){
									last_ack_lost_number++;
								}
								ack_lost_number++;
							}
							else if(ch.l3Prot == 0xFD)
							{
								if(m_RecMode == 0  || m_RecMode == 2)
								{
									std::cout << "nack lost " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
									std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
									std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << ch.ack.sequence <<std::endl;
								}
								else if(m_RecMode == 1)
								{
									std::cout << "nack lost " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
									std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
									std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << ch.ack.sequence << std::endl;
								}
								nack_lost_number++;
							}
							else if(ch.l3Prot == 0xFB)
							{
								if(m_RecMode == 0)
								{
									std::cout << "0xFB exception !" << std::endl;
								}
								else if(m_RecMode == 1)
								{
									std::cout << "notify lost " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
									std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
									std::cout << std::dec << ch.notify.dport << " " << ch.notify.sport << " " << ch.notify.m_notifyseq << " " << ch.notify.m_sequence << " " << unsigned(ch.notify.m_pg) << " " << ch.notify.m_round << std::endl;
								}
								FB_lost_number++;
							}
						}
						return false; //drop
					}
				}
			}
		}
	}
	else{ //lossless

	}
	
//*/

/*
	if(GetId() == 3 && device->GetIfIndex() == 1) //设置丢包
	// if((GetId() == 360 && device->GetIfIndex() == 1) ||
	//    (GetId() == 373 && device->GetIfIndex() == 2) || 
	//    (GetId() == 345 && device->GetIfIndex() == 5) ||
	//    (GetId() == 355 && device->GetIfIndex() == 6) ||
	//    (GetId() == 333 && device->GetIfIndex() == 3) ||
	//    (GetId() == 328 && device->GetIfIndex() == 4))
	{
		if(ch.l3Prot == 0x11 || ch.l3Prot == 0xFC || ch.l3Prot == 0xFD) //udp or ack or nack
		//if(ch.l3Prot == 0x11)
		{
			uint16_t randomnum = rand() % 1000;
			if(randomnum == 0)
			//if(((ch.udp.seq == 10000 && lost_seq_udp == 0) || (ch.udp.seq == 10000 && lost_seq_udp == 1) || (ch.udp.seq == 10000 && lost_seq_udp == 2)))
			{
				//lost_seq_udp += 1;
				//0b010b01 0b009401 10000 100
				//if(ch.sip == 0x0b010b01 && ch.dip == 0x0b009401 && ch.udp.sport == 10000 && ch.udp.dport ==100)
				if(ch.l3Prot == 0x11)
				{
					std::cout << Simulator::Now() << " " << "lost udp " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
					std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
					//std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.sequence << " " << ch.udp.seq << std::endl;//" m_round " << ch.udp.round << std::endl;
					std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.sequence << std::endl;
					if((ch.udp.flags >> SeqTsHeader::FLAG_LAST) & 1){
						last_udp_lost_number++;
					}
					udp_lost_number++;
				}
				else if(ch.l3Prot == 0xFC)
				{
					std::cout << Simulator::Now() << " " << "lost ack " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
					std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
					std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << ch.ack.sequence << std::endl;
					if((ch.ack.flags >> qbbHeader::FLAG_LAST) & 1){
						last_ack_lost_number++;
					}
					ack_lost_number++;
				}
				else if(ch.l3Prot == 0xFD)
				{
					std::cout << Simulator::Now() << " " << "lost nack " << std::dec << GetId() << " " << device->GetIfIndex() << " ";
					std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
					std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << ch.ack.sequence << std::endl;
					nack_lost_number++;
				}
				return false; //drop
			}
		}
	}
*/
	
	SendToDev(packet, ch);
	return true;
}

void SwitchNode::SwitchNotifyDequeue(uint32_t ifIndex, uint32_t qIndex, Ptr<Packet> p){
//n
	if(m_RecMode == 1){
	 	//if(GetId() == 2 && ifIndex == 2){ //指定链路
		if(ToIsSwitch(ifIndex)){
			uint8_t* bufe = p->GetBuffer();
			if(bufe[PppHeader::GetStaticSize() + 9] == 0xF9){ //empty data p
				bufe[PppHeader::GetStaticSize() + 21] = (m_seqnumber[ifIndex] >> 8) & 0xff; //ff
				bufe[PppHeader::GetStaticSize() + 20] = m_seqnumber[ifIndex] & 0xff; //ff
				//std::cout << "empty " << m_seqnumber[ifIndex] << std::endl;
				m_seqnumber[ifIndex]++;
				return;
			}
		}
	}
//r
	if(m_RecMode == 2){
		// if(GetId() == 2 && ifIndex == 2){
			// if((GetId() == 340 && ifIndex == 1) ||
			//     (GetId() == 347 && ifIndex == 2) ||
			// 	(GetId() == 324 && ifIndex == 2) ||
			// 	(GetId() == 333 && ifIndex == 4) ||
			// 	(GetId() == 354 && ifIndex == 6) ||
			// 	(GetId() == 351 && ifIndex == 5)){
		if(ToIsSwitch(ifIndex)){
			uint8_t* bufe = p->GetBuffer();
			if(bufe[PppHeader::GetStaticSize() + 9] == 0xF9){ //empty data p
				bufe[PppHeader::GetStaticSize() + 21] = (m_seqnumber[ifIndex] >> 8) & 0xff; //ff
				bufe[PppHeader::GetStaticSize() + 20] = m_seqnumber[ifIndex] & 0xff; //ff
				m_seqnumber[ifIndex]++;
				return;
			}
		}
		// else if(GetId() == 3 && ifIndex == 1){
		// else if((GetId() == 360 && ifIndex == 1) ||
	   	//    	   (GetId() == 373 && ifIndex == 2) || 
	    //        (GetId() == 345 && ifIndex == 5) ||
	    //        (GetId() == 355 && ifIndex == 6) ||
	    //        (GetId() == 333 && ifIndex == 3) ||
	    //        (GetId() == 328 && ifIndex == 4)){
		
		if(ToIsSwitch(ifIndex)){
			uint8_t* bufe = p->GetBuffer();
			if(bufe[PppHeader::GetStaticSize() + 9] == 0xF8){ //empty ack p
				if(m_downPendingAck[ifIndex] == 1){ //should update uplatestRxSeqNo
					//sequence
					bufe[PppHeader::GetStaticSize() + 21] = (m_downlatestRxSeqNo[ifIndex] >> 8) & 0xff; //ff
					bufe[PppHeader::GetStaticSize() + 20] = m_downlatestRxSeqNo[ifIndex] & 0xff; //ff
					//pending flag
					bufe[PppHeader::GetStaticSize() + 23] = (uint16_t(1) >> 8) & 0xff;
					bufe[PppHeader::GetStaticSize() + 22] = uint16_t(1) & 0xff;
					m_downPendingAck[ifIndex] = 0;
				}
				return;
			}
		}
	}

	FlowIdTag t;
	p->PeekPacketTag(t);
	if (qIndex != 0){
		uint32_t inDev = t.GetFlowId();		//std::cout << inDev << std::endl;
		if(inDev != 888) //except those p
		{
			m_mmu->RemoveFromIngressAdmission(inDev, qIndex, p->GetSize());
			m_mmu->RemoveFromEgressAdmission(ifIndex, qIndex, p->GetSize());
			m_bytes[inDev][ifIndex][qIndex] -= p->GetSize();
			if (m_ecnEnabled){
				bool egressCongested = m_mmu->ShouldSendCN(ifIndex, qIndex);
				if (egressCongested){
					PppHeader ppp;
					Ipv4Header h;
					p->RemoveHeader(ppp);
					p->RemoveHeader(h);
					h.SetEcn((Ipv4Header::EcnType)0x03);
					p->AddHeader(h);
					p->AddHeader(ppp);
				}
			}
			//CheckAndSendPfc(inDev, qIndex);
			CheckAndSendResume(inDev, qIndex);
		}	
	}
	if (1){
		uint8_t* buf = p->GetBuffer();
		if (buf[PppHeader::GetStaticSize() + 9] == 0x11){ // udp packet
			//std::cout << MineUdpHeader::GetStaticSize() << std::endl;
			IntHeader *ih = (IntHeader*)&buf[PppHeader::GetStaticSize() + 20 + 8 + 12]; // ppp, ip, udp, SeqTs, INT //8 12
			Ptr<QbbNetDevice> dev = DynamicCast<QbbNetDevice>(m_devices[ifIndex]);
			if (m_ccMode == 3){ // HPCC
				ih->PushHop(Simulator::Now().GetTimeStep(), m_txBytes[ifIndex], dev->GetQueue()->GetNBytesTotal(), dev->GetDataRate().GetBitRate());
			}else if (m_ccMode == 10){ // HPCC-PINT
				uint64_t t = Simulator::Now().GetTimeStep();
				uint64_t dt = t - m_lastPktTs[ifIndex];
				if (dt > m_maxRtt)
					dt = m_maxRtt;
				uint64_t B = dev->GetDataRate().GetBitRate() / 8; //Bps
				uint64_t qlen = dev->GetQueue()->GetNBytesTotal();
				double newU;

				/**************************
				 * approximate calc
				 *************************/
				int b = 20, m = 16, l = 20; // see log2apprx's paremeters
				int sft = logres_shift(b,l);
				double fct = 1<<sft; // (multiplication factor corresponding to sft)
				double log_T = log2(m_maxRtt)*fct; // log2(T)*fct
				double log_B = log2(B)*fct; // log2(B)*fct
				double log_1e9 = log2(1e9)*fct; // log2(1e9)*fct
				double qterm = 0;
				double byteTerm = 0;
				double uTerm = 0;
				if ((qlen >> 8) > 0){
					int log_dt = log2apprx(dt, b, m, l); // ~log2(dt)*fct
					int log_qlen = log2apprx(qlen >> 8, b, m, l); // ~log2(qlen / 256)*fct
					qterm = pow(2, (
								log_dt + log_qlen + log_1e9 - log_B - 2*log_T
								)/fct
							) * 256;
					// 2^((log2(dt)*fct+log2(qlen/256)*fct+log2(1e9)*fct-log2(B)*fct-2*log2(T)*fct)/fct)*256 ~= dt*qlen*1e9/(B*T^2)
				}
				if (m_lastPktSize[ifIndex] > 0){
					int byte = m_lastPktSize[ifIndex];
					int log_byte = log2apprx(byte, b, m, l);
					byteTerm = pow(2, (
								log_byte + log_1e9 - log_B - log_T
								)/fct
							);
					// 2^((log2(byte)*fct+log2(1e9)*fct-log2(B)*fct-log2(T)*fct)/fct) ~= byte*1e9 / (B*T)
				}
				if (m_maxRtt > dt && m_u[ifIndex] > 0){
					int log_T_dt = log2apprx(m_maxRtt - dt, b, m, l); // ~log2(T-dt)*fct
					int log_u = log2apprx(int(round(m_u[ifIndex] * 8192)), b, m, l); // ~log2(u*512)*fct
					uTerm = pow(2, (
								log_T_dt + log_u - log_T
								)/fct
							) / 8192;
					// 2^((log2(T-dt)*fct+log2(u*512)*fct-log2(T)*fct)/fct)/512 = (T-dt)*u/T
				}
				newU = qterm+byteTerm+uTerm;

				#if 0
				/**************************
				 * accurate calc
				 *************************/
				double weight_ewma = double(dt) / m_maxRtt;
				double u;
				if (m_lastPktSize[ifIndex] == 0)
					u = 0;
				else{
					double txRate = m_lastPktSize[ifIndex] / double(dt); // B/ns
					u = (qlen / m_maxRtt + txRate) * 1e9 / B;
				}
				newU = m_u[ifIndex] * (1 - weight_ewma) + u * weight_ewma;
				printf(" %lf\n", newU);
				#endif

				/************************
				 * update PINT header
				 ***********************/
				uint16_t power = Pint::encode_u(newU);
				if (power > ih->GetPower())
					ih->SetPower(power);

				m_u[ifIndex] = newU;
			}	
		}

		if(m_RecMode == 1) //notification
		{
			//if(GetId() == 2 && ifIndex == 2) //指定链路添加序号并保存包头部
			if(ToIsSwitch(ifIndex))
			{
				if(buf[PppHeader::GetStaticSize() + 9] == 0x11) //data
				{
					buf[PppHeader::GetStaticSize() + 20 + 8 + 8] = (m_seqnumber[ifIndex] >> 8) & 0xff; //ff
					buf[PppHeader::GetStaticSize() + 20 + 8 + 9] = m_seqnumber[ifIndex] & 0xff;
					CustomHeader chpc(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					p->PeekHeader(chpc);
					SavePHeader(p, chpc, ifIndex, m_seqnumber[ifIndex]);
					// if(m_onlylastack == 1) //sv
					// {
					// 	if(((buf[PppHeader::GetStaticSize() + 20 + 8 + 7] >> 2) & 1) == 1){ //last
					// 		Ptr<Packet> pc = CopyPacket(p, chpc, ifIndex, m_seqnumber[ifIndex]);
					// 		EnCopyQueue(pc, ifIndex);
					// 	}
					// }
					//std::cout << "data " << chpc.udp.seq << " " << m_seqnumber[ifIndex] << std::endl;
					m_seqnumber[ifIndex]++;
				}
				else if(buf[PppHeader::GetStaticSize() + 9] == 0xFC || buf[PppHeader::GetStaticSize() + 9] == 0xFD) //ack nack
				{
					buf[PppHeader::GetStaticSize() + 20 + 13] = (m_seqnumber[ifIndex] >> 8) & 0xff;
					buf[PppHeader::GetStaticSize() + 20 + 12] = m_seqnumber[ifIndex] & 0xff;
					CustomHeader chpc(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					p->PeekHeader(chpc);
					SavePHeader(p, chpc, ifIndex, m_seqnumber[ifIndex]);
					//std::cout << "ack " << GetId() << " " << m_seqnumber[ifIndex] << std::endl;
					m_seqnumber[ifIndex]++;
				}
				else if(buf[PppHeader::GetStaticSize() + 9] == 0xFB) //notify
				{
					buf[PppHeader::GetStaticSize() + 20 + 12] = (m_seqnumber[ifIndex] >> 8) & 0xff;
					buf[PppHeader::GetStaticSize() + 20 + 11] = m_seqnumber[ifIndex] & 0xff;
					CustomHeader chpc(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					p->PeekHeader(chpc);
					SavePHeader(p, chpc, ifIndex, m_seqnumber[ifIndex]);
					m_seqnumber[ifIndex]++;
				}
			}

			// if(m_onlylastack == 0){
			// 	if(IsTorSwitch()){
			// 		if(buf[PppHeader::GetStaticSize() + 9] == 0xFD && ((buf[PppHeader::GetStaticSize() + 20 + 5] >> 7) & 1) == 1){
			// 			buf[PppHeader::GetStaticSize() + 20 + 5] |= 0x7f; //TOR将交换机产生的NACK标志位 置0
			// 		}
			// 	}
			// }
		}

		else if(m_RecMode == 2) //retransmit
		{
			// if(GetId() == 2 && ifIndex == 2) //指定链路添加序号并缓存副本
			// if((GetId() == 340 && ifIndex == 1) ||
			//     (GetId() == 347 && ifIndex == 2) ||
			// 	(GetId() == 324 && ifIndex == 2) ||
			// 	(GetId() == 333 && ifIndex == 4) ||
			// 	(GetId() == 354 && ifIndex == 6) ||
			// 	(GetId() == 351 && ifIndex == 5))

			if(ToIsSwitch(ifIndex))
			{
				if(buf[PppHeader::GetStaticSize() + 9] == 0x11) //data
				{
					if(((buf[PppHeader::GetStaticSize() + 20 + 8 + 6] >> 1) & 1) != 1) //except retransmit p
					{	
						buf[PppHeader::GetStaticSize() + 20 + 8 + 8] = (m_seqnumber[ifIndex] >> 8) & 0xff; //ff
						buf[PppHeader::GetStaticSize() + 20 + 8 + 9] = m_seqnumber[ifIndex] & 0xff;
						CustomHeader chpc(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
				    	p->PeekHeader(chpc);
						//std::cout << "origin udp " << chpc.udp.sequence << std::endl;
						Ptr<Packet> pc = CopyPacket(p, chpc, ifIndex, m_seqnumber[ifIndex]); //all udp packet
						EnCopyQueue(pc, ifIndex);
						m_seqnumber[ifIndex]++;
					}
				}
				else if(buf[PppHeader::GetStaticSize() + 9] == 0xFC || buf[PppHeader::GetStaticSize() + 9] == 0xFD) //ack
				{
					if(((buf[PppHeader::GetStaticSize() + 20 + 5] >> 1) & 1) != 1) //except retransmit p
					{
						buf[PppHeader::GetStaticSize() + 20 + 13] = (m_seqnumber[ifIndex] >> 8) & 0xff;
						buf[PppHeader::GetStaticSize() + 20 + 12] = m_seqnumber[ifIndex] & 0xff;
						CustomHeader chpc(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
						p->PeekHeader(chpc);
						Ptr<Packet> pc = CopyPacket(p, chpc, ifIndex, m_seqnumber[ifIndex]);
						EnCopyQueue(pc, ifIndex);
						m_seqnumber[ifIndex]++;
					}
				}
			}
			// else if(GetId() == 3 && ifIndex == 1) //指定链路给normal packets(data,ack)加上m_downlatestRxSeqNo
			// else if((GetId() == 360 && ifIndex == 1) ||
	   	   	//    (GetId() == 373 && ifIndex == 2) || 
	        //    (GetId() == 345 && ifIndex == 5) ||
	        //    (GetId() == 355 && ifIndex == 6) ||
	        //    (GetId() == 333 && ifIndex == 3) ||
	        //    (GetId() == 328 && ifIndex == 4))

			if(ToIsSwitch(ifIndex))
			{
				// if(GetId() == 373 && ifIndex == 2){
				// 	std::cout << "herehhh " <<  m_downlatestRxSeqNo[ifIndex] << " " << m_downPendingAck[ifIndex] << std::endl;
				// }
				//std::cout << "here " << m_downlatestRxSeqNo[ifIndex] << std::endl;
				if(buf[PppHeader::GetStaticSize() + 9] == 0x11) //data
				{
					if(m_downPendingAck[ifIndex] == 1 && (((buf[PppHeader::GetStaticSize() + 20 + 8 + 6] >> 1) & 1) != 1)) //should update uplatestRxSeqNo
					{
						// if(GetId() == 373 && ifIndex == 2){
						// 	std::cout << "here " <<  m_downlatestRxSeqNo[ifIndex] << std::endl;
						// }
						// std::cout << Simulator::Now() << " " << "send data normal p " << std::dec << GetId() << " " << ifIndex << " ";
						// std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
						// std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << m_uplatestRxSeqNo[inDev] << std::endl;

						buf[PppHeader::GetStaticSize() + 20 + 8 + 10] = (m_downlatestRxSeqNo[ifIndex] >> 8) & 0xff; //ff
						buf[PppHeader::GetStaticSize() + 20 + 8 + 11] = m_downlatestRxSeqNo[ifIndex] & 0xff;
						
						buf[PppHeader::GetStaticSize() + 20 + 8 + 6] |= (uint8_t(1) & 0xff); //pending_flag
						m_downPendingAck[ifIndex] = 0;
					}
				}
				else if(buf[PppHeader::GetStaticSize() + 9] == 0xFC || buf[PppHeader::GetStaticSize() + 9] == 0xFD) //ack
				{
					if(m_downPendingAck[ifIndex] == 1 && (((buf[PppHeader::GetStaticSize() + 20 + 5] >> 1) & 1) != 1)) //should update uplatestRxSeqNo
					{
						// if(GetId() == 373 && ifIndex == 2){
						// 	std::cout << "here " <<  m_downlatestRxSeqNo[ifIndex] << std::endl;
						// }
						buf[PppHeader::GetStaticSize() + 20 + 15] = (m_downlatestRxSeqNo[ifIndex] >> 8) & 0xff;
						buf[PppHeader::GetStaticSize() + 20 + 14] = m_downlatestRxSeqNo[ifIndex] & 0xff;

						buf[PppHeader::GetStaticSize() + 20 + 5] |= (uint8_t(1) & 0xff); //pending_flag
						m_downPendingAck[ifIndex] = 0;
					}
				}
			}
		}
	}
	m_txBytes[ifIndex] += p->GetSize();
	m_lastPktSize[ifIndex] = p->GetSize();
	m_lastPktTs[ifIndex] = Simulator::Now().GetTimeStep();
}

int SwitchNode::logres_shift(int b, int l){
	static int data[] = {0,0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5};
	return l - data[b];
}

int SwitchNode::log2apprx(int x, int b, int m, int l){
	int x0 = x;
	int msb = int(log2(x)) + 1;
	if (msb > m){
		x = (x >> (msb - m) << (msb - m));
		#if 0
		x += + (1 << (msb - m - 1));
		#else
		int mask = (1 << (msb-m)) - 1;
		if ((x0 & mask) > (rand() & mask))
			x += 1<<(msb-m);
		#endif
	}
	return int(log2(x) * (1<<logres_shift(b, l)));
}

void SwitchNode::AddHeader (Ptr<Packet> p, uint16_t protocolNumber){
	PppHeader ppp;
	ppp.SetProtocol (EtherToPpp (protocolNumber));
	p->AddHeader (ppp);
}
uint16_t SwitchNode::EtherToPpp (uint16_t proto){
	switch(proto){
		case 0x0800: return 0x0021;   //IPv4
		case 0x86DD: return 0x0057;   //IPv6
		default: NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
	}
	return 0;
}

uint16_t SwitchNode::GeSeqnumber(uint16_t PortIndex)
{
	return m_seqnumber[PortIndex];
}

uint16_t SwitchNode::GetPendingack(uint16_t PortIndex)
{
	return m_downPendingAck[PortIndex];
}
///*
Ptr<Queue> SwitchNode::GetQueue (void) const
{ 
    return m_copyqueue;
}

void SwitchNode::SetQueue (Ptr<Queue> q)
{
  	m_copyqueue = q;
}

Ptr<Packet> SwitchNode::CopyPacket(Ptr<Packet> p, CustomHeader &ch, uint16_t pidx1, uint16_t sequence1) //要区分交换机与交换机之间的链路
{
	if(ch.l3Prot == 0x11) //udp
	{
		uint32_t payload_size = p->GetSize() - ch.GetSerializedSize(); //payload
		//std::cout << "payload_size " << payload_size << std::endl;
		Ptr<Packet> pc = Create<Packet> (payload_size);
		SeqTsHeader seqTs;
		seqTs.SetSeq (ch.udp.seq);
		seqTs.SetPG (ch.udp.pg);
		if((ch.udp.flags >> SeqTsHeader::FLAG_LAST) & 1) //last p
		{
			seqTs.SetLast();
		}
		else if((ch.udp.flags >> SeqTsHeader::FLAG_FIRST) & 1) //first p
		{
			seqTs.SetFirst();
		}
		else if((ch.udp.flags >> SeqTsHeader::FLAG_MIDDLE) & 1) //middle p
		{
			seqTs.SetMiddle();
		}
		seqTs.SetCopyRecirculate(); //set
		seqTs.SetSequence (ch.udp.sequence); //retransmit
		seqTs.SetRound (ch.udp.round); //useless
		pc->AddHeader(seqTs);
		UdpHeader udpHeader;
		udpHeader.SetDestinationPort (ch.udp.dport);
		udpHeader.SetSourcePort (ch.udp.sport);
		pc->AddHeader (udpHeader);
		Ipv4Header ipHeader;
		ipHeader.SetSource (Ipv4Address(ch.sip));
		ipHeader.SetDestination (Ipv4Address(ch.dip));
		ipHeader.SetProtocol (0x11);
		ipHeader.SetPayloadSize (pc->GetSize());
		ipHeader.SetTtl (64);
		ipHeader.SetTos (0);
		ipHeader.SetIdentification (ch.ipid);
		pc->AddHeader(ipHeader);
		PppHeader ppp;
		ppp.SetProtocol (0x0021); // EtherToPpp(0x800), see point-to-point-net-device.cc
		pc->AddHeader (ppp);
		return pc;
	}
	else if(ch.l3Prot == 0xFC || ch.l3Prot == 0xFD)
	{
		uint32_t payload_size = p->GetSize() - ch.GetSerializedSize(); //payload
		Ptr<Packet> pc = Create<Packet> (payload_size);
		qbbHeader ackh;
		ackh.SetSeq(ch.ack.seq);
		ackh.SetPG(ch.ack.pg);
		ackh.SetSport(ch.ack.sport);
		ackh.SetDport(ch.ack.dport);
		if((ch.ack.flags >> qbbHeader::FLAG_CNP) & 1) //CNP
		{
			ackh.SetCnp();
		}
		if((ch.ack.flags >> qbbHeader::FLAG_LAST) & 1) //LAST
		{
			ackh.SetLast();
		}
		ackh.SetCopyRecirculate();
		ackh.SetSequence (ch.ack.sequence); //retransmit
		pc->AddHeader(ackh);
		Ipv4Header ipHeader;
		ipHeader.SetSource (Ipv4Address(ch.sip));
		ipHeader.SetDestination (Ipv4Address(ch.dip));
		ipHeader.SetProtocol (ch.l3Prot);
		ipHeader.SetPayloadSize (pc->GetSize());
		ipHeader.SetTtl (64);
		ipHeader.SetIdentification (ch.ipid);
		pc->AddHeader(ipHeader);
		AddHeader(pc, 0x0800);
		return pc;
	}
}

void SwitchNode::SavePHeader(Ptr<Packet>p, CustomHeader &ch, uint16_t pidx, uint16_t sequence)
{
	uint16_t index = sequence % m_lookuptablesize; //look up table size = m_lookuptablesize
	if(ch.l3Prot == 0x11) //udp
	{
		lookuptable[pidx][index].sport = ch.udp.sport;
		lookuptable[pidx][index].dport = ch.udp.dport;
		lookuptable[pidx][index].sip = ch.sip;
		lookuptable[pidx][index].dip = ch.dip;
		lookuptable[pidx][index].protocol = ch.l3Prot;
		lookuptable[pidx][index].seq = ch.udp.seq;
		lookuptable[pidx][index].pg = ch.udp.pg;
		lookuptable[pidx][index].round = ch.udp.round;
		lookuptable[pidx][index].flag = ch.udp.flags;
		lookuptable[pidx][index].size = p->GetSize() - ch.GetSerializedSize(); //payload size
	}
	else if(ch.l3Prot == 0xFC) //ack
	{
		lookuptable[pidx][index].sport = ch.ack.sport;
		lookuptable[pidx][index].dport = ch.ack.dport;
		lookuptable[pidx][index].sip = ch.sip;
		lookuptable[pidx][index].dip = ch.dip;
		lookuptable[pidx][index].protocol = ch.l3Prot;
		lookuptable[pidx][index].seq = ch.ack.seq;
		lookuptable[pidx][index].pg = ch.ack.pg;
		lookuptable[pidx][index].round = 0;
		lookuptable[pidx][index].flag = ch.ack.flags;
	}
	else if(ch.l3Prot == 0xFD) //ack nack
	{
		lookuptable[pidx][index].sport = ch.ack.sport;
		lookuptable[pidx][index].dport = ch.ack.dport;
		lookuptable[pidx][index].sip = ch.sip;
		lookuptable[pidx][index].dip = ch.dip;
		lookuptable[pidx][index].protocol = ch.l3Prot;
		lookuptable[pidx][index].seq = ch.ack.seq;
		lookuptable[pidx][index].pg = ch.ack.pg;
		lookuptable[pidx][index].round = ch.ack.latestRxSeqNo; //latestRxSeqNo当作序号
		lookuptable[pidx][index].flag = ch.ack.flags;
	}
	else if(ch.l3Prot == 0xFB) //notify
	{
		lookuptable[pidx][index].sport = ch.notify.sport;
		lookuptable[pidx][index].dport = ch.notify.dport;
		lookuptable[pidx][index].sip = ch.sip;
		lookuptable[pidx][index].dip = ch.dip;
		lookuptable[pidx][index].protocol = ch.l3Prot;
		lookuptable[pidx][index].seq = ch.notify.m_notifyseq;
		lookuptable[pidx][index].pg = ch.notify.m_pg;
		lookuptable[pidx][index].round = ch.notify.m_round;
		lookuptable[pidx][index].flag = 0;
	}
	return;
}

Ptr<Packet> SwitchNode::ReplicateDataPacket(Ptr<Packet> p, CustomHeader &ch){
	uint32_t payload_size = p->GetSize() - ch.GetSerializedSize(); //payload
	Ptr<Packet> pc = Create<Packet> (payload_size);
	SeqTsHeader seqTs;
	seqTs.SetSeq (ch.udp.seq);
	seqTs.SetPG (ch.udp.pg);
	if((ch.udp.flags >> SeqTsHeader::FLAG_LAST) & 1) //last p
	{
		seqTs.SetLast();
	}
	else if((ch.udp.flags >> SeqTsHeader::FLAG_FIRST) & 1) //first p
	{
		seqTs.SetFirst();
	}
	else if((ch.udp.flags >> SeqTsHeader::FLAG_MIDDLE) & 1) //middle p
	{
		seqTs.SetMiddle();
	}
	seqTs.SetRound(ch.udp.round);
	seqTs.SetNotDataFilter();
	pc->AddHeader(seqTs);
	UdpHeader udpHeader;
	udpHeader.SetDestinationPort (ch.udp.dport);
	udpHeader.SetSourcePort (ch.udp.sport);
	pc->AddHeader (udpHeader);
	Ipv4Header ipHeader;
	ipHeader.SetSource (Ipv4Address(ch.sip));
	ipHeader.SetDestination (Ipv4Address(ch.dip));
	ipHeader.SetProtocol (0x11);
	ipHeader.SetPayloadSize (pc->GetSize());
	ipHeader.SetTtl (64);
	ipHeader.SetTos (0);
	ipHeader.SetIdentification (ch.ipid);
	pc->AddHeader(ipHeader);
	PppHeader ppp;
	ppp.SetProtocol (0x0021); // EtherToPpp(0x800), see point-to-point-net-device.cc
	pc->AddHeader (ppp);
	return pc;
}


int SwitchNode::CheckInOrderRecoverySeq(uint16_t ackNo, uint16_t p_seq)
{
	uint16_t counter_era = (ackNo >> 15) & 0xff;
	uint16_t sequence_era = (p_seq >> 15) & 0xff;
	uint16_t counter_value = ackNo & 0x7fff;
	uint16_t sequence_value = p_seq & 0x7fff;
	if(counter_era == sequence_era){
		if(sequence_value == counter_value){
			return 1;
		}else if(sequence_value > counter_value){
			return 2;
		}else if(sequence_value < counter_value){ //dup
			return 3;
		}
	}
	else if(counter_era != sequence_era){
		if(sequence_value < counter_value){
			return 2;
		}else if(sequence_value > counter_value){ //dup
			return 3;
		}else if(sequence_value == counter_value){
			std::cout << "why" << std::endl;
			return 4;
		}
	}
}

int SwitchNode::CopyCheck(uint16_t p_seq, uint16_t upackNo, uint16_t port){
	uint16_t counter_era = (upackNo >> 15) & 0xff;
	uint16_t sequence_era = (p_seq >> 15) & 0xff;
	uint16_t counter_value = upackNo & 0x7fff;
	uint16_t sequence_value = p_seq & 0x7fff;
	// if(GetId() == 347 && port == 2)
	// {
	// 	std::cout << "issue " << p_seq << " " << upackNo << std::endl;
	// }
	if(counter_era == sequence_era){
		if(sequence_value > counter_value){ //不知道是否成功被下游接收到
			return 1;
		}else if(sequence_value <= counter_value){
			return 2;
		}
	}
	else if(counter_era != sequence_era){
		if(sequence_value < counter_value){ //不知道是否成功被下游接收到
			return 1;
		}else if(sequence_value > counter_value){
			return 2;
		}else if(sequence_value == counter_value){
			std::cout << "why " << GetId() << " " << port << " " << p_seq << " " << upackNo << std::endl;
			return 4;
		}
	}
}

int SwitchNode::ReorderingCheck(uint16_t p_seq, uint16_t downackNo){
	uint16_t counter_era = (downackNo >> 15) & 0xff;
	uint16_t sequence_era = (p_seq >> 15) & 0xff;
	uint16_t counter_value = downackNo & 0x7fff;
	uint16_t sequence_value = p_seq & 0x7fff;
	if(counter_era == sequence_era){
		if(sequence_value == counter_value){ //expected p
			return 1;
		}else if(sequence_value > counter_value){ //unexpected p
			return 2;
		}else if(sequence_value < counter_value){ //de-duplication
			return 3;
		}
	}
	else if(counter_era != sequence_era){
		if(sequence_value < counter_value){ //unexpected p
			return 2;
		}else if(sequence_value > counter_value){ //de-duplication
			return 3;
		}else if(sequence_value == counter_value){
			std::cout << "why" << std::endl;
			return 4;
		}
	}
}

bool SwitchNode::SeqNoInReTxReq(uint16_t seqno, uint16_t port)
{
	auto it = std::find(m_reTxReqs[port].begin(), m_reTxReqs[port].end(), seqno);
	if(it != m_reTxReqs[port].end()) //find seqno
	{
		m_reTxReqs[port].erase(it); //clear
		return true;
	}
	else{
		return false;
	}
}

bool SwitchNode::CopyRecirculateTransmitStart(Ptr<Packet> p, uint16_t port,  bool a){
	m_CopyrecirculatetxMachineState[port] = Copy_BUSY;
	Time txTime = NanoSeconds(p->GetSize() * 8.0 / 100);
	Simulator::Schedule(txTime, &SwitchNode::CopyRecirculateTransmitComplete, this, port);
	if(a){
		Time recirTime = txTime + NanoSeconds(500);
		Simulator::Schedule(recirTime, &SwitchNode::EnCopyQueue, this, p, port);
	}
}
	
void SwitchNode::CopyRecirculateTransmitComplete(uint16_t port){
	m_CopyrecirculatetxMachineState[port] = Copy_READY;
	CopyDequeue(port);
}

void SwitchNode::CopyDequeue(uint16_t port){
	if(m_CopyrecirculatetxMachineState[port] == Copy_BUSY){
		return;
	}
	Ptr<Packet> p = m_copyqueues[port]->Dequeue();
	if(p != 0)
	{
		m_copysize[port] -= p->GetSize();

		CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
		p->PeekHeader(ch);
		if(ch.l3Prot == 0x11) //udp
		{
			// if(Simulator::Now().GetTimeStep() > 2005278207){
			// 	std::cout << Simulator::Now() << " " << "copy recirulate data " << std::dec << GetId() << " " << port << " ";
			// 	std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
			// 	std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.sequence << " " << m_uplatestRxSeqNo[port]<< std::endl;
			// }
			int re = CopyCheck(ch.udp.sequence, m_uplatestRxSeqNo[port], port);
			if(!start_state[port] || re == 1) //不确定是否成功被下游接收到，进队列继续循环
			{ 
				CopyRecirculateTransmitStart(p, port, true);
			}
			else if(start_state[port] && re == 2)
			{
				//CopyRecirculateTransmitStart(p, port, false);
				//m_copysize -= p->GetSize();
				//std::cout << "... " << ch.udp.seq << " " << ch.udp.sequence << std::endl;std::cout << "... " << ch.udp.seq << " " << ch.udp.sequence << std::endl;
				if(SeqNoInReTxReq(ch.udp.sequence, port)) //丢包
				{
					//出队列，并且retransmit locally
					std::cout << Simulator::Now() << " " << "copyqueue retransmit data " << std::dec << GetId() << " " << port << " ";
					std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
					std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.sequence << std::endl;
					CustomHeader chrp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					p->PeekHeader(chrp);
					uint32_t qIndex = 0;
					p->AddPacketTag(FlowIdTag(888));
					m_devices[port]->SwitchSend(qIndex, p, chrp);

					// dup 1
					Ptr<Packet> pc1 = CopyPacket(p, chrp, 1, 1);
					CustomHeader ch1(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					pc1->PeekHeader(ch1);
					m_devices[port]->SwitchSend(qIndex, pc1, ch1);

					// dup 2
					// Ptr<Packet> pc2 = CopyPacket(p, chrp, 1, 1);
					// CustomHeader ch2(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					// pc2->PeekHeader(ch2);
					// m_devices[port]->SwitchSend(qIndex, pc2, ch2);
					//return;
				}
				else{ //已经确定被成功接收，不循环，直接从循环端口出去丢掉
					//std::cout << "remove from copy queue " << Simulator::Now() << " " << ch.udp.seq << " " << ch.udp.sequence << std::endl;
					//return;
				}
				CopyDequeue(port);
			}
		}
		else if(ch.l3Prot == 0xFC || ch.l3Prot == 0xFD) //ack nack
		{
			// if(Simulator::Now().GetTimeStep() > 2005278207){
			// 	std::cout << Simulator::Now() << " " << "copy recirulate ack " << std::dec << GetId() << " " << port << " ";
			// 	std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
			// 	std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << ch.ack.sequence << " " << m_uplatestRxSeqNo[port] << std::endl;
			// }
			int re = CopyCheck(ch.ack.sequence, m_uplatestRxSeqNo[port], port);
			if(!start_state[port] || re == 1) //不确定是否成功被下游接收到，进队列继续循环
			{
				CopyRecirculateTransmitStart(p, port, true);
			}
			else if(start_state[port] && re == 2)
			{
				if(SeqNoInReTxReq(ch.ack.sequence, port)) //丢包
				{
					//出队列，并且retransmit locally
					std::cout << Simulator::Now() << " " << "copyqueue retransmit ack " << std::dec << GetId() << " " << port << " ";
					std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
					std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << ch.ack.sequence << std::endl;
					CustomHeader chrp(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					p->PeekHeader(chrp);
					uint32_t qIndex = 0;
					p->AddPacketTag(FlowIdTag(888));
					m_devices[port]->SwitchSend(qIndex, p, chrp);

					// dup 1
					Ptr<Packet> pc1 = CopyPacket(p, chrp, 1, 1);
					CustomHeader ch1(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					pc1->PeekHeader(ch1);
					m_devices[port]->SwitchSend(qIndex, pc1, ch1);

					// dup 2
					// Ptr<Packet> pc2 = CopyPacket(p, chrp, 1, 1);
					// CustomHeader ch2(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
					// pc2->PeekHeader(ch2);
					// m_devices[port]->SwitchSend(qIndex, pc2, ch2);
					//return;
				}
				else{ //已经确定被成功接收，不循环，直接从循环端口出去丢掉
					//std::cout << "remove from copy queue " << Simulator::Now() << " " << ch.udp.seq << " " << ch.udp.sequence << std::endl;
					//return;
				}
				CopyDequeue(port);
			}
		}
	}
	return;
}

void SwitchNode::EnCopyQueue(Ptr<Packet> p, uint16_t port)
{
	CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
	p->PeekHeader(ch);
	//std::cout << Simulator::Now() << " " << "copy enqueue udp " << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.sequence << std::endl;

	m_copyqueues[port]->Enqueue(p);
	m_copysize[port] += p->GetSize();
	if(m_copysize[port] > max_copysize[port])
	{
		max_copysize[port] = m_copysize[port];
	}

	if(m_RecMode == 2){
		CopyDequeue(port);
	}else{

	}
}

// Ptr<Packet> SwitchNode::GetDequeueReorderingPacket(uint16_t port){
// 	Ptr<Packet> p = m_reorderingqueues[port]->Dequeue();
// 	m_reorderingsize -= p->GetSize();
// 	return p;
// }
bool SwitchNode::ReorderingRecirculateTransmitStart(Ptr<Packet> p, uint16_t port){
	m_ReorderingrecirculatetxMachineState[port] = Reordering_BUSY;
	Time txTime = NanoSeconds(p->GetSize() * 8.0 / 100);
	Simulator::Schedule(txTime, &SwitchNode::ReorderingRecirculateTransmitComplete, this, port);
	Time recirTime = txTime + NanoSeconds(500);
	Simulator::Schedule(recirTime, &SwitchNode::RecirculateReceive, this, p, port);
	return true;
}

void SwitchNode::ReorderingRecirculateTransmitComplete(uint16_t port){
	m_ReorderingrecirculatetxMachineState[port] = Reordering_READY;
	ReorderingDequeue(port);
}

void SwitchNode::ReorderingDequeue(uint16_t port)
{
	if(m_ReorderingrecirculatetxMachineState[port] == Reordering_BUSY){
		return;
	}
	Ptr<Packet> p = m_reorderingqueues[port]->Dequeue();
	if(p != 0)
	{
		m_reorderingsize[port] -= p->GetSize();
		if(m_reorderingsize[port] <= resume_threshold && m_curr_state[port] == 1)
		{
			//send_resume();
			Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[port]);
			device->SendPauseToUp(0, 3);
			m_curr_state[port] = 0;  //resume
		}
		ReorderingRecirculateTransmitStart(p, port);
		// Time txTime = NanoSeconds(p->GetSize() * 8.0 / 100);
		// Simulator::Schedule(txTime, &SwitchNode::ReorderingDequeue, this, port); //next p dequeue
		// CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
		// p->PeekHeader(ch);

		// if(ch.l3Prot == 0x11) //udp
		// {
		// 	std::cout << "dequeue reordering udp p " << Simulator::Now() << ch.udp.seq << " " << ch.udp.sequence << std::endl;
		// 	uint8_t* buf = p->GetBuffer();
		// 	int re = ReorderingCheck(ch.udp.sequence, m_ackNo[port]);
		// 	if(re == 1){
		// 		m_recirculatetxMachineState = BUSY;
		// 		buf[PppHeader::GetStaticSize() + 20 + 8 + 6] &= ~(1 << 2); //reordering_re = 0

		// 		std::cout << "reordering queue find expected normal data " << Simulator::Now() << " " << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.sequence << " " << m_ackNo[port] << std::endl;
		// 		// std::cout << "reordering queue find expected normal data " << std::dec << GetId() << " " << port << " ";
		// 		// std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " ";
		// 		// std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << std::endl;
		// 		int idx = GetOutDev(p, ch); //out port
		// 		uint32_t qIndex = ch.udp.pg;
		// 		// FlowIdTag t;
		// 		// p->RemovePacketTag(t);
		// 		//p->AddPacketTag(FlowIdTag(888));
		// 		m_devices[idx]->SwitchSend(qIndex, p, ch);

		// 		m_ackNo[port]++;

		// 		m_recirculatetxMachineState = READY;
		// 		ReorderingDequeue(port);
		// 		return;
		// 	}
		// 	else if(re == 2){
		// 		RecirculateTransmitStart(p, port);
		// 		//std::cout << "reordering queue another time normal data " << Simulator::Now() << " " << ch.udp.seq << " " << ch.udp.sequence << " " << m_ackNo[port] << std::endl;
		// 	// 	Time recirTime = txTime + NanoSeconds(500);
		// 	// 	Simulator::Schedule(recirTime, &SwitchNode::EnReorderingQueue, this, p, port);
		// 	}
		// 	else if(re == 3){
		// 		m_recirculatetxMachineState = BUSY;
		// 		// m_reorderingsize -= p->GetSize();
		// 		// if(m_reorderingsize <= 37888 && m_curr_state[port] == 1)
		// 		// {
		// 		// 	//send_resume();
		// 		// 	Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[port]);
		// 		// 	device->SendPauseToUp(0, 3);
		// 		// 	m_curr_state[port] = 0;  //resume
		// 		// }
		// 		//de-duplication
		// 		m_recirculatetxMachineState = READY;
		// 		ReorderingDequeue(port);
		// 		return;
		// 	}
		// }
		// else if(ch.l3Prot == 0xF9){
		// 	std::cout << "dequeue reordering empty p " << Simulator::Now() << " " << ch.empty.empty_seq << std::endl;
		// 	int re = ReorderingCheck(ch.empty.empty_seq, m_ackNo[port]);
		// 	if(re == 1){
		// 		m_recirculatetxMachineState = BUSY;
		// 		// m_reorderingsize -= p->GetSize();
		// 		// if(m_reorderingsize <= 37888 && m_curr_state[port] == 1)
		// 		// {
		// 		// 	//send_resume();
		// 		// 	Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[port]);
		// 		// 	device->SendPauseToUp(0, 3);
		// 		// 	m_curr_state[port] = 0;  //resume
		// 		// }
		// 		std::cout << "reordering queue find expected empty data " << Simulator::Now() << " " << ch.empty.empty_seq << " " << m_ackNo[port] << std::endl;
		// 		m_ackNo[port]++;
		// 		m_recirculatetxMachineState = READY;
		// 		ReorderingDequeue(port);
		// 		return;
		// 	}
		// 	else if(re == 2){
		// 		//std::cout << "reordering queue another time empty data " << Simulator::Now() << " " << ch.empty.empty_seq << " " << m_ackNo[port] << std::endl;
		// 		// Time recirTime = txTime + NanoSeconds(500);
		// 		// Simulator::Schedule(recirTime, &SwitchNode::EnReorderingQueue, this, p, port);
		// 		RecirculateTransmitStart(p, port);
		// 	}
		// 	else if(re == 3){
		// 		m_recirculatetxMachineState = BUSY;
		// 		// m_reorderingsize -= p->GetSize();
		// 		// if(m_reorderingsize <= 37888 && m_curr_state[port] == 1)
		// 		// {
		// 		// 	//send_resume();
		// 		// 	Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[port]);
		// 		// 	device->SendPauseToUp(0, 3);
		// 		// 	m_curr_state[port] = 0;  //resume
		// 		// }
		// 		m_recirculatetxMachineState = READY;
		// 		ReorderingDequeue(port);
		// 		return;
		// 	}
		// }
	}
	return;
}
void SwitchNode::EnReorderingQueue(Ptr<Packet> p, uint16_t port){
	CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
	p->PeekHeader(ch);
	// if(ch.l3Prot == 0x11){
	// 	std::cout <<  Simulator::Now() << " " << "reordering enqueue udp p " << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.sequence << std::endl;
	// }else if(ch.l3Prot == 0xF9){
	// 	std::cout <<  Simulator::Now() << " " << "reordering enqueue [empty] p " << ch.empty.empty_seq << std::endl;
	// }
	
	m_reorderingqueues[port]->Enqueue(p);
	m_reorderingsize[port] += p->GetSize();

	if(m_RecMode == 2 && GetId() == 3 && port == 1)
	{
		// std::ofstream fct_output_finish("small_30load_reordering_buffer_size.size", std::ofstream::app);
		// fct_output_finish << Simulator::Now() << " " << m_reorderingsize / 1024.0 << std::endl;
		// fct_output_finish.close();
	}

	//std::cout << "reordering size " << m_reorderingsize << " " << m_reorderingqueues[port]->GetNPackets() << " " << m_reorderingqueues[port]->GetNBytes() << std::endl;
	if(m_reorderingsize[port] >= pause_threshold && m_curr_state[port] == 0)
	{
		//send_pause();
		Ptr<QbbNetDevice> device = DynamicCast<QbbNetDevice>(m_devices[port]);
		device->SendPauseToUp(1, 3);
		m_curr_state[port] = 1; //pause
	}
	//更新max_reorderingsize
	if(m_reorderingsize[port] > max_reorderingsize[port])
	{
		max_reorderingsize[port] = m_reorderingsize[port];
	}

	ReorderingDequeue(port);
}

void SwitchNode::RecirculateReceive(Ptr<Packet> p, uint16_t port){
	CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
	p->PeekHeader(ch);
// recirculate p
	if(ch.l3Prot == 0x11){ //udp
		//std::cout << "dequeue reordering udp p " << Simulator::Now() << ch.udp.seq << " " << ch.udp.sequence << std::endl;
		uint8_t* buf = p->GetBuffer();
		int re = ReorderingCheck(ch.udp.sequence, m_ackNo[port]);
		if(re == 1){
			buf[PppHeader::GetStaticSize() + 20 + 8 + 6] &= ~(1 << 2); //reordering_re = 0

			//std::cout << Simulator::Now() << " " << "reordering queue find expected normal data " << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.sequence << " " << m_ackNo[port] << std::endl;

			int idx = GetOutDev(p, ch); //out port
			uint32_t qIndex = ch.udp.pg;
			FlowIdTag t;
			p->PeekPacketTag(t);
			uint32_t inDev = t.GetFlowId();
			if (qIndex != 0){ //not highest priority, and noe lowest priority
				if (m_mmu->CheckIngressAdmission(inDev, qIndex, p->GetSize()) && m_mmu->CheckEgressAdmission(idx, qIndex, p->GetSize())){			// Admission control
					m_mmu->UpdateIngressAdmission(inDev, qIndex, p->GetSize());
					m_mmu->UpdateEgressAdmission(idx, qIndex, p->GetSize());
				}else{
					std::cout << "congestion drop" << std::endl;
					return; // Drop
				}
				CheckAndSendPfc(inDev, qIndex);
			}
			m_bytes[inDev][idx][qIndex] += p->GetSize();

			m_devices[idx]->SwitchSend(qIndex, p, ch);

			m_ackNo[port]++;
			return;
		}
		else if(re == 2){
			EnReorderingQueue(p, port);
			return;
		}
		else if(re == 3){ //de-duplication
			return;
		}
	}else if(ch.l3Prot == 0xF9){
		int re = ReorderingCheck(ch.empty.empty_seq, m_ackNo[port]);
		if(re == 1){
			//std::cout << Simulator::Now() << " " << "reordering queue find expected [empty] data " << ch.empty.empty_seq << " " << m_ackNo[port] << std::endl;
			m_ackNo[port]++;
			return;
		}else if(re == 2){
			EnReorderingQueue(p, port);
			return;
		}
		else if(re == 3){
			return;
		}
	}else if(ch.l3Prot == 0xFC || ch.l3Prot == 0xFD){
		uint8_t* buf = p->GetBuffer();
		int re = ReorderingCheck(ch.ack.sequence, m_ackNo[port]);
		if(re == 1){
			buf[PppHeader::GetStaticSize() + 20 + 5] &= ~(1 << 2); //reordering_re = 0

			//std::cout << Simulator::Now() << " " << "reordering queue find expected normal ack " << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << ch.ack.sequence << " " << m_ackNo[port] << std::endl;

			int idx = GetOutDev(p, ch); //out port
			uint32_t qIndex = ch.ack.pg;
			FlowIdTag t;
			p->PeekPacketTag(t);
			uint32_t inDev = t.GetFlowId();
			if (qIndex != 0){ //not highest priority, and noe lowest priority
				if (m_mmu->CheckIngressAdmission(inDev, qIndex, p->GetSize()) && m_mmu->CheckEgressAdmission(idx, qIndex, p->GetSize())){			// Admission control
					m_mmu->UpdateIngressAdmission(inDev, qIndex, p->GetSize());
					m_mmu->UpdateEgressAdmission(idx, qIndex, p->GetSize());
				}else{
					std::cout << "congestion drop" << std::endl;
					return; // Drop
				}
				CheckAndSendPfc(inDev, qIndex);
			}
			m_bytes[inDev][idx][qIndex] += p->GetSize();

			m_devices[idx]->SwitchSend(qIndex, p, ch);

			m_ackNo[port]++;
			return;
		}
		else if(re == 2){
			EnReorderingQueue(p, port);
			return;
		}
		else if(re == 3){ //de-duplication
			return;
		}
	}
	return;
}
// void SwitchNode::ReorderingPacketRecirculate(uint16_t port){
// 	Ptr<Packet> p = GetDequeueReorderingPacket(port);
// 	CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
// 	p->PeekHeader(ch);
// 	if(ch.l3Prot == 0x11) //udp
// 	{
// 		if(ch.udp.sequence == m_ackNo[port]){
// 			std::cout << "reordering recirculate find expected normal data " << Simulator::Now() << " " << ch.udp.seq << " " << ch.udp.sequence << " " << m_ackNo[port] << std::endl;
// 			int idx = GetOutDev(p, ch); //out port
// 			uint32_t qIndex = ch.udp.pg;
// 			//p->AddPacketTag(FlowIdTag(888));
// 			m_devices[idx]->SwitchSend(qIndex, p, ch);

// 			m_ackNo[port]++;
// 		}
// 		else if(ch.udp.sequence > m_ackNo[port]){
// 			std::cout << "reordering recirculate another time normal data " << Simulator::Now() << " " << ch.udp.seq << " " << ch.udp.sequence << " " << m_ackNo[port] << std::endl;
// 			EnReorderingQueue(p, port);
// 		}
// 		else if(ch.udp.sequence < m_ackNo[port]){
// 			//de-duplication
// 			return;
// 		}
// 	}
// 	else if(ch.l3Prot == 0xF9){
// 		if(ch.empty.empty_seq == m_ackNo[port]){
// 			std::cout << "reordering recirculate find expected empty data " << Simulator::Now() << " " << ch.empty.empty_seq << " " << m_ackNo[port] << std::endl;
// 			m_ackNo[port]++;
// 		}
// 		else if(ch.empty.empty_seq > m_ackNo[port]){
// 			std::cout << "reordering recirculate another time empty data " << Simulator::Now() << " " << ch.empty.empty_seq << " " << m_ackNo[port] << std::endl;
// 			EnReorderingQueue(p, port);
// 		}
// 		else if(ch.empty.empty_seq < m_ackNo[port]){
// 			return;
// 		}
// 	}
// 	return;
// }

void SwitchNode::ConfigLossRate(uint16_t rate)
{
	m_lossrate = rate;
}
void SwitchNode::ConfigLookupTableSize(uint16_t tablesize)
{
	m_lookuptablesize = tablesize;
}
void SwitchNode::ConfigFilterSize(uint32_t filtersize)
{
	m_filtersize = filtersize;
}
void SwitchNode::ConfigFilterTime(uint16_t filtertime)
{
	m_filtertime = filtertime;
}
void SwitchNode::ConfigOnlyLastAck(uint16_t onlylastack)
{
	m_onlylastack = onlylastack;
}
void SwitchNode::ConfigFilterFBSize(uint32_t filterFBsize)
{
	m_filterFBsize = filterFBsize;
}
void SwitchNode::ConfigRoundSeqSize(uint16_t roundseqsize)
{
	m_roundseqsize = roundseqsize;
}

uint16_t SwitchNode::GetLookUpTableEntrySize()
{
	return 23;
}

uint16_t SwitchNode::GetFilterValueEntrySize()
{
	return 27;
}

void SwitchNode::AddExtraHeader (Ptr<Packet> p, uint16_t sequence, uint16_t latestRxSeqNo)
{
	ExtraHeader eHeader;
	eHeader.SetSequence(sequence);
	eHeader.SetlatestRxSeqNo(latestRxSeqNo);
	p->AddHeader(eHeader);
}
void SwitchNode::ProcessExtraHeader (Ptr<Packet> p, uint16_t& sequence_param, uint16_t& latestRxSeqNo_param)
{
	ExtraHeader eHeader;
	p->RemoveHeader(eHeader);
	sequence_param = eHeader.GetSequence();
	latestRxSeqNo_param = eHeader.GetlatestRxSeqNo();
}

} /* namespace ns3 */
