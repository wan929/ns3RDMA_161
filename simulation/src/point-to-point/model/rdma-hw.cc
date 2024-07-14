#include <ns3/simulator.h>
#include <ns3/seq-ts-header.h>
#include <ns3/udp-header.h>
#include <ns3/ipv4-header.h>
#include "ns3/ppp-header.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/data-rate.h"
#include "ns3/pointer.h"
#include "rdma-hw.h"
#include "ppp-header.h"
#include "qbb-header.h"
#include "cn-header.h"
#include <bits/stdc++.h>

namespace ns3{

TypeId RdmaHw::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::RdmaHw")
		.SetParent<Object> ()
		.AddAttribute("MinRate",
				"Minimum rate of a throttled flow",
				DataRateValue(DataRate("100Mb/s")),
				MakeDataRateAccessor(&RdmaHw::m_minRate),
				MakeDataRateChecker())
		.AddAttribute("Mtu",
				"Mtu.",
				UintegerValue(1000),
				MakeUintegerAccessor(&RdmaHw::m_mtu),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute ("CcMode",
				"which mode of DCQCN is running",
				UintegerValue(0),
				MakeUintegerAccessor(&RdmaHw::m_cc_mode),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute ("RecMode",
				"which mode of recovery is running",
				UintegerValue(0),
				MakeUintegerAccessor(&RdmaHw::m_rec_mode),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute ("OnlyLastAck",
				"sv or cv",
				UintegerValue(0),
				MakeUintegerAccessor(&RdmaHw::m_onlylastack),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("NACK Generation Interval",
				"The NACK Generation interval",
				DoubleValue(500.0),
				MakeDoubleAccessor(&RdmaHw::m_nack_interval),
				MakeDoubleChecker<double>())
		.AddAttribute("L2ChunkSize",
				"Layer 2 chunk size. Disable chunk mode if equals to 0.",
				UintegerValue(0),
				MakeUintegerAccessor(&RdmaHw::m_chunk),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("L2AckInterval",
				"Layer 2 Ack intervals. Disable ack if equals to 0.",
				UintegerValue(0),
				MakeUintegerAccessor(&RdmaHw::m_ack_interval),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("L2BackToZero",
				"Layer 2 go back to zero transmission.",
				BooleanValue(false),
				MakeBooleanAccessor(&RdmaHw::m_backto0),
				MakeBooleanChecker())
		.AddAttribute("EwmaGain",
				"Control gain parameter which determines the level of rate decrease",
				DoubleValue(1.0 / 16),
				MakeDoubleAccessor(&RdmaHw::m_g),
				MakeDoubleChecker<double>())
		.AddAttribute ("RateOnFirstCnp",
				"the fraction of rate on first CNP",
				DoubleValue(1.0),
				MakeDoubleAccessor(&RdmaHw::m_rateOnFirstCNP),
				MakeDoubleChecker<double> ())
		.AddAttribute("ClampTargetRate",
				"Clamp target rate.",
				BooleanValue(false),
				MakeBooleanAccessor(&RdmaHw::m_EcnClampTgtRate),
				MakeBooleanChecker())
		.AddAttribute("RPTimer",
				"The rate increase timer at RP in microseconds",
				DoubleValue(1500.0),
				MakeDoubleAccessor(&RdmaHw::m_rpgTimeReset),
				MakeDoubleChecker<double>())
		.AddAttribute("RateDecreaseInterval",
				"The interval of rate decrease check",
				DoubleValue(4.0),
				MakeDoubleAccessor(&RdmaHw::m_rateDecreaseInterval),
				MakeDoubleChecker<double>())
		.AddAttribute("FastRecoveryTimes",
				"The rate increase timer at RP",
				UintegerValue(5),
				MakeUintegerAccessor(&RdmaHw::m_rpgThreshold),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("AlphaResumInterval",
				"The interval of resuming alpha",
				DoubleValue(55.0),
				MakeDoubleAccessor(&RdmaHw::m_alpha_resume_interval),
				MakeDoubleChecker<double>())
		.AddAttribute("RateAI",
				"Rate increment unit in AI period",
				DataRateValue(DataRate("5Mb/s")),
				MakeDataRateAccessor(&RdmaHw::m_rai),
				MakeDataRateChecker())
		.AddAttribute("RateHAI",
				"Rate increment unit in hyperactive AI period",
				DataRateValue(DataRate("50Mb/s")),
				MakeDataRateAccessor(&RdmaHw::m_rhai),
				MakeDataRateChecker())
		.AddAttribute("VarWin",
				"Use variable window size or not",
				BooleanValue(false),
				MakeBooleanAccessor(&RdmaHw::m_var_win),
				MakeBooleanChecker())
		.AddAttribute("FastReact",
				"Fast React to congestion feedback",
				BooleanValue(true),
				MakeBooleanAccessor(&RdmaHw::m_fast_react),
				MakeBooleanChecker())
		.AddAttribute("MiThresh",
				"Threshold of number of consecutive AI before MI",
				UintegerValue(5),
				MakeUintegerAccessor(&RdmaHw::m_miThresh),
				MakeUintegerChecker<uint32_t>())
		.AddAttribute("TargetUtil",
				"The Target Utilization of the bottleneck bandwidth, by default 95%",
				DoubleValue(0.95),
				MakeDoubleAccessor(&RdmaHw::m_targetUtil),
				MakeDoubleChecker<double>())
		.AddAttribute("UtilHigh",
				"The upper bound of Target Utilization of the bottleneck bandwidth, by default 98%",
				DoubleValue(0.98),
				MakeDoubleAccessor(&RdmaHw::m_utilHigh),
				MakeDoubleChecker<double>())
		.AddAttribute("RateBound",
				"Bound packet sending by rate, for test only",
				BooleanValue(true),
				MakeBooleanAccessor(&RdmaHw::m_rateBound),
				MakeBooleanChecker())
		.AddAttribute("MultiRate",
				"Maintain multiple rates in HPCC",
				BooleanValue(true),
				MakeBooleanAccessor(&RdmaHw::m_multipleRate),
				MakeBooleanChecker())
		.AddAttribute("SampleFeedback",
				"Whether sample feedback or not",
				BooleanValue(false),
				MakeBooleanAccessor(&RdmaHw::m_sampleFeedback),
				MakeBooleanChecker())
		.AddAttribute("TimelyAlpha",
				"Alpha of TIMELY",
				DoubleValue(0.875),
				MakeDoubleAccessor(&RdmaHw::m_tmly_alpha),
				MakeDoubleChecker<double>())
		.AddAttribute("TimelyBeta",
				"Beta of TIMELY",
				DoubleValue(0.8),
				MakeDoubleAccessor(&RdmaHw::m_tmly_beta),
				MakeDoubleChecker<double>())
		.AddAttribute("TimelyTLow",
				"TLow of TIMELY (ns)",
				UintegerValue(50000),
				MakeUintegerAccessor(&RdmaHw::m_tmly_TLow),
				MakeUintegerChecker<uint64_t>())
		.AddAttribute("TimelyTHigh",
				"THigh of TIMELY (ns)",
				UintegerValue(500000),
				MakeUintegerAccessor(&RdmaHw::m_tmly_THigh),
				MakeUintegerChecker<uint64_t>())
		.AddAttribute("TimelyMinRtt",
				"MinRtt of TIMELY (ns)",
				UintegerValue(20000),
				MakeUintegerAccessor(&RdmaHw::m_tmly_minRtt),
				MakeUintegerChecker<uint64_t>())
		.AddAttribute("DctcpRateAI",
				"DCTCP's Rate increment unit in AI period",
				DataRateValue(DataRate("1000Mb/s")),
				MakeDataRateAccessor(&RdmaHw::m_dctcp_rai),
				MakeDataRateChecker())
		.AddAttribute("PintSmplThresh",
				"PINT's sampling threshold in rand()%65536",
				UintegerValue(65536),
				MakeUintegerAccessor(&RdmaHw::pint_smpl_thresh),
				MakeUintegerChecker<uint32_t>())
		;
	return tid;
}

RdmaHw::RdmaHw(){
	first_p = true;
	start_time = 0;
	end_time = 0;
}

void RdmaHw::SetNode(Ptr<Node> node){
	m_node = node;
}
void RdmaHw::Setup(QpCompleteCallback cb){
	for (uint32_t i = 0; i < m_nic.size(); i++){
		Ptr<QbbNetDevice> dev = m_nic[i].dev;
		if (dev == NULL)
			continue;
		// share data with NIC
		dev->m_rdmaEQ->m_qpGrp = m_nic[i].qpGrp;
		// setup callback
		dev->m_rdmaReceiveCb = MakeCallback(&RdmaHw::Receive, this); //here
		dev->m_rdmaLinkDownCb = MakeCallback(&RdmaHw::SetLinkDown, this);
		dev->m_rdmaPktSent = MakeCallback(&RdmaHw::PktSent, this);
		// config NIC
		dev->m_rdmaEQ->m_rdmaGetNxtPkt = MakeCallback(&RdmaHw::GetNxtPacket, this);
		dev->m_rdmaEQ->m_rdmaGetPrbPkt = MakeCallback(&RdmaHw::GetPrbPacket, this);
	}
	// setup qp complete callback
	m_qpCompleteCallback = cb;
}

uint32_t RdmaHw::GetNicIdxOfQp(Ptr<RdmaQueuePair> qp){
	auto &v = m_rtTable[qp->dip.Get()];
	if (v.size() > 0){
		return v[qp->GetHash() % v.size()];
	}else{
		NS_ASSERT_MSG(false, "We assume at least one NIC is alive");
	}
}
uint64_t RdmaHw::GetQpKey(uint32_t dip, uint16_t sport, uint16_t pg){
	return ((uint64_t)dip << 32) | ((uint64_t)sport << 16) | (uint64_t)pg;
}
Ptr<RdmaQueuePair> RdmaHw::GetQp(uint32_t dip, uint16_t sport, uint16_t pg){
	uint64_t key = GetQpKey(dip, sport, pg);
	auto it = m_qpMap.find(key);
	if (it != m_qpMap.end())
		return it->second;
	return NULL;
}
void RdmaHw::AddQueuePair(uint64_t size, uint16_t pg, Ipv4Address sip, Ipv4Address dip, uint16_t sport, uint16_t dport, uint32_t win, uint64_t baseRtt, Callback<void> notifyAppFinish){
	// create qp
	Ptr<RdmaQueuePair> qp = CreateObject<RdmaQueuePair>(pg, sip, dip, sport, dport);
	qp->SetSize(size);
	qp->SetWin(win);
	qp->SetBaseRtt(baseRtt);
	qp->SetVarWin(m_var_win);
	qp->SetAppNotifyCallback(notifyAppFinish);
	//qp->SetTimeout(MilliSeconds (1)); //1 4ms

	// add qp
	uint32_t nic_idx = GetNicIdxOfQp(qp);
	m_nic[nic_idx].qpGrp->AddQp(qp);
	uint64_t key = GetQpKey(dip.Get(), sport, pg);
	m_qpMap[key] = qp;

	// set init variables
	DataRate m_bps = m_nic[nic_idx].dev->GetDataRate();
	qp->m_rate = m_bps;
	qp->m_max_rate = m_bps;
	if (m_cc_mode == 1){
		qp->mlx.m_targetRate = m_bps;
	}else if (m_cc_mode == 3){
		qp->hp.m_curRate = m_bps;
		if (m_multipleRate){
			for (uint32_t i = 0; i < IntHeader::maxHop; i++)
				qp->hp.hopState[i].Rc = m_bps;
		}
	}else if (m_cc_mode == 7){
		qp->tmly.m_curRate = m_bps;
	}else if (m_cc_mode == 10){
		qp->hpccPint.m_curRate = m_bps;
	}

	// Notify Nic
	m_nic[nic_idx].dev->NewQp(qp);
}

void RdmaHw::DeleteQueuePair(Ptr<RdmaQueuePair> qp){
	// remove qp from the m_qpMap
	uint64_t key = GetQpKey(qp->dip.Get(), qp->sport, qp->m_pg);
	m_qpMap.erase(key);
}

Ptr<RdmaRxQueuePair> RdmaHw::GetRxQp(uint32_t sip, uint32_t dip, uint16_t sport, uint16_t dport, uint16_t pg, bool create){
	uint64_t key = ((uint64_t)dip << 32) | ((uint64_t)pg << 16) | (uint64_t)dport;
	auto it = m_rxQpMap.find(key);
	if (it != m_rxQpMap.end())
		return it->second;
	if (create){
		// create new rx qp
		Ptr<RdmaRxQueuePair> q = CreateObject<RdmaRxQueuePair>();
		// init the qp
		q->sip = sip;
		q->dip = dip;
		q->sport = sport;
		q->dport = dport;
		q->m_ecn_source.qIndex = pg;
		// store in map
		m_rxQpMap[key] = q;
		return q;
	}
	return NULL;
}
uint32_t RdmaHw::GetNicIdxOfRxQp(Ptr<RdmaRxQueuePair> q){
	auto &v = m_rtTable[q->dip];
	if (v.size() > 0){
		return v[q->GetHash() % v.size()];
	}else{
		NS_ASSERT_MSG(false, "We assume at least one NIC is alive");
	}
}
void RdmaHw::DeleteRxQp(uint32_t dip, uint16_t pg, uint16_t dport){
	uint64_t key = ((uint64_t)dip << 32) | ((uint64_t)pg << 16) | (uint64_t)dport;
	m_rxQpMap.erase(key);
}

int RdmaHw::ReceiveUdp(Ptr<Packet> p, CustomHeader &ch){
	//std::cout << "receive udp " << ch.udp.seq << std::endl;
	uint8_t ecnbits = ch.GetIpv4EcnBits();

	uint32_t payload_size = p->GetSize() - ch.GetSerializedSize();
	// TODO find corresponding rx queue pair
	//Ptr<RdmaRxQueuePair> rxQp = GetRxQp(ch.dip, ch.sip, ch.udp.dport, ch.udp.sport, (ch.udp.pg >> 8) & 0xff, true);
	Ptr<RdmaRxQueuePair> rxQp = GetRxQp(ch.dip, ch.sip, ch.udp.dport, ch.udp.sport, ch.udp.pg, true);
	if (ecnbits != 0){
		rxQp->m_ecn_source.ecnbits |= ecnbits;
		rxQp->m_ecn_source.qfb++;
	}
	rxQp->m_ecn_source.total++;
	rxQp->m_milestone_rx = m_ack_interval;
	
	int x = ReceiverCheckSeq(ch.udp.seq, rxQp, payload_size);
	///*
	//0b006101 0b007201 10000 100
	// if(ch.sip == 0x0b006101 && ch.dip == 0x0b007201 && ch.udp.sport == 10000 && ch.udp.dport ==100)
	// {
	// 	std::cout << "receive udp " << std::dec << m_node->GetId() << " " ;
	// 	std::cout << std::hex << "0" << ch.sip << " 0" << ch.dip << " " ;
	// 	std::cout << std::dec << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " rxqp->expected " <<  rxQp->ReceiverNextExpectedSeq << " ack_x " << x << std::endl;
	// }
	//*/
	//0b005f01 0b00ad01 10000 100
	if ((m_rec_mode == 0 && (x == 1 || x == 2)) || (m_rec_mode == 1 && (x == 1 || (m_onlylastack == 1 && x == 2))) || (m_rec_mode == 2 && (x == 1 || x == 2))){ //generate ACK or NACK
		qbbHeader seqh;
		seqh.SetSeq(rxQp->ReceiverNextExpectedSeq);

		// if(m_rec_mode == 0 && ((ch.udp.flags >> SeqTsHeader::FLAG_LAST) & 1)) //last p
		// {
		// 	m_node->seq_expected_number++;
		// 	rxQp->ReceiverNextExpectedSeq = ch.udp.seq;
		// }

		seqh.SetPG(ch.udp.pg);
		seqh.SetSport(ch.udp.dport);
		seqh.SetDport(ch.udp.sport);
		seqh.SetIntHeader(ch.udp.ih);
		if (ecnbits)
			seqh.SetCnp();
		if ((ch.udp.flags >> SeqTsHeader::FLAG_LAST) & 1)
		{
			seqh.SetLast();
		}
		Ptr<Packet> newp = Create<Packet>(std::max(60-14-20-(int)seqh.GetSerializedSize(), 0));
		newp->AddHeader(seqh);

		Ipv4Header head;	// Prepare IPv4 header
		head.SetDestination(Ipv4Address(ch.sip));
		head.SetSource(Ipv4Address(ch.dip));

		if(m_onlylastack == 1 && x == 2){
			// std::cout << "generate nack " << std::dec << m_node->GetId() << " " ;
			// std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
			// std::cout << std::dec << ch.udp.dport << " " << ch.udp.sport << " " << ch.udp.seq  << " " << rxQp->ReceiverNextExpectedSeq << std::endl;
		}
		
		head.SetProtocol(x == 1 ? 0xFC : 0xFD); //ack=0xFC nack=0xFD
		head.SetTtl(64);
		head.SetPayloadSize(newp->GetSize());
		head.SetIdentification(rxQp->m_ipid++);

		newp->AddHeader(head);
		AddHeader(newp, 0x800);	// Attach PPP header
		// send
		uint32_t nic_idx = GetNicIdxOfRxQp(rxQp);
		m_nic[nic_idx].dev->RdmaEnqueueHighPrioQ(newp);
		m_nic[nic_idx].dev->TriggerTransmit();
	}
	return 0;
}

int RdmaHw::ReceiveCnp(Ptr<Packet> p, CustomHeader &ch){
	// QCN on NIC
	// This is a Congestion signal
	// Then, extract data from the congestion packet.
	// We assume, without verify, the packet is destinated to me
	uint32_t qIndex = ch.cnp.qIndex;
	if (qIndex == 1){		//DCTCP
		std::cout << "TCP--ignore\n";
		return 0;
	}
	uint16_t udpport = ch.cnp.fid; // corresponds to the sport
	uint8_t ecnbits = ch.cnp.ecnBits;
	uint16_t qfb = ch.cnp.qfb;
	uint16_t total = ch.cnp.total;
	uint32_t i;
	// get qp
	Ptr<RdmaQueuePair> qp = GetQp(ch.sip, udpport, qIndex);
	if (qp == NULL)
		std::cout << "ERROR: QCN NIC cannot find the flow\n";
	// get nic
	uint32_t nic_idx = GetNicIdxOfQp(qp);
	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;

	if (qp->m_rate == 0)			//lazy initialization	
	{
		qp->m_rate = dev->GetDataRate();
		if (m_cc_mode == 1){
			qp->mlx.m_targetRate = dev->GetDataRate();
		}else if (m_cc_mode == 3){
			qp->hp.m_curRate = dev->GetDataRate();
			if (m_multipleRate){
				for (uint32_t i = 0; i < IntHeader::maxHop; i++)
					qp->hp.hopState[i].Rc = dev->GetDataRate();
			}
		}else if (m_cc_mode == 7){
			qp->tmly.m_curRate = dev->GetDataRate();
		}else if (m_cc_mode == 10){
			qp->hpccPint.m_curRate = dev->GetDataRate();
		}
	}
	return 0;
}

//in-network notification
int RdmaHw::ReceiveNotify(Ptr<Packet> p, CustomHeader &ch){
	uint32_t nextseq = ch.notify.m_notifyseq;
	uint16_t port = ch.notify.dport;
	uint16_t qIndex = ch.notify.m_pg;

	//std::cout << "sender has receive a notify " << std::dec << m_node->GetId() << " " ; 
	//std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
	//std::cout << std::dec << ch.notify.dport << " " << ch.notify.sport << " " << ch.notify.m_notifyseq << " " << unsigned(ch.notify.m_round) << " " <<  unsigned(ch.notify.m_pg) << std::endl;

	Ptr<RdmaQueuePair> qp = GetQp(ch.sip, port, qIndex);
	//std::cout << "Notify " << ch.sip << " " << port << " " << qIndex << " " << qp->snd_nxt << " " << qp->snd_una << " " << qp->m_size << std::endl;
	if (qp == NULL){
		// std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
		// std::cout << std::dec << ch.notify.dport << " " << ch.notify.sport << " " << ch.notify.m_notifyseq << " ";
		std::cout << "ERROR: " << "node:" << m_node->GetId() << ' ' << "Notify" << " NIC cannot find the flow\n";
		return 0;
	}
	//0b010b01 0b009401 10000 100
	///*
	// if(ch.dip == 0x0b003601 && ch.sip == 0x0b004501 && ch.udp.dport == 10000 && ch.udp.sport ==100)
	// {
	// 	std::cout << "notify " << std::dec << m_node->GetId() << " " ;
	// 	std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
	// 	std::cout << std::dec << ch.udp.dport << " " << ch.udp.sport << " notify_nextseq " << ch.notify.m_notifyseq << " qp->snd_nxt " << qp->snd_nxt << " qp->snd_una " << qp->snd_una << " ";
	// 	std::cout << "notify_loss_round " << unsigned(ch.notify.m_round) << " qp->m_round " << qp->m_round << std::endl;
	// }
	//*/
	uint32_t nic_idx = GetNicIdxOfQp(qp);
	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;
	// && ch.notify.m_loss_round == qp->m_round
	// && nextseq < qp->round_seq[ch.notify.m_loss_round]
	
// version 4
	if(qp->m_round == 0) //first FB
	{
		qp->m_round++;
		qp->round_seq = nextseq;
		qp->snd_nxt = nextseq;
	}
	else{ //later FB
		if(ch.notify.m_round < qp->m_round)
		{
			if(nextseq >= qp->round_seq) //NACK from receiver
			{
				m_node->filter_notify_number++;
				return 0;
			}
			else if(nextseq < qp->round_seq)
			{
				qp->m_round++;
				qp->round_seq = nextseq;
				qp->snd_nxt = nextseq;
			}
		}
		else if(ch.notify.m_round == qp->m_round)
		{
			if(nextseq >= qp->round_seq)
			{
				qp->m_round++;
				qp->round_seq = nextseq;
				qp->snd_nxt = nextseq;
			}
			else{
				std::cout << "Notify error: why same round, but small notify?" << std::endl;
			}
		}
	}
// version 3
	/*
	if(qp->round_vaild[ch.notify.m_round] == 1) //vaild
	{
		if(ch.notify.m_round == qp->m_round)
		{
			if(qp->snd_nxt > nextseq && nextseq >= qp->snd_una)
			{
				qp->round_seq[ch.notify.m_round] = nextseq;
				qp->snd_nxt = nextseq;
				qp->m_round++;
			}
		}
		else if(ch.notify.m_round < qp->m_round)
		{
			if(nextseq < qp->round_seq[ch.notify.m_round] && nextseq >= qp->snd_una)
			{
				qp->round_seq[ch.notify.m_round] = nextseq;
				qp->snd_nxt = nextseq;
				qp->m_round++;
				for(uint32_t i = ch.notify.m_round + 1; i < qp->m_round; i++)
				{
					qp->round_vaild[i] = 0; //unvaild
				}
			}
		}
	}
	else{ //regardless
		//std::cout << "this round is vaild" << std::endl;
		return 0;
	}

	*/

// version 2
	/*
	if(ch.notify.m_round == qp->m_round)
	{
		if(qp->snd_nxt > nextseq && nextseq >= qp->snd_una)
		{
			qp->round_seq[ch.notify.m_round] = nextseq;
			qp->snd_nxt = nextseq;
			qp->m_round++;
		}
	}
	else if(ch.notify.m_round < qp->m_round)
	{
		if(nextseq < qp->round_seq[ch.notify.m_round] && nextseq >= qp->snd_una)
		{
			qp->round_seq[ch.notify.m_round] = nextseq;
			qp->snd_nxt = nextseq;
			qp->m_round++;
		}
	}
	*/

// version 1
	/*
	if(nextseq < qp->snd_nxt) //just good
	{
		qp->snd_nxt = nextseq;
	}
	*/
	dev->TriggerTransmit();
	return 0;
}

int RdmaHw::ReceiveAck(Ptr<Packet> p, CustomHeader &ch){
	uint16_t qIndex = ch.ack.pg;
	uint16_t port = ch.ack.dport;
	uint32_t seq = ch.ack.seq;
	uint8_t cnp = (ch.ack.flags >> qbbHeader::FLAG_CNP) & 1;
	int i;
	Ptr<RdmaQueuePair> qp = GetQp(ch.sip, port, qIndex);
	if (qp == NULL){
		// std::cout << "ERROR: " << "node:" << m_node->GetId() << ' ' << (ch.l3Prot == 0xFC ? "ACK" : "NACK") << " NIC cannot find the flow\n";

		// std::cout << "ack ERROR " << std::dec << m_node->GetId() << " " ;
		// std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
		// std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " ack_nextseq " << ch.ack.seq << std::endl;
		
		return 0;
	}
	//0b010b01 0b009401 10000 100
	/*
	if(ch.dip == 0x0b00d501 && ch.sip == 0x0b010101 && ch.udp.dport == 10000 && ch.udp.sport ==100)
	{
		std::cout << "ack " << std::dec << m_node->GetId() << " " ;
		std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
		std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " ack_nextseq " << ch.ack.seq << " qp->snd_nxt " << qp->snd_nxt << " qp->snd_una " << qp->snd_una << " ";
		std::cout << " qp->m_round " << qp->m_round << std::endl;
	}
	*/
	uint32_t nic_idx = GetNicIdxOfQp(qp);
	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;

	if(m_rec_mode == 1 && m_onlylastack != 1) //SADR_CV //SADR_SLR
	{
		if(m_onlylastack == 0 && ch.l3Prot == 0xFD) //SADR_CV NACK
		{
			std::cout << "receive nack " << std::dec << m_node->GetId() << " " ;
			std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
			std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << ch.ack.latestRxSeqNo << " qp->snd_nxt " << qp->snd_nxt << std::endl;
			
			if(qp->m_round == 0){
				qp->m_round++;
				qp->round_seq = ch.ack.seq;
				qp->snd_nxt = ch.ack.seq;
			}else{
				if(ch.ack.latestRxSeqNo < qp->m_round){
					if(ch.ack.seq >= qp->round_seq) //NACK from receiver
					{
						m_node->filter_notify_number++;
						return 0;
					}else if(ch.ack.seq < qp->round_seq)
					{
						qp->m_round++;
						qp->round_seq = ch.ack.seq;
						qp->snd_nxt = ch.ack.seq;
					}
				}else if(ch.ack.latestRxSeqNo == qp->m_round){
					if(ch.ack.seq >= qp->round_seq)
					{
						qp->m_round++;
						qp->round_seq = ch.ack.seq;
						qp->snd_nxt = ch.ack.seq;
					}
					else{
						std::cout << "NACK error: why same round, but small notify?" << std::endl;
					}
				}
			}
			dev->TriggerTransmit();
			return 0;
		}

		if(m_onlylastack == 2 && ch.l3Prot == 0xFD){ //SADR_SLR NACK
			std::cout << "receive nack " << std::dec << m_node->GetId() << " " ;
			std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
			std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << " " << ch.ack.latestRxSeqNo << " qp->snd_nxt " << qp->snd_nxt << std::endl;

			if(qp->halt_state == false) //first
			{
				qp->deduced_seq = ch.ack.seq;
				qp->snd_nxt = qp->deduced_seq;
				qp->halt_state = true;
				qp->halt_time_expired == false;
				qp->halt_probe = Simulator::Schedule(MicroSeconds(160), &RdmaHw::SLR_Probe, this, qp);
			}else if(qp->halt_state == true) //later
			{
				if(qp->halt_time_expired == false){
					qp->deduced_seq = ch.ack.seq < qp->deduced_seq ? ch.ack.seq : qp->deduced_seq;
					qp->snd_nxt = qp->deduced_seq;
				}else if(qp->halt_time_expired == true){
					if(ch.ack.seq <= qp->deduced_seq){
						qp->deduced_seq = ch.ack.seq;
						qp->snd_nxt = qp->deduced_seq;
						qp->probe_state = true;
					}
				}
			}
			return 0;
		}

		if(m_onlylastack == 2 && ch.l3Prot == 0xFC) //SADR_SLR ACK
		{
			if(qp->halt_state == true){ //
				if((ch.ack.seq > qp->deduced_seq) && (ch.ack.seq <= qp->deduced_seq + 1000)) //confirm expected seq
				{
					std::cout << Simulator::Now() << " confirmed seq " << std::dec << m_node->GetId() << " " ;
					std::cout << std::hex << "0" << qp->sip.Get() << " 0" << qp->dip.Get() << " " ;
					std::cout << std::dec << qp->sport << " " << qp->dport << " " << ch.ack.seq << " " <<qp->deduced_seq << std::endl;
					qp->halt_state = false;
					qp->snd_nxt = ch.ack.seq;
				}
			}
		}
	}
	
	if (m_ack_interval == 0) //init value = 1
		std::cout << "ERROR: shouldn't receive ack\n";
	else {
		if (!m_backto0){
			qp->Acknowledge(seq);
		}else {
			uint32_t goback_seq = seq / m_chunk * m_chunk;
			qp->Acknowledge(goback_seq);
		}
		if (qp->IsFinished()){
			QpComplete(qp);
		}
	}
///*
// if necessery, do rto
/*
	//if(!qp->IsFinished() && ch.l3Prot == 0xFC) //ack, not nack
	if(!qp->IsFinished())
	{
		Simulator::Cancel(qp->ack_rto);
		qp->ack_rto = Simulator::Schedule((MicroSeconds(1000)), &RdmaHw::AckTimeoutRetransmit, this, qp, seq); //1ms
	}
*/
	/** 
	 * IB Spec Vol. 1 o9-85
	 * The requester need not separately time each request launched into the
	 * fabric, but instead simply begins the timer whenever it is expecting a response.
	 * Once started, the timer is restarted each time an acknowledge
	 * packet is received as long as there are outstanding expected responses.
	 * The timer does not detect the loss of a particular expected acknowledge
	 * packet, but rather simply detects the persistent absence of response
	 * packets.
	 * */
	//if(m_rec_mode == 0)
	{
		if (!qp->IsFinished() && qp->GetOnTheFly() > 0) {
			if (qp->m_retransmit.IsRunning())
				qp->m_retransmit.Cancel();
			qp->m_retransmit = Simulator::Schedule(qp->GetRto(), &RdmaHw::HandleTimeout, this, qp, qp->GetRto());
		}
	}

	if (ch.l3Prot == 0xFD) // SADR_SV NACK
	{
		std::cout << "receive nack " << std::dec << m_node->GetId() << " " ;
		std::cout << std::hex << "0" << ch.dip << " 0" << ch.sip << " " ;
		std::cout << std::dec << ch.ack.dport << " " << ch.ack.sport << " " << ch.ack.seq << std::endl;

		if(m_rec_mode == 1 && m_onlylastack == 1){
			qp->m_round++;
		}
		RecoverQueue(qp);

	}
	// handle cnp
	if (cnp){
		if (m_cc_mode == 1){ // mlx version
			cnp_received_mlx(qp);
		} 
	}

	if (m_cc_mode == 3){
		HandleAckHp(qp, p, ch);
	}else if (m_cc_mode == 7){
		HandleAckTimely(qp, p, ch);
	}else if (m_cc_mode == 8){
		HandleAckDctcp(qp, p, ch);
	}else if (m_cc_mode == 10){
		HandleAckHpPint(qp, p, ch);
	}
	// ACK may advance the on-the-fly window, allowing more packets to send
	dev->TriggerTransmit();
	return 0;
}

int RdmaHw::Receive(Ptr<Packet> p, CustomHeader &ch){
	if (ch.l3Prot == 0x11){ // UDP
		ReceiveUdp(p, ch);
	}else if (ch.l3Prot == 0xFF){ // CNP
		ReceiveCnp(p, ch);
	}else if (ch.l3Prot == 0xFD){ // NACK
		ReceiveAck(p, ch);
	}else if (ch.l3Prot == 0xFC){ // ACK
		ReceiveAck(p, ch);
	}else if (ch.l3Prot == 0xFB){ //Notify
		ReceiveNotify(p, ch);
	}
	return 0;
}

int RdmaHw::ReceiverCheckSeq(uint32_t seq, Ptr<RdmaRxQueuePair> q, uint32_t size){
	uint32_t expected = q->ReceiverNextExpectedSeq;
	if (seq == expected || (seq < expected && seq + size >= expected)){ //tlt
		if(seq < expected && seq + size >= expected){
			// std::cout << "Here " << std::dec << m_node->GetId() << " " ;
			// std::cout << std::hex << "0" << q->dip << " 0" << q->sip << " " ;
			// std::cout << std::dec << q->dport << " " << q->sport << " seq " << seq << " size " << size << " expected " << expected << std::endl;
			m_node->seq_expected_number++;
		}
		q->ReceiverNextExpectedSeq += size - (expected - seq);
	// if (seq == expected){
	//  	q->ReceiverNextExpectedSeq = expected + size;
		if (q->ReceiverNextExpectedSeq >= q->m_milestone_rx){
			q->m_milestone_rx += m_ack_interval;
			return 1; //Generate ACK
		}else if (q->ReceiverNextExpectedSeq % m_chunk == 0){
			return 1;
		}else {
			return 5;
		}
	} else if (seq > expected) {
		// Generate NACK
		if (Simulator::Now() >= q->m_nackTimer || q->m_lastNACK != expected){
			q->m_nackTimer = Simulator::Now() + MicroSeconds(m_nack_interval);
			q->m_lastNACK = expected;
			if (m_backto0){
				q->ReceiverNextExpectedSeq = q->ReceiverNextExpectedSeq / m_chunk*m_chunk;
			}
			return 2;
		}else
			return 4;
	}else {
		// Duplicate.
		//std::cout << "duplicate " << seq << std::endl;
/*
		std::cout << "Duplicate " << std::dec << m_node->GetId() << " " ;
		std::cout << std::hex << "0" << q->dip << " 0" << q->sip << " " ;
		std::cout << std::dec << q->dport << " " << q->sport << " " << " p_seq " << seq << " rxqp->expected " << expected << std::endl;
*/
		return 3;

		// Duplicate.  tlt
		//return 1; // According to IB Spec C9-110
		/**
		 * IB Spec C9-110
		 * A responder shall respond to all duplicate requests in PSN order;
		 * i.e. the request with the (logically) earliest PSN shall be executed first. If,
		 * while responding to a new or duplicate request, a duplicate request is received
		 * with a logically earlier PSN, the responder shall cease responding
		 * to the original request and shall begin responding to the duplicate request
		 * with the logically earlier PSN.
		 */
	}
}
void RdmaHw::AddHeader (Ptr<Packet> p, uint16_t protocolNumber){
	PppHeader ppp;
	ppp.SetProtocol (EtherToPpp (protocolNumber));
	p->AddHeader (ppp);
}
uint16_t RdmaHw::EtherToPpp (uint16_t proto){
	switch(proto){
		case 0x0800: return 0x0021;   //IPv4
		case 0x86DD: return 0x0057;   //IPv6
		default: NS_ASSERT_MSG (false, "PPP Protocol number not defined!");
	}
	return 0;
}

// void RdmaHw::AckTimeoutRetransmit(Ptr<RdmaQueuePair> qp, uint32_t seq)
// {
// 	if(qp->IsFinished())
// 	{
// 		return;
// 	}

// 	if (qp == NULL){
// 		std::cout << "AckTimeoutERROR: " << "node:" << m_node->GetId() << ' ' << "ACK" << " NIC cannot find the flow\n";
// 		return;
// 	}

// 	uint32_t nic_idx = GetNicIdxOfQp(qp);
// 	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;

// 	//this qp doesn't finish
// 	if(seq == qp->snd_una) //during 1ms, no other ack come
// 	{
// 		/*
// 		//if(qp->sip.Get() == 0x0b006301 && qp->dip.Get() == 0x0b000501 && qp->sport == 10000 && qp->dport ==100)
// 		{
// 			std::cout << "AckTimeout " << std::dec << m_node->GetId() << " " ;
// 			std::cout << std::hex << "0" << qp->sip.Get() << " 0" << qp->dip.Get() << " " ;
// 			std::cout << std::dec << qp->sport << " " << qp->dport << " " << seq << " qp->snd_nxt " << qp->snd_nxt << " qp->snd_una " << qp->snd_una << " qp->m_pg " << qp->m_pg << " qp->m_round " << qp->m_round << std::endl;
// 		}
// 		*/
// 		RecoverQueue(qp); //from snd_una
// 		dev->TriggerTransmit();
// 		return;
// 	}
// 	else{ //during 1ms, at least one other ack come
// 		return;
// 	}
// 	return;
// }

// void RdmaHw::LastDataTimeoutRetransmit(Ptr<RdmaQueuePair> qp, uint32_t seq)
// {
// 	if (qp == NULL){
// 		//std::cout << "LastDataTimeout NULL ERROR: " << Simulator::Now() << " " << std::dec << m_node->GetId() << " " ;
// 		//std::cout << std::hex << "0" << qp->sip.Get() << " 0" << qp->dip.Get() << " " ;
// 		//std::cout << std::dec << qp->sport << " " << qp->dport << " this seq " << seq << std::endl;
// 		return;
// 	}

// 	if(qp->IsFinished())
// 	{
// 		//std::cout << "LastDataTimeout finish ERROR: " << Simulator::Now() << " " << std::dec << m_node->GetId() << " " ;
// 		//std::cout << std::hex << "0" << qp->sip.Get() << " 0" << qp->dip.Get() << " " ;
// 		//std::cout << std::dec << qp->sport << " " << qp->dport << " this seq " << seq << std::endl;
// 		return;
// 	}
// 	//*/
// 	uint32_t nic_idx = GetNicIdxOfQp(qp);
// 	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;

// 	//this qp doesn't finish
// 	if(seq == qp->snd_una) //during 1ms, no other ack come
// 	{
// 		//if(qp->sip.Get() == 0x0b00b701 && qp->dip.Get() == 0x0b00c301 && qp->sport == 10000 && qp->dport ==100)
// 		// {
// 		// 	std::cout << "LastDataTimeout trigger: " << Simulator::Now() << " " << std::dec << m_node->GetId() << " " ;
// 		// 	std::cout << std::hex << "0" << qp->sip.Get() << " 0" << qp->dip.Get() << " " ;
// 		// 	std::cout << std::dec << qp->sport << " " << qp->dport << " this seq " << seq << " qp->snd_nxt " << qp->snd_nxt << " qp->snd_una " << qp->snd_una << " qp->m_pg " << qp->m_pg << std::endl;
// 		// }
// 		RecoverQueue(qp); //from snd_una
// 		dev->TriggerTransmit();
// 		return;
// 	}
// 	else if(seq < qp->snd_una)
// 	{
// 		//std::cout << "Has acked: " << Simulator::Now() << " " << std::dec << m_node->GetId() << " " ;
// 		//std::cout << std::hex << "0" << qp->sip.Get() << " 0" << qp->dip.Get() << " " ;
// 		//std::cout << std::dec << qp->sport << " " << qp->dport << " this seq " << seq << " qp->snd_nxt " << qp->snd_nxt << " qp->snd_una " << qp->snd_una << " qp->m_pg " << qp->m_pg << std::endl;
// 		return;
// 	}
// 	else if(seq > qp->snd_una)
// 	{
// 		//std::cout << "Smaller has not acked: " << Simulator::Now() << " " << std::dec << m_node->GetId() << " " ;
// 		//std::cout << std::hex << "0" << qp->sip.Get() << " 0" << qp->dip.Get() << " " ;
// 		//std::cout << std::dec << qp->sport << " " << qp->dport << " this seq " << seq << " qp->snd_nxt " << qp->snd_nxt << " qp->snd_una " << qp->snd_una << " qp->m_pg " << qp->m_pg << std::endl;
// 		return;
// 	}
// 	return;
// }


void RdmaHw::HandleTimeout(Ptr<RdmaQueuePair> qp, Time rto) {
	if (qp == NULL)
	{
		return;
	}

	if (qp->IsFinished())
	{
		return;
	}

	uint32_t nic_idx = GetNicIdxOfQp(qp);
	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;

	std::cout << "Timeout " << std::dec << m_node->GetId() << " " ;
	std::cout << std::hex << "0" << qp->sip.Get() << " 0" << qp->dip.Get() << " " ;
	std::cout << std::dec << qp->sport << " " << qp->dport << " snd_nxt " << qp->snd_nxt << " snd_una " << qp->snd_una << std::endl;
	
	m_node->timeout_number++;	
	if(m_rec_mode == 1 && m_onlylastack == 1){
		qp->m_round++;
	}
	if(m_rec_mode == 1 && m_onlylastack == 2){
		qp->halt_state = false;
		qp->probe_state = false;
		qp->halt_time_expired = false;
	}
	RecoverQueue(qp);
	dev->TriggerTransmit();
	return;
}


Ptr<Packet> RdmaHw::GetPrbPacket(Ptr<RdmaQueuePair> qp){
	if (qp == NULL){
		return 0;
	}

	uint32_t payload_size = qp->m_size >= qp->deduced_seq ? qp->m_size - qp->deduced_seq : 0;
	if (m_mtu < payload_size)
		payload_size = m_mtu;
	Ptr<Packet> p = Create<Packet> (payload_size);
	SeqTsHeader seqTs;
	seqTs.SetSeq (qp->deduced_seq);
	seqTs.SetPG (qp->m_pg);
	if(qp->deduced_seq + payload_size == qp->m_size) //last p
	{
		seqTs.SetLast();
	}
	else if(qp->deduced_seq == 0) //first p
	{
		seqTs.SetFirst();
	}
	else{ //middle p
		seqTs.SetMiddle();
	}
	seqTs.SetSequence(0);
	seqTs.SetRound(qp->m_round); //0
	p->AddHeader (seqTs);
	UdpHeader udpHeader;
	udpHeader.SetDestinationPort (qp->dport);
	udpHeader.SetSourcePort (qp->sport);
	p->AddHeader (udpHeader);
	Ipv4Header ipHeader;
	ipHeader.SetSource (qp->sip);
	ipHeader.SetDestination (qp->dip);
	ipHeader.SetProtocol (0x11);
	ipHeader.SetPayloadSize (p->GetSize());
	ipHeader.SetTtl (64);
	ipHeader.SetTos (0);
	ipHeader.SetIdentification (qp->m_ipid);
	p->AddHeader(ipHeader);
	PppHeader ppp;
	ppp.SetProtocol (0x0021); // EtherToPpp(0x800), see point-to-point-net-device.cc
	p->AddHeader (ppp);

	std::cout << Simulator::Now() << " probe " << std::dec << m_node->GetId() << " " ;
	std::cout << std::hex << "0" << qp->sip.Get() << " 0" << qp->dip.Get() << " " ;
	std::cout << std::dec << qp->sport << " " << qp->dport << " " <<qp->deduced_seq << std::endl;

	qp->probe_state = false;
	qp->halt_state = true;

	return p;
}

void RdmaHw::SLR_Probe(Ptr<RdmaQueuePair> qp){
	if (qp == NULL){
		return;
	}

	std::cout << Simulator::Now() << " halt time expired " << std::dec << m_node->GetId() << " " ;
	std::cout << std::hex << "0" << qp->sip.Get() << " 0" << qp->dip.Get() << " " ;
	std::cout << std::dec << qp->sport << " " << qp->dport << " " <<qp->deduced_seq << std::endl;

	qp->probe_state = true;
	qp->halt_time_expired = true;

	uint32_t nic_idx = GetNicIdxOfQp(qp);
	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;
	dev->TriggerTransmit();
	return;

}

void RdmaHw::RecoverQueue(Ptr<RdmaQueuePair> qp){
	qp->snd_nxt = qp->snd_una;
}

void RdmaHw::QpComplete(Ptr<RdmaQueuePair> qp){
	//std::cout << m_node->GetId() << " end " << Simulator::Now().GetTimeStep() << std::endl;
	m_node->end_time = Simulator::Now().GetTimeStep();
	NS_ASSERT(!m_qpCompleteCallback.IsNull());
	if (m_cc_mode == 1){
		Simulator::Cancel(qp->mlx.m_eventUpdateAlpha);
		Simulator::Cancel(qp->mlx.m_eventDecreaseRate);
		Simulator::Cancel(qp->mlx.m_rpTimer);
	}

	// This callback will log info
	// It may also delete the rxQp on the receiver
	m_qpCompleteCallback(qp);

	qp->m_notifyAppFinish();

	// delete the qp
	DeleteQueuePair(qp);
}

void RdmaHw::SetLinkDown(Ptr<QbbNetDevice> dev){
	printf("RdmaHw: node:%u a link down\n", m_node->GetId());
}

void RdmaHw::AddTableEntry(Ipv4Address &dstAddr, uint32_t intf_idx){
	uint32_t dip = dstAddr.Get();
	m_rtTable[dip].push_back(intf_idx);
}

void RdmaHw::ClearTable(){
	m_rtTable.clear();
}

void RdmaHw::RedistributeQp(){
	// clear old qpGrp
	for (uint32_t i = 0; i < m_nic.size(); i++){
		if (m_nic[i].dev == NULL)
			continue;
		m_nic[i].qpGrp->Clear();
	}

	// redistribute qp
	for (auto &it : m_qpMap){
		Ptr<RdmaQueuePair> qp = it.second;
		uint32_t nic_idx = GetNicIdxOfQp(qp);
		m_nic[nic_idx].qpGrp->AddQp(qp);
		// Notify Nic
		m_nic[nic_idx].dev->ReassignedQp(qp);
	}
}

Ptr<Packet> RdmaHw::GetNxtPacket(Ptr<RdmaQueuePair> qp){
	if(first_p == true)
	{
		m_node->start_time = Simulator::Now().GetTimeStep();
		first_p = false;
	}
	//std::cout << Simulator::Now().GetTimeStep() << std::endl;
	//0b00b701 0b00c301 10000 100
	/*
	if(qp->sip.Get() == 0x0b00d501 && qp->dip.Get() == 0x0b010101 && qp->sport == 10000 && qp->dport ==100)
	{
		std::cout << "alreadysend " << Simulator::Now() << " " << std::dec << m_node->GetId() << " " ;
		std::cout << std::hex << "0" << qp->sip.Get() << " 0" << qp->dip.Get() << " " ;
		std::cout << std::dec << qp->sport << " " << qp->dport << " qp->snd_nxt " << qp->snd_nxt << " qp->snd_una " << qp->snd_una << " qp->m_pg " << qp->m_pg << " qp->m_round " << qp->m_round << " " << qp->m_win << std::endl;
	}
	*/
	//0b011c01 0b00e501 10000 100
	// if(qp->sport == 18628)
	{
		//std::cout << Simulator::Now() << " " << "alreadysend " << m_node->GetId() << " " << qp->sport << " " << qp->dport << " " << qp->snd_nxt << std::endl;
	}
	uint32_t payload_size = qp->GetBytesLeft();
	if (m_mtu < payload_size)
		payload_size = m_mtu;
	Ptr<Packet> p = Create<Packet> (payload_size);
	// add SeqTsHeader
	SeqTsHeader seqTs;
	seqTs.SetSeq (qp->snd_nxt);
	seqTs.SetPG (qp->m_pg);
	//seqTs.SetRound(qp->m_round);
	if(qp->snd_nxt + payload_size == qp->m_size) //last p
	{
		seqTs.SetLast();
	}
	else if(qp->snd_nxt == 0) //first p
	{
		seqTs.SetFirst();
	}
	else{ //middle p
		seqTs.SetMiddle();
	}

	if(m_rec_mode == 1){
		seqTs.SetSequence(0);
		seqTs.SetRound(qp->m_round);
	}
	else if(m_rec_mode == 2){
		seqTs.SetSequence(0);
		seqTs.SetRound(qp->m_round);
	}
	p->AddHeader (seqTs);
	// add udp header
	UdpHeader udpHeader;
	udpHeader.SetDestinationPort (qp->dport);
	udpHeader.SetSourcePort (qp->sport);
	p->AddHeader (udpHeader);
	// add ipv4 header
	Ipv4Header ipHeader;
	ipHeader.SetSource (qp->sip);
	ipHeader.SetDestination (qp->dip);
	ipHeader.SetProtocol (0x11);
	ipHeader.SetPayloadSize (p->GetSize());
	ipHeader.SetTtl (64);
	ipHeader.SetTos (0);
	ipHeader.SetIdentification (qp->m_ipid);
	p->AddHeader(ipHeader);
	// add ppp header
	PppHeader ppp;
	ppp.SetProtocol (0x0021); // EtherToPpp(0x800), see point-to-point-net-device.cc
	p->AddHeader (ppp);
	//std::cout << seqTs.GetSerializedSize() << " " << udpHeader.GetSerializedSize() << " " << ipHeader.GetSerializedSize() << " " << ppp.GetSerializedSize() << " " << p->GetSize() << std::endl;

	// if necessery, do rto
	/*
	if(!qp->IsFinished() && qp->snd_nxt + payload_size == qp->m_size) //last p
	{
		Simulator::Cancel(qp->one_p_rto);
		qp->one_p_rto = Simulator::Schedule((MicroSeconds(1000)), &RdmaHw::LastDataTimeoutRetransmit, this, qp, qp->snd_nxt); //1ms
	}
	*/
	//Simulator::Cancel(qp->one_p_rto);
	// if(m_rec_mode == 0)
	// {
	// 	qp->one_p_rto = Simulator::Schedule((MicroSeconds(1000)), &RdmaHw::LastDataTimeoutRetransmit, this, qp, qp->snd_nxt); //1ms
	// }
	
	// update state
	qp->snd_nxt += payload_size;
	qp->m_ipid++; //had

	return p;
}

void RdmaHw::PktSent(Ptr<RdmaQueuePair> qp, Ptr<Packet> pkt, Time interframeGap){
	qp->lastPktSize = pkt->GetSize();
	UpdateNextAvail(qp, interframeGap, pkt->GetSize());

	//if(m_rec_mode == 0)
	{
		if(pkt) {
			CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
			pkt->PeekHeader(ch);
			if(ch.l3Prot == 0x11) { // UDP
				// Update Timer
				if (qp->m_retransmit.IsRunning())
					qp->m_retransmit.Cancel();
				qp->m_retransmit = Simulator::Schedule(qp->GetRto(), &RdmaHw::HandleTimeout, this, qp, qp->GetRto());
			}
		}
	}
}

void RdmaHw::UpdateNextAvail(Ptr<RdmaQueuePair> qp, Time interframeGap, uint32_t pkt_size){
	Time sendingTime;
	if (m_rateBound)
		sendingTime = interframeGap + Seconds(qp->m_rate.CalculateTxTime(pkt_size));
	else
		sendingTime = interframeGap + Seconds(qp->m_max_rate.CalculateTxTime(pkt_size));
	qp->m_nextAvail = Simulator::Now() + sendingTime;
}

void RdmaHw::ChangeRate(Ptr<RdmaQueuePair> qp, DataRate new_rate){
	#if 1
	Time sendingTime = Seconds(qp->m_rate.CalculateTxTime(qp->lastPktSize));
	Time new_sendintTime = Seconds(new_rate.CalculateTxTime(qp->lastPktSize));
	qp->m_nextAvail = qp->m_nextAvail + new_sendintTime - sendingTime;
	// update nic's next avail event
	uint32_t nic_idx = GetNicIdxOfQp(qp);
	m_nic[nic_idx].dev->UpdateNextAvail(qp->m_nextAvail);
	#endif

	// change to new rate
	qp->m_rate = new_rate;
	//std::cout << qp->m_rate << std::endl;
}

#define PRINT_LOG 0
/******************************
 * Mellanox's version of DCQCN
 *****************************/
void RdmaHw::UpdateAlphaMlx(Ptr<RdmaQueuePair> q){
	#if PRINT_LOG
	//std::cout << Simulator::Now() << " alpha update:" << m_node->GetId() << ' ' << q->mlx.m_alpha << ' ' << (int)q->mlx.m_alpha_cnp_arrived << '\n';
	//printf("%lu alpha update: %08x %08x %u %u %.6lf->", Simulator::Now().GetTimeStep(), q->sip.Get(), q->dip.Get(), q->sport, q->dport, q->mlx.m_alpha);
	#endif
	if (q->mlx.m_alpha_cnp_arrived){
		q->mlx.m_alpha = (1 - m_g)*q->mlx.m_alpha + m_g; 	//binary feedback
	}else {
		q->mlx.m_alpha = (1 - m_g)*q->mlx.m_alpha; 	//binary feedback
	}
	#if PRINT_LOG
	//printf("%.6lf\n", q->mlx.m_alpha);
	#endif
	q->mlx.m_alpha_cnp_arrived = false; // clear the CNP_arrived bit
	ScheduleUpdateAlphaMlx(q);
}
void RdmaHw::ScheduleUpdateAlphaMlx(Ptr<RdmaQueuePair> q){
	q->mlx.m_eventUpdateAlpha = Simulator::Schedule(MicroSeconds(m_alpha_resume_interval), &RdmaHw::UpdateAlphaMlx, this, q);
}

void RdmaHw::cnp_received_mlx(Ptr<RdmaQueuePair> q){
	q->mlx.m_alpha_cnp_arrived = true; // set CNP_arrived bit for alpha update
	q->mlx.m_decrease_cnp_arrived = true; // set CNP_arrived bit for rate decrease
	if (q->mlx.m_first_cnp){
		// init alpha
		q->mlx.m_alpha = 1;
		q->mlx.m_alpha_cnp_arrived = false;
		// schedule alpha update
		ScheduleUpdateAlphaMlx(q);
		// schedule rate decrease
		ScheduleDecreaseRateMlx(q, 1); // add 1 ns to make sure rate decrease is after alpha update
		// set rate on first CNP
		q->mlx.m_targetRate = q->m_rate = m_rateOnFirstCNP * q->m_rate;
		q->mlx.m_first_cnp = false;
	}
}

void RdmaHw::CheckRateDecreaseMlx(Ptr<RdmaQueuePair> q){
	ScheduleDecreaseRateMlx(q, 0);
	if (q->mlx.m_decrease_cnp_arrived){
		#if PRINT_LOG
		printf("%lu rate dec: %08x %08x %u %u (%0.3lf %.3lf)->", Simulator::Now().GetTimeStep(), q->sip.Get(), q->dip.Get(), q->sport, q->dport, q->mlx.m_targetRate.GetBitRate() * 1e-9, q->m_rate.GetBitRate() * 1e-9);
		#endif
		bool clamp = true;
		if (!m_EcnClampTgtRate){
			if (q->mlx.m_rpTimeStage == 0)
				clamp = false;
		}
		if (clamp)
			q->mlx.m_targetRate = q->m_rate;
		q->m_rate = std::max(m_minRate, q->m_rate * (1 - q->mlx.m_alpha / 2));
		// reset rate increase related things
		q->mlx.m_rpTimeStage = 0;
		q->mlx.m_decrease_cnp_arrived = false;
		Simulator::Cancel(q->mlx.m_rpTimer);
		q->mlx.m_rpTimer = Simulator::Schedule(MicroSeconds(m_rpgTimeReset), &RdmaHw::RateIncEventTimerMlx, this, q);
		#if PRINT_LOG
		printf("(%.3lf %.3lf)\n", q->mlx.m_targetRate.GetBitRate() * 1e-9, q->m_rate.GetBitRate() * 1e-9);
		#endif
	}
}
void RdmaHw::ScheduleDecreaseRateMlx(Ptr<RdmaQueuePair> q, uint32_t delta){
	q->mlx.m_eventDecreaseRate = Simulator::Schedule(MicroSeconds(m_rateDecreaseInterval) + NanoSeconds(delta), &RdmaHw::CheckRateDecreaseMlx, this, q);
}

void RdmaHw::RateIncEventTimerMlx(Ptr<RdmaQueuePair> q){
	q->mlx.m_rpTimer = Simulator::Schedule(MicroSeconds(m_rpgTimeReset), &RdmaHw::RateIncEventTimerMlx, this, q);
	RateIncEventMlx(q);
	q->mlx.m_rpTimeStage++;
}
void RdmaHw::RateIncEventMlx(Ptr<RdmaQueuePair> q){
	// check which increase phase: fast recovery, active increase, hyper increase
	if (q->mlx.m_rpTimeStage < m_rpgThreshold){ // fast recovery
		FastRecoveryMlx(q);
	}else if (q->mlx.m_rpTimeStage == m_rpgThreshold){ // active increase
		ActiveIncreaseMlx(q);
	}else { // hyper increase
		HyperIncreaseMlx(q);
	}
}

void RdmaHw::FastRecoveryMlx(Ptr<RdmaQueuePair> q){
	#if PRINT_LOG
	printf("%lu fast recovery: %08x %08x %u %u (%0.3lf %.3lf)->", Simulator::Now().GetTimeStep(), q->sip.Get(), q->dip.Get(), q->sport, q->dport, q->mlx.m_targetRate.GetBitRate() * 1e-9, q->m_rate.GetBitRate() * 1e-9);
	#endif
	q->m_rate = (q->m_rate / 2) + (q->mlx.m_targetRate / 2);
	#if PRINT_LOG
	printf("(%.3lf %.3lf)\n", q->mlx.m_targetRate.GetBitRate() * 1e-9, q->m_rate.GetBitRate() * 1e-9);
	#endif
}
void RdmaHw::ActiveIncreaseMlx(Ptr<RdmaQueuePair> q){
	#if PRINT_LOG
	printf("%lu active inc: %08x %08x %u %u (%0.3lf %.3lf)->", Simulator::Now().GetTimeStep(), q->sip.Get(), q->dip.Get(), q->sport, q->dport, q->mlx.m_targetRate.GetBitRate() * 1e-9, q->m_rate.GetBitRate() * 1e-9);
	#endif
	// get NIC
	uint32_t nic_idx = GetNicIdxOfQp(q);
	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;
	// increate rate
	q->mlx.m_targetRate += m_rai;
	if (q->mlx.m_targetRate > dev->GetDataRate())
		q->mlx.m_targetRate = dev->GetDataRate();
	q->m_rate = (q->m_rate / 2) + (q->mlx.m_targetRate / 2);
	#if PRINT_LOG
	printf("(%.3lf %.3lf)\n", q->mlx.m_targetRate.GetBitRate() * 1e-9, q->m_rate.GetBitRate() * 1e-9);
	#endif
}
void RdmaHw::HyperIncreaseMlx(Ptr<RdmaQueuePair> q){
	#if PRINT_LOG
	printf("%lu hyper inc: %08x %08x %u %u (%0.3lf %.3lf)->", Simulator::Now().GetTimeStep(), q->sip.Get(), q->dip.Get(), q->sport, q->dport, q->mlx.m_targetRate.GetBitRate() * 1e-9, q->m_rate.GetBitRate() * 1e-9);
	#endif
	// get NIC
	uint32_t nic_idx = GetNicIdxOfQp(q);
	Ptr<QbbNetDevice> dev = m_nic[nic_idx].dev;
	// increate rate
	q->mlx.m_targetRate += m_rhai;
	if (q->mlx.m_targetRate > dev->GetDataRate())
		q->mlx.m_targetRate = dev->GetDataRate();
	q->m_rate = (q->m_rate / 2) + (q->mlx.m_targetRate / 2);
	#if PRINT_LOG
	printf("(%.3lf %.3lf)\n", q->mlx.m_targetRate.GetBitRate() * 1e-9, q->m_rate.GetBitRate() * 1e-9);
	#endif
}

/***********************
 * High Precision CC
 ***********************/
void RdmaHw::HandleAckHp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
	uint32_t ack_seq = ch.ack.seq;
	// update rate
	if (ack_seq > qp->hp.m_lastUpdateSeq){ // if full RTT feedback is ready, do full update
		UpdateRateHp(qp, p, ch, false);
	}else{ // do fast react
		FastReactHp(qp, p, ch);
	}
}

void RdmaHw::UpdateRateHp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch, bool fast_react){
	uint32_t next_seq = qp->snd_nxt;
	bool print = !fast_react || true;
	if (qp->hp.m_lastUpdateSeq == 0){ // first RTT
		qp->hp.m_lastUpdateSeq = next_seq;
		// store INT
		IntHeader &ih = ch.ack.ih;
		//std::cout << ih.nhop << " " << IntHeader::maxHop << std::endl;
		NS_ASSERT(ih.nhop <= IntHeader::maxHop);
		for (uint32_t i = 0; i < ih.nhop; i++)
			qp->hp.hop[i] = ih.hop[i];
		#if PRINT_LOG
		if (print){
			printf("%lu %s %08x %08x %u %u [%u,%u,%u]", Simulator::Now().GetTimeStep(), fast_react? "fast" : "update", qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->hp.m_lastUpdateSeq, ch.ack.seq, next_seq);
			for (uint32_t i = 0; i < ih.nhop; i++)
				printf(" %u %lu %lu", ih.hop[i].GetQlen(), ih.hop[i].GetBytes(), ih.hop[i].GetTime());
			printf("\n");
		}
		#endif
	}else {
		// check packet INT
		IntHeader &ih = ch.ack.ih;
		if (ih.nhop <= IntHeader::maxHop){
			double max_c = 0;
			bool inStable = false;
			#if PRINT_LOG
			if (print)
				printf("%lu %s %08x %08x %u %u [%u,%u,%u]", Simulator::Now().GetTimeStep(), fast_react? "fast" : "update", qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->hp.m_lastUpdateSeq, ch.ack.seq, next_seq);
			#endif
			// check each hop
			double U = 0;
			uint64_t dt = 0;
			bool updated[IntHeader::maxHop] = {false}, updated_any = false;
			NS_ASSERT(ih.nhop <= IntHeader::maxHop);
			for (uint32_t i = 0; i < ih.nhop; i++){
				if (m_sampleFeedback){
					if (ih.hop[i].GetQlen() == 0 && fast_react)
						continue;
				}
				updated[i] = updated_any = true;
				#if PRINT_LOG
				if (print)
					printf(" %u(%u) %lu(%lu) %lu(%lu)", ih.hop[i].GetQlen(), qp->hp.hop[i].GetQlen(), ih.hop[i].GetBytes(), qp->hp.hop[i].GetBytes(), ih.hop[i].GetTime(), qp->hp.hop[i].GetTime());
				#endif
				uint64_t tau = ih.hop[i].GetTimeDelta(qp->hp.hop[i]);;
				double duration = tau * 1e-9;
				double txRate = (ih.hop[i].GetBytesDelta(qp->hp.hop[i])) * 8 / duration;
				double u = txRate / ih.hop[i].GetLineRate() + (double)std::min(ih.hop[i].GetQlen(), qp->hp.hop[i].GetQlen()) * qp->m_max_rate.GetBitRate() / ih.hop[i].GetLineRate() /qp->m_win;
				#if PRINT_LOG
				if (print)
					printf(" %.3lf %.3lf", txRate, u);
				#endif
				if (!m_multipleRate){
					// for aggregate (single R)
					if (u > U){
						U = u;
						dt = tau;
					}
				}else {
					// for per hop (per hop R)
					if (tau > qp->m_baseRtt)
						tau = qp->m_baseRtt;
					qp->hp.hopState[i].u = (qp->hp.hopState[i].u * (qp->m_baseRtt - tau) + u * tau) / double(qp->m_baseRtt);
				}
				//std::cout << ch.ack.seq << " " << u << " " << U << " " << duration << " " << txRate << " " << ih.hop[i].GetTime() << " " <<  ih.hop[i].GetBytes() << " " << ih.hop[i].GetQlen() << " " << ih.hop[i].GetTimeDelta(qp->hp.hop[i]) << " " << ih.hop[i].GetBytesDelta(qp->hp.hop[i]) <<  std::endl;
				qp->hp.hop[i] = ih.hop[i];
				
			}

			DataRate new_rate;
			int32_t new_incStage;
			DataRate new_rate_per_hop[IntHeader::maxHop];
			int32_t new_incStage_per_hop[IntHeader::maxHop];
			if (!m_multipleRate){
				//std::cout << "!m_multipleRate " << std::endl;
				// for aggregate (single R)
				if (updated_any){
					if (dt > qp->m_baseRtt)
						dt = qp->m_baseRtt;
					qp->hp.u = (qp->hp.u * (qp->m_baseRtt - dt) + U * dt) / double(qp->m_baseRtt);
					max_c = qp->hp.u / m_targetUtil;
					//std::cout << ch.ack.seq << " " << dt << " " << qp->m_baseRtt << " " << qp->hp.u  << " " <<  U << " " << max_c << std::endl;

					if (max_c >= 1 || qp->hp.m_incStage >= m_miThresh){
						new_rate = qp->hp.m_curRate / max_c + m_rai;
						new_incStage = 0;
					}else{
						new_rate = qp->hp.m_curRate + m_rai;
						new_incStage = qp->hp.m_incStage+1;
					}
					if (new_rate < m_minRate)
						new_rate = m_minRate;
					if (new_rate > qp->m_max_rate)
						new_rate = qp->m_max_rate;
					#if PRINT_LOG
					if (print)
						printf(" u=%.6lf U=%.3lf dt=%u max_c=%.3lf", qp->hp.u, U, dt, max_c);
					#endif
					#if PRINT_LOG
					if (print)
						printf(" rate:%.3lf->%.3lf\n", qp->hp.m_curRate.GetBitRate()*1e-9, new_rate.GetBitRate()*1e-9);
					#endif
				}
			}else{
				std::cout << "m_multipleRate " << std::endl;
				// for per hop (per hop R)
				new_rate = qp->m_max_rate;
				for (uint32_t i = 0; i < ih.nhop; i++){
					if (updated[i]){
						double c = qp->hp.hopState[i].u / m_targetUtil;
						if (c >= 1 || qp->hp.hopState[i].incStage >= m_miThresh){
							new_rate_per_hop[i] = qp->hp.hopState[i].Rc / c + m_rai;
							new_incStage_per_hop[i] = 0;
						}else{
							new_rate_per_hop[i] = qp->hp.hopState[i].Rc + m_rai;
							new_incStage_per_hop[i] = qp->hp.hopState[i].incStage+1;
						}
						// bound rate
						if (new_rate_per_hop[i] < m_minRate)
							new_rate_per_hop[i] = m_minRate;
						if (new_rate_per_hop[i] > qp->m_max_rate)
							new_rate_per_hop[i] = qp->m_max_rate;
						// find min new_rate
						if (new_rate_per_hop[i] < new_rate)
							new_rate = new_rate_per_hop[i];
						#if PRINT_LOG
						if (print)
							printf(" [%u]u=%.6lf c=%.3lf", i, qp->hp.hopState[i].u, c);
						#endif
						#if PRINT_LOG
						if (print)
							printf(" %.3lf->%.3lf", qp->hp.hopState[i].Rc.GetBitRate()*1e-9, new_rate.GetBitRate()*1e-9);
						#endif
					}else{
						if (qp->hp.hopState[i].Rc < new_rate)
							new_rate = qp->hp.hopState[i].Rc;
					}
				}
				#if PRINT_LOG
				printf("\n");
				#endif
			}
			if (updated_any)
				ChangeRate(qp, new_rate);
			if (!fast_react){
				if (updated_any){
					qp->hp.m_curRate = new_rate;
					qp->hp.m_incStage = new_incStage;
				}
				if (m_multipleRate){
					// for per hop (per hop R)
					for (uint32_t i = 0; i < ih.nhop; i++){
						if (updated[i]){
							qp->hp.hopState[i].Rc = new_rate_per_hop[i];
							qp->hp.hopState[i].incStage = new_incStage_per_hop[i];
						}
					}
				}
			}
		}
		if (!fast_react){
			if (next_seq > qp->hp.m_lastUpdateSeq)
				qp->hp.m_lastUpdateSeq = next_seq; //+ rand() % 2 * m_mtu;
		}
	}
}

void RdmaHw::FastReactHp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
	if (m_fast_react)
		UpdateRateHp(qp, p, ch, true);
}

/**********************
 * TIMELY
 *********************/
void RdmaHw::HandleAckTimely(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
	uint32_t ack_seq = ch.ack.seq;
	// update rate
	if (ack_seq > qp->tmly.m_lastUpdateSeq){ // if full RTT feedback is ready, do full update
		UpdateRateTimely(qp, p, ch, false);
	}else{ // do fast react
		FastReactTimely(qp, p, ch);
	}
}
void RdmaHw::UpdateRateTimely(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch, bool us){
	uint32_t next_seq = qp->snd_nxt;
	uint64_t rtt = Simulator::Now().GetTimeStep() - ch.ack.ih.ts;
	bool print = !us;
	if (qp->tmly.m_lastUpdateSeq != 0){ // not first RTT
		int64_t new_rtt_diff = (int64_t)rtt - (int64_t)qp->tmly.lastRtt;
		double rtt_diff = (1 - m_tmly_alpha) * qp->tmly.rttDiff + m_tmly_alpha * new_rtt_diff;
		double gradient = rtt_diff / m_tmly_minRtt;
		bool inc = false;
		double c = 0;
		#if PRINT_LOG
		if (print)
			printf("%lu node:%u rtt:%lu rttDiff:%.0lf gradient:%.3lf rate:%.3lf", Simulator::Now().GetTimeStep(), m_node->GetId(), rtt, rtt_diff, gradient, qp->tmly.m_curRate.GetBitRate() * 1e-9);
		#endif
		if (rtt < m_tmly_TLow){
			inc = true;
		}else if (rtt > m_tmly_THigh){
			c = 1 - m_tmly_beta * (1 - (double)m_tmly_THigh / rtt);
			inc = false;
		}else if (gradient <= 0){
			inc = true;
		}else{
			c = 1 - m_tmly_beta * gradient;
			if (c < 0)
				c = 0;
			inc = false;
		}
		if (inc){
			if (qp->tmly.m_incStage < 5){
				qp->m_rate = qp->tmly.m_curRate + m_rai;
			}else{
				qp->m_rate = qp->tmly.m_curRate + m_rhai;
			}
			if (qp->m_rate > qp->m_max_rate)
				qp->m_rate = qp->m_max_rate;
			if (!us){
				qp->tmly.m_curRate = qp->m_rate;
				qp->tmly.m_incStage++;
				qp->tmly.rttDiff = rtt_diff;
			}
		}else{
			qp->m_rate = std::max(m_minRate, qp->tmly.m_curRate * c); 
			if (!us){
				qp->tmly.m_curRate = qp->m_rate;
				qp->tmly.m_incStage = 0;
				qp->tmly.rttDiff = rtt_diff;
			}
		}
		#if PRINT_LOG
		if (print){
			printf(" %c %.3lf\n", inc? '^':'v', qp->m_rate.GetBitRate() * 1e-9);
		}
		#endif
	}
	if (!us && next_seq > qp->tmly.m_lastUpdateSeq){
		qp->tmly.m_lastUpdateSeq = next_seq;
		// update
		qp->tmly.lastRtt = rtt;
	}
}
void RdmaHw::FastReactTimely(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
}

/**********************
 * DCTCP
 *********************/
void RdmaHw::HandleAckDctcp(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
	uint32_t ack_seq = ch.ack.seq;
	uint8_t cnp = (ch.ack.flags >> qbbHeader::FLAG_CNP) & 1;
	bool new_batch = false;

	// update alpha
	qp->dctcp.m_ecnCnt += (cnp > 0);
	if (ack_seq > qp->dctcp.m_lastUpdateSeq){ // if full RTT feedback is ready, do alpha update
		#if PRINT_LOG
		printf("%lu %s %08x %08x %u %u [%u,%u,%u] %.3lf->", Simulator::Now().GetTimeStep(), "alpha", qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->dctcp.m_lastUpdateSeq, ch.ack.seq, qp->snd_nxt, qp->dctcp.m_alpha);
		#endif
		new_batch = true;
		if (qp->dctcp.m_lastUpdateSeq == 0){ // first RTT
			qp->dctcp.m_lastUpdateSeq = qp->snd_nxt;
			qp->dctcp.m_batchSizeOfAlpha = qp->snd_nxt / m_mtu + 1;
		}else {
			double frac = std::min(1.0, double(qp->dctcp.m_ecnCnt) / qp->dctcp.m_batchSizeOfAlpha);
			qp->dctcp.m_alpha = (1 - m_g) * qp->dctcp.m_alpha + m_g * frac;
			qp->dctcp.m_lastUpdateSeq = qp->snd_nxt;
			qp->dctcp.m_ecnCnt = 0;
			qp->dctcp.m_batchSizeOfAlpha = (qp->snd_nxt - ack_seq) / m_mtu + 1;
			#if PRINT_LOG
			printf("%.3lf F:%.3lf", qp->dctcp.m_alpha, frac);
			#endif
		}
		#if PRINT_LOG
		printf("\n");
		#endif
	}

	// check cwr exit
	if (qp->dctcp.m_caState == 1){
		if (ack_seq > qp->dctcp.m_highSeq)
			qp->dctcp.m_caState = 0;
	}

	// check if need to reduce rate: ECN and not in CWR
	if (cnp && qp->dctcp.m_caState == 0){
		#if PRINT_LOG
		printf("%lu %s %08x %08x %u %u %.3lf->", Simulator::Now().GetTimeStep(), "rate", qp->sip.Get(), qp->dip.Get(), qp->sport, qp->dport, qp->m_rate.GetBitRate()*1e-9);
		#endif
		qp->m_rate = std::max(m_minRate, qp->m_rate * (1 - qp->dctcp.m_alpha / 2));
		#if PRINT_LOG
		printf("%.3lf\n", qp->m_rate.GetBitRate() * 1e-9);
		#endif
		qp->dctcp.m_caState = 1;
		qp->dctcp.m_highSeq = qp->snd_nxt;
	}

	// additive inc
	if (qp->dctcp.m_caState == 0 && new_batch)
		qp->m_rate = std::min(qp->m_max_rate, qp->m_rate + m_dctcp_rai);
}

/*********************
 * HPCC-PINT
 ********************/
void RdmaHw::SetPintSmplThresh(double p){
       pint_smpl_thresh = (uint32_t)(65536 * p);
}
void RdmaHw::HandleAckHpPint(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch){
       uint32_t ack_seq = ch.ack.seq;
       if (rand() % 65536 >= pint_smpl_thresh)
               return;
       // update rate
       if (ack_seq > qp->hpccPint.m_lastUpdateSeq){ // if full RTT feedback is ready, do full update
               UpdateRateHpPint(qp, p, ch, false);
       }else{ // do fast react
               UpdateRateHpPint(qp, p, ch, true);
       }
}

void RdmaHw::UpdateRateHpPint(Ptr<RdmaQueuePair> qp, Ptr<Packet> p, CustomHeader &ch, bool fast_react){
       uint32_t next_seq = qp->snd_nxt;
       if (qp->hpccPint.m_lastUpdateSeq == 0){ // first RTT
               qp->hpccPint.m_lastUpdateSeq = next_seq;
       }else {
               // check packet INT
               IntHeader &ih = ch.ack.ih;
               double U = Pint::decode_u(ih.GetPower());

               DataRate new_rate;
               int32_t new_incStage;
               double max_c = U / m_targetUtil;

               if (max_c >= 1 || qp->hpccPint.m_incStage >= m_miThresh){
                       new_rate = qp->hpccPint.m_curRate / max_c + m_rai;
                       new_incStage = 0;
               }else{
                       new_rate = qp->hpccPint.m_curRate + m_rai;
                       new_incStage = qp->hpccPint.m_incStage+1;
               }
               if (new_rate < m_minRate)
                       new_rate = m_minRate;
               if (new_rate > qp->m_max_rate)
                       new_rate = qp->m_max_rate;
               ChangeRate(qp, new_rate);
               if (!fast_react){
                       qp->hpccPint.m_curRate = new_rate;
                       qp->hpccPint.m_incStage = new_incStage;
               }
               if (!fast_react){
                       if (next_seq > qp->hpccPint.m_lastUpdateSeq)
                               qp->hpccPint.m_lastUpdateSeq = next_seq; //+ rand() % 2 * m_mtu;
               }
       }
}

}
