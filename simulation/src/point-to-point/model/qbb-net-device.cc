/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
* Copyright (c) 2006 Georgia Tech Research Corporation, INRIA
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
* Author: Yuliang Li <yuliangli@g.harvard.com>
*/

#define __STDC_LIMIT_MACROS 1
#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "ns3/qbb-net-device.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/data-rate.h"
#include "ns3/object-vector.h"
#include "ns3/pause-header.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/assert.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/simulator.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/qbb-channel.h"
#include "ns3/random-variable.h"
#include "ns3/flow-id-tag.h"
#include "ns3/qbb-header.h"
#include "ns3/error-model.h"
#include "ns3/cn-header.h"
#include "ns3/ppp-header.h"
#include "ns3/empty-header.h"
#include "ns3/notify-header.h"
#include "ns3/notify-header-to-up.h"
#include "ns3/udp-header.h"
#include "ns3/seq-ts-header.h"
#include "ns3/pointer.h"
#include "ns3/custom-header.h"
#include "ns3/seq-port-header.h"

NS_LOG_COMPONENT_DEFINE("QbbNetDevice");

namespace ns3 {
	
	uint32_t RdmaEgressQueue::ack_q_idx = 3;
	uint32_t RdmaEgressQueue::m_RecMode = 0;
  	uint32_t RdmaEgressQueue::m_CcMode = 3;
	// RdmaEgressQueue
	TypeId RdmaEgressQueue::GetTypeId (void)
	{
		static TypeId tid = TypeId ("ns3::RdmaEgressQueue")
			.SetParent<Object> ()
			.AddTraceSource ("RdmaEnqueue", "Enqueue a packet in the RdmaEgressQueue.",
					MakeTraceSourceAccessor (&RdmaEgressQueue::m_traceRdmaEnqueue))
			.AddTraceSource ("RdmaDequeue", "Dequeue a packet in the RdmaEgressQueue.",
					MakeTraceSourceAccessor (&RdmaEgressQueue::m_traceRdmaDequeue))
			;
		return tid;
	}

	RdmaEgressQueue::RdmaEgressQueue(){
		m_rrlast = 0;
		m_qlast = 0;
		m_ackQ = CreateObject<DropTailQueue>();
		m_ackQ->SetAttribute("MaxBytes", UintegerValue(0xffffffff)); // queue limit is on a higher level, not here
	}

	Ptr<Packet> RdmaEgressQueue::DequeueQindex(int qIndex){
		if (qIndex == -1){ // high prio
			Ptr<Packet> p = m_ackQ->Dequeue();
			m_qlast = -1;
			m_traceRdmaDequeue(p, 0);
			return p;
		}
		if (qIndex >= 0){ // qp
			if(m_qpGrp->Get(qIndex)->halt_state && m_qpGrp->Get(qIndex)->probe_state){ //probe packet
				Ptr<Packet> p = m_rdmaGetPrbPkt(m_qpGrp->Get(qIndex));
				m_rrlast = qIndex;
				m_qlast = qIndex;
				m_traceRdmaDequeue(p, m_qpGrp->Get(qIndex)->m_pg);
				return p;
			}else if(!m_qpGrp->Get(qIndex)->halt_state){
				Ptr<Packet> p = m_rdmaGetNxtPkt(m_qpGrp->Get(qIndex));
				m_rrlast = qIndex;
				m_qlast = qIndex;
				m_traceRdmaDequeue(p, m_qpGrp->Get(qIndex)->m_pg);
				return p;
			}
		}
		return 0;
	}

	int RdmaEgressQueue::GetNextQindex(bool paused[]){ //flow or qp
		bool found = false;
		uint32_t qIndex;
		//std::cout << ack_q_idx << std::endl;
		if (!paused[ack_q_idx] && m_ackQ->GetNPackets() > 0)
			return -1;

		// no pkt in highest priority queue, do rr for each qp
		int res = -1024;
		uint32_t fcount = m_qpGrp->GetN();
		uint32_t min_finish_id = 0xffffffff;
		for (qIndex = 1; qIndex <= fcount; qIndex++){
			uint32_t idx = (qIndex + m_rrlast) % fcount; //flow is also rr
			Ptr<RdmaQueuePair> qp = m_qpGrp->Get(idx);

			//jia
			// ///*
			// if(m_RecMode == 0 && (m_CcMode == 3 || m_CcMode == 8))
			// {
			// 	if(qp->snd_nxt < qp->snd_una)
			// 	{
			// 		qp->snd_nxt = qp->snd_una;
			// 	}
			// }

			// //*/
			//0b010b01 0b009401 10000 100
			/*
			if(qp->sip.Get() == 0x0b010b01 && qp->dip.Get() == 0x0b009401 && qp->sport == 10000 && qp->dport ==100)
			{
				std::cout << "the qp pause " << paused[qp->m_pg] << " BytesLeft " << qp->GetBytesLeft() << " GetOnTheFly " << qp->GetOnTheFly() << " m_rate " << qp->m_rate //<< " WinBound " << qp->IsWinBound() 
				<< " snd_nxt " << qp->snd_nxt << " snd_una " << qp->snd_una << " m_win " << qp->m_win << " m_var_win " << qp->m_var_win 
				<< " wp " << qp->wp << " " ;
			}
			*/
			if (!paused[qp->m_pg] && qp->GetBytesLeft() > 0 && !qp->IsWinBound() && (!qp->halt_state || (qp->halt_state && qp->probe_state)))
			{
				//0b008c01 0b00dd01 10000 100 19000
				// if(qp->sip.Get() == 0x0b008c01 && qp->dip.Get() == 0x0b00dd01 && qp->sport == 10000 && qp->dport ==100)
				// {
				// 	std::cout <<"here" << std::endl;
				// }

				if (m_qpGrp->Get(idx)->m_nextAvail.GetTimeStep() > Simulator::Now().GetTimeStep()) //not available now
				{
					//0b010b01 0b009401 10000 100
					///*
					// if(qp->sip.Get() == 0x0b008c01 && qp->dip.Get() == 0x0b00dd01 && qp->sport == 10000 && qp->dport ==100)
					// {
					// 	std::cout << " not available now " << std::endl;
					// }
					//*/
					continue;
				}
				res = idx;
				break;
			}
			else if (qp->IsFinished()){
				min_finish_id = idx < min_finish_id ? idx : min_finish_id;
			}
		}

		// clear the finished qp
		if (min_finish_id < 0xffffffff){
			int nxt = min_finish_id;
			auto &qps = m_qpGrp->m_qps;
			for (int i = min_finish_id + 1; i < fcount; i++) if (!qps[i]->IsFinished()){
				if (i == res) // update res to the idx after removing finished qp
					res = nxt;
				qps[nxt] = qps[i];
				nxt++;
			}
			qps.resize(nxt);
		}
		return res;
	}

	int RdmaEgressQueue::GetLastQueue(){
		return m_qlast;
	}

	uint32_t RdmaEgressQueue::GetNBytes(uint32_t qIndex){
		NS_ASSERT_MSG(qIndex < m_qpGrp->GetN(), "RdmaEgressQueue::GetNBytes: qIndex >= m_qpGrp->GetN()");
		return m_qpGrp->Get(qIndex)->GetBytesLeft();
	}

	uint32_t RdmaEgressQueue::GetFlowCount(void){
		return m_qpGrp->GetN();
	}

	Ptr<RdmaQueuePair> RdmaEgressQueue::GetQp(uint32_t i){
		return m_qpGrp->Get(i);
	}
 
	void RdmaEgressQueue::RecoverQueue(uint32_t i){
		NS_ASSERT_MSG(i < m_qpGrp->GetN(), "RdmaEgressQueue::RecoverQueue: qIndex >= m_qpGrp->GetN()");
		m_qpGrp->Get(i)->snd_nxt = m_qpGrp->Get(i)->snd_una;
	}

	void RdmaEgressQueue::EnqueueHighPrioQ(Ptr<Packet> p){
		m_traceRdmaEnqueue(p, 0);
		m_ackQ->Enqueue(p);
	}

	void RdmaEgressQueue::CleanHighPrio(TracedCallback<Ptr<const Packet>, uint32_t> dropCb){
		while (m_ackQ->GetNPackets() > 0){
			Ptr<Packet> p = m_ackQ->Dequeue();
			dropCb(p, 0);
		}
	}

	/******************
	 * QbbNetDevice
	 *****************/
	NS_OBJECT_ENSURE_REGISTERED(QbbNetDevice);

	TypeId
		QbbNetDevice::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::QbbNetDevice")
			.SetParent<PointToPointNetDevice>()
			.AddConstructor<QbbNetDevice>()
			.AddAttribute("QbbEnabled",
				"Enable the generation of PAUSE packet.",
				BooleanValue(true),
				MakeBooleanAccessor(&QbbNetDevice::m_qbbEnabled),
				MakeBooleanChecker())
			.AddAttribute("QcnEnabled",
				"Enable the generation of PAUSE packet.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_qcnEnabled),
				MakeBooleanChecker())
			.AddAttribute("DynamicThreshold",
				"Enable dynamic threshold.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_dynamicth),
				MakeBooleanChecker())
			.AddAttribute("PauseTime",
				"Number of microseconds to pause upon congestion",
				UintegerValue(5),
				MakeUintegerAccessor(&QbbNetDevice::m_pausetime),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("RecMode",
				"Rec Mode",
				UintegerValue(0),
				MakeUintegerAccessor(&QbbNetDevice::m_RecMode),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("CcMode",
				"Cc Mode",
				UintegerValue(0),
				MakeUintegerAccessor(&QbbNetDevice::m_CcMode),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute ("TxBeQueue", 
					"A queue to use as the transmit queue in the device.",
					PointerValue (),
					MakePointerAccessor (&QbbNetDevice::m_queue),
					MakePointerChecker<Queue> ())
			.AddAttribute ("RdmaEgressQueue", 
					"A queue to use as the transmit queue in the device.",
					PointerValue (),
					MakePointerAccessor (&QbbNetDevice::m_rdmaEQ),
					MakePointerChecker<Object> ())
			.AddTraceSource ("QbbEnqueue", "Enqueue a packet in the QbbNetDevice.",
					MakeTraceSourceAccessor (&QbbNetDevice::m_traceEnqueue))
			.AddTraceSource ("QbbDequeue", "Dequeue a packet in the QbbNetDevice.",
					MakeTraceSourceAccessor (&QbbNetDevice::m_traceDequeue))
			.AddTraceSource ("QbbDrop", "Drop a packet in the QbbNetDevice.",
					MakeTraceSourceAccessor (&QbbNetDevice::m_traceDrop))
			.AddTraceSource ("RdmaQpDequeue", "A qp dequeue a packet.",
					MakeTraceSourceAccessor (&QbbNetDevice::m_traceQpDequeue))
			.AddTraceSource ("QbbPfc", "get a PFC packet. 0: resume, 1: pause",
					MakeTraceSourceAccessor (&QbbNetDevice::m_tracePfc))
			;

		return tid;
	}

	QbbNetDevice::QbbNetDevice()
	{
		NS_LOG_FUNCTION(this);
		normal_last = 0;
		sample_time = NanoSeconds(2000002000);
 		should_sample = true;
		m_ecn_source = new std::vector<ECNAccount>;
		for (uint32_t i = 0; i < qCnt; i++){
			m_paused[i] = false;
		}

		m_rdmaEQ = CreateObject<RdmaEgressQueue>();
	}

	QbbNetDevice::~QbbNetDevice()
	{
		NS_LOG_FUNCTION(this);
	}

	void
		QbbNetDevice::DoDispose()
	{
		NS_LOG_FUNCTION(this);

		PointToPointNetDevice::DoDispose();
	}

	void
		QbbNetDevice::TransmitComplete(void) //6
	{
		NS_LOG_FUNCTION(this);
		NS_ASSERT_MSG(m_txMachineState == BUSY, "Must be BUSY if transmitting");
		m_txMachineState = READY;
		NS_ASSERT_MSG(m_currentPkt != 0, "QbbNetDevice::TransmitComplete(): m_currentPkt zero");
		m_phyTxEndTrace(m_currentPkt);
		m_currentPkt = 0;
		DequeueAndTransmit();
	}

	void QbbNetDevice::TransmitCompleteToRecirculate(void){
		NS_ASSERT_MSG(m_txMachineState == BUSY, "Must be BUSY if transmitting");
		m_txMachineState = READY;
		m_currentPkt = 0;
		DequeueAndTransmit();
	}
/*
	void QbbNetDevice::AddSeqHeader (Ptr<Packet> p, uint16_t seqnumber)
	{
  		SeqHeader Seq;
  		Seq.SetSeq (seqnumber);
  		p->AddHeader (Seq);
	}

	//remove seq
	bool QbbNetDevice::ProcessSeqHeader (Ptr<Packet> p, uint16_t& seqparam)
	{
  		SeqHeader Seq;
  		p->RemoveHeader (Seq);
  		seqparam = Seq.GetSeq ();
  		return true;
	}
*/
	void
		QbbNetDevice::DequeueAndTransmit(void)
	{
		NS_LOG_FUNCTION(this);
		if (!m_linkUp) return; // if link is down, return
		if (m_txMachineState == BUSY) return;	// Quit if channel busy
		Ptr<Packet> p;
		if (m_node->GetNodeType() == 0){
			int qIndex = m_rdmaEQ->GetNextQindex(m_paused);
			if (qIndex != -1024){
				if (qIndex == -1){ // high prio maybe ack
					p = m_rdmaEQ->DequeueQindex(qIndex);
					m_traceDequeue(p, 0);
					TransmitStart(p);
					return;
				}
				// a qp dequeue a packet
				Ptr<RdmaQueuePair> lastQp = m_rdmaEQ->GetQp(qIndex);
				p = m_rdmaEQ->DequeueQindex(qIndex);

				// transmit
				m_traceQpDequeue(p, lastQp);
				TransmitStart(p);

				// update for the next avail time
				m_rdmaPktSent(lastQp, p, m_tInterframeGap);
			}else { // no packet to send
				NS_LOG_INFO("PAUSE prohibits send at node " << m_node->GetId());
				Time t = Simulator::GetMaximumSimulationTime();
				for (uint32_t i = 0; i < m_rdmaEQ->GetFlowCount(); i++){
					Ptr<RdmaQueuePair> qp = m_rdmaEQ->GetQp(i);
					if (qp->GetBytesLeft() == 0)
						continue;
					t = Min(qp->m_nextAvail, t);
				}
				if (m_nextSend.IsExpired() && t < Simulator::GetMaximumSimulationTime() && t > Simulator::Now()){
					m_nextSend = Simulator::Schedule(t - Simulator::Now(), &QbbNetDevice::DequeueAndTransmit, this);
				}
			}
			return;
		}else{   //switch, doesn't care about qcn, just send

//self-replenishing queue of dummy packets
			// if(m_RecMode == 1){ 
			// 	if(m_node->GetId() == 2 && GetIfIndex() == 2){ //指定链路发送空数据包为了防止tail packet
			// 		if(m_channel->GetQbbDevice(0) != this) //to qbb
			// 		{
			// 			Ptr<QbbNetDevice> tmp = m_channel->GetQbbDevice (0);
			// 			if(tmp->m_node->GetNodeType() == 1) //to is sw
			// 			{
			// 				//std::cout << "to is sw" << std::endl;
			// 				if(m_queue->GetNBytes(7) == 0)
			// 				{
			// 					//std::cout << "low prio is empty" << std::endl;
			// 					ProduceEmptyP();
			// 				}
			// 				else
			// 				{
			// 					//std::cout << "low prio is not empty" << std::endl;
			// 				}
			// 			}
			// 		}
			// 		else if(m_channel->GetQbbDevice(1) != this) //to qbb
			// 		{
			// 			Ptr<QbbNetDevice> tmp = m_channel->GetQbbDevice (1);
			// 			if(tmp->m_node->GetNodeType() == 1) //to is sw
			// 			{
			// 				//std::cout << "to is sw" << std::endl;
			// 				if(m_queue->GetNBytes(7) == 0)
			// 				{
			// 					//std::cout << "low prio is empty" << std::endl;
			// 					ProduceEmptyP();
			// 				}
			// 				else
			// 				{
			// 					//std::cout << "low prio is not empty" << std::endl;
			// 				}
			// 			}
			// 		}
			// 	}
			// }
			// else if(m_RecMode == 2){
			// 	if(m_node->GetId() == 2 && GetIfIndex() == 2){ //指定链路发送空数据包为了防止tail packet
			// 		if(m_queue->GetNBytes(7) == 0){
			// 			ProduceEmptyP();
			// 		}
			// 	}
			// 	else if(m_node->GetId() == 3 && GetIfIndex() == 1){ //指定链路发送ack给上游目的是及时clear copybuffer
			// 		if(m_queue->GetNBytes(7) == 0){
			// 			ProduceEmptyAckP();
			// 		}
			// 	}
			// }

			p = m_queue->DequeueRR(m_paused);		//this is round-robin
			if (p != 0){
				m_snifferTrace(p);
				m_promiscSnifferTrace(p);
				Ipv4Header h;
				Ptr<Packet> packet = p->Copy();
				uint16_t protocol = 0;
				ProcessHeader(packet, protocol);
				packet->RemoveHeader(h); 
				FlowIdTag t;
				uint32_t qIndex = m_queue->GetLastQueue();

				normal_last = 1;
				
				if (qIndex == 0){//this is a pause or cnp, send it immediately!
					m_node->SwitchNotifyDequeue(m_ifIndex, qIndex, p);
					p->RemovePacketTag(t);
				}else{
					// if(qIndex == 7) //empty p dequeue
					// {
					// 	m_node->SwitchNotifyDequeue(m_ifIndex, qIndex, p);
					// }else{
						// if(m_node->GetId() == 3 && m_ifIndex == 2){ //only this link
						// 	CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
						// 	p->PeekHeader(ch);
						// 	if(ch.l3Prot == 0x11){
						// 		// if((ch.udp.flags >> SeqTsHeader::FLAG_REORDERINGRECIRCULATE) & 1 == 1){ //need reordering recir data p
						// 		// 	std::cout << "qbb udp need reordering recir data p " << Simulator::Now() << " " << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.sequence << std::endl;
						// 		// 	TramsmitStartToRecirculate(p);
						// 		// 	return;
						// 		// }
						// 		// else{
						// 			std::cout << Simulator::Now() << " " << "qbb udp need send " << ch.udp.sport << " " << ch.udp.dport << " " << ch.udp.seq << " " << ch.udp.sequence << std::endl;
						// 		}
						// 	// }else if(ch.l3Prot == 0xF9){ //need reordering recir empty data p
						// 	// 	std::cout << "qbb empty need reordering recir empty p " << Simulator::Now() << " " << ch.empty.empty_seq << std::endl;
						// 	// 	TramsmitStartToRecirculate(p);
						// 	// 	return;
						// 	// }
						// }
						m_node->SwitchNotifyDequeue(m_ifIndex, qIndex, p);
						p->RemovePacketTag(t);
					//}
				}
				m_traceDequeue(p, qIndex);
				TransmitStart(p);
				return;
			}else{ //No queue can deliver any packet
//mine
				if(m_RecMode == 1){ 
					//if(m_node->GetId() == 2 && GetIfIndex() == 2){ //指定链路发送空数据包为了防止tail packet
					if(m_node->ToIsSwitch(m_ifIndex)){
						if(normal_last == 1){
							Ptr<Packet> p = ProduceEmptyP();
							normal_last = 0;
							m_node->SwitchNotifyDequeue(m_ifIndex, 7, p);
							TransmitStart(p);
							return;
						}
					}	
				}
				else if(m_RecMode == 2){
					// if(m_node->GetId() == 2 && GetIfIndex() == 2){ //指定链路发送空数据包为了防止tail packet
				// 	if((m_node->GetId() == 340 && GetIfIndex() == 1) ||
			    // (m_node->GetId() == 347 && GetIfIndex() == 2) ||
				// (m_node->GetId() == 324 && GetIfIndex() == 2) ||
				// (m_node->GetId() == 333 && GetIfIndex() == 4) ||
				// (m_node->GetId() == 354 && GetIfIndex() == 6) ||
				// (m_node->GetId() == 351 && GetIfIndex() == 5)){

					if(m_node->ToIsSwitch(m_ifIndex)){
						// if(normal_last == 1){
						// 	Ptr<Packet> p1 = ProduceEmptyP();
						// 	normal_last = 0;
						// 	m_node->SwitchNotifyDequeue(m_ifIndex, 7, p);
						// 	TransmitStart(p);
						// 	// return;
						// }

						if(m_node->GetPendingack(GetIfIndex()) == 1){ //only pending
							Ptr<Packet> p2 = ProduceEmptyAckP();
							m_node->SwitchNotifyDequeue(m_ifIndex, 7, p2);
							TransmitStart(p2);
							return;
						}

						//return;
					}
					//else if(m_node->GetId() == 3 && GetIfIndex() == 1){ //指定链路发送ack给上游目的是及时clear copybuffer
			// 		else if((m_node->GetId() == 360 && GetIfIndex() == 1) ||
	   	   	//    (m_node->GetId() == 373 && GetIfIndex() == 2) || 
	        //    (m_node->GetId() == 345 && GetIfIndex() == 5) ||
	        //    (m_node->GetId() == 355 && GetIfIndex() == 6) ||
	        //    (m_node->GetId() == 333 && GetIfIndex() == 3) ||
	        //    (m_node->GetId() == 328 && GetIfIndex() == 4)){
					// if(m_node->ToIsSwitch(m_ifIndex)){
					// 	if(m_node->GetPendingack(GetIfIndex()) == 1){ //only pending
					// 		Ptr<Packet> p = ProduceEmptyAckP();
					// 		m_node->SwitchNotifyDequeue(m_ifIndex, 7, p);
					// 		TransmitStart(p);
					// 		return;
					// 	}
					// }
				}
				NS_LOG_INFO("PAUSE prohibits send at node " << m_node->GetId());
				//std::cout << m_node->GetId() << " no queue can deliver any packet" << std::endl;
				// just sever
				if (m_node->GetNodeType() == 0 && m_qcnEnabled){ //nothing to send, possibly due to qcn flow control, if so reschedule sending
					Time t = Simulator::GetMaximumSimulationTime();
					for (uint32_t i = 0; i < m_rdmaEQ->GetFlowCount(); i++){
						Ptr<RdmaQueuePair> qp = m_rdmaEQ->GetQp(i);
						if (qp->GetBytesLeft() == 0)
							continue;
						t = Min(qp->m_nextAvail, t); //t = qp->m_nextAvail
					}
					if (m_nextSend.IsExpired() && t < Simulator::GetMaximumSimulationTime() && t > Simulator::Now()){
						m_nextSend = Simulator::Schedule(t - Simulator::Now(), &QbbNetDevice::DequeueAndTransmit, this);
					}
				}
			}
		}
		return;
	}

	void
		QbbNetDevice::Resume(unsigned qIndex)
	{
		NS_LOG_FUNCTION(this << qIndex);
		NS_ASSERT_MSG(m_paused[qIndex], "Must be PAUSEd");
		m_paused[qIndex] = false;
		NS_LOG_INFO("Node " << m_node->GetId() << " dev " << m_ifIndex << " queue " << qIndex <<
			" resumed at " << Simulator::Now().GetSeconds());
		DequeueAndTransmit();
	}

	void
		QbbNetDevice::Receive(Ptr<Packet> packet)
	{
		//std::cout << m_node->GetId() << " " << m_ifIndex << std::endl;
		NS_LOG_FUNCTION(this << packet);
		if (!m_linkUp){
			m_traceDrop(packet, 0);
			return;
		}

		if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt(packet))
		{
			// 
			// If we have an error model and it indicates that it is time to lose a
			// corrupted packet, don't forward this packet up, let it go.
			//
			m_phyRxDropTrace(packet);
			return;
		}
		m_macRxTrace(packet);
		CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
		ch.getInt = 1; // parse INT header
		packet->PeekHeader(ch); //extract ch
		if (ch.l3Prot == 0xFE){ // PFC
			if (!m_qbbEnabled) return;
			unsigned qIndex = ch.pfc.qIndex;
			if (ch.pfc.time > 0){
				std::cout << Simulator::Now() << " receive pfcpause " << m_node->GetId() << " " << m_ifIndex << std::endl;
				m_tracePfc(1);
				m_paused[qIndex] = true;
			}else{
				std::cout << Simulator::Now() <<" receive pfcresume " << m_node->GetId() << " " << m_ifIndex << std::endl;
				m_tracePfc(0);
				Resume(qIndex);
			}
		}else if(ch.l3Prot == 0xF7){ // pause
			unsigned qIndex = ch.lgpause.qIndex;
			if(ch.lgpause.lgtype == 1){
				std::cout << Simulator::Now() << " receive lgpause " << m_node->GetId() << " " << m_ifIndex << std::endl;
				m_paused[qIndex] = true;
			}else{
				std::cout << Simulator::Now() << " receive lgresume " << m_node->GetId() << " " << m_ifIndex << std::endl;
				m_paused[qIndex] = false;
				DequeueAndTransmit();
			}
		}else if(ch.l3Prot == 0xFA){ //notifytoup, the up receive then to produce a notify to sender
			//n
			if(m_RecMode == 1){
				m_node->SendNotifyToSender(m_ifIndex, ch.notifytoup.m_notifyleft, ch.notifytoup.m_notifyright);
			}
			//r
			else if(m_RecMode == 2){
				//std::cout << "receive FA " << m_ifIndex << " " << ch.notifytoup.m_notifyleft << " " << ch.notifytoup.m_notifyright << std::endl;
				m_node->UpdateReTxReqs(m_ifIndex, ch.notifytoup.m_notifyleft, ch.notifytoup.m_notifyright);
			}
		}else if(ch.l3Prot == 0xFB){ //notify, that is to sender
			if(m_node->GetNodeType() > 0) //switch, only transmit, like ack
			{
				//std::cout << m_node->GetId() << " " << m_ifIndex << " " << ch.notify.m_notifyleft << std::endl;
				packet->AddPacketTag(FlowIdTag(m_ifIndex));
				m_node->SwitchReceiveFromDevice(this, packet, ch);
			}
			else //nic
			{
				int ret = m_rdmaReceiveCb(packet, ch);
			}
		}else if(ch.l3Prot == 0xF9){ //empty data p
			if (m_node->GetNodeType() > 0){ // switch
				packet->AddPacketTag(FlowIdTag(m_ifIndex));
				m_node->SwitchReceiveFromDevice(this, packet, ch);
			}
			else{
				return; //sever no care
			}
		}else if(ch.l3Prot == 0xF8){ //empty ack p
			if (m_node->GetNodeType() > 0){ // switch
				packet->AddPacketTag(FlowIdTag(m_ifIndex));
				m_node->SwitchReceiveFromDevice(this, packet, ch);
			}
			else{
				return; //sever no care
			}
		}else { // non-PFC packets (data, ACK, NACK, CNP...)
			if (m_node->GetNodeType() > 0){ // switch
				packet->AddPacketTag(FlowIdTag(m_ifIndex));
				m_node->SwitchReceiveFromDevice(this, packet, ch);
		}else { // NIC
				// send to RdmaHw
/*
				if(ch.l3Prot == 0x11)
				{
					SeqHeader seqh;
					packet->RemoveHeader(seqh);
				}
*/
				int ret = m_rdmaReceiveCb(packet, ch);
				// TODO we may based on the ret do something
			}
		}
		return;
	}

	bool QbbNetDevice::Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber) //no
	{
		return false;
	}

	bool QbbNetDevice::SwitchSend (uint32_t qIndex, Ptr<Packet> packet, CustomHeader &ch){
		m_macTxTrace(packet);
		m_traceEnqueue(packet, qIndex);
		m_queue->Enqueue(packet, qIndex);
		if(m_RecMode == 2 && m_node->GetId() == 2 && m_ifIndex == 2){
			// if((m_node->GetId() == 340 && GetIfIndex() == 1) ||
			//     (m_node->GetId() == 347 && GetIfIndex() == 2) ||
			// 	(m_node->GetId() == 324 && GetIfIndex() == 2) ||
			// 	(m_node->GetId() == 333 && GetIfIndex() == 4) ||
			// 	(m_node->GetId() == 354 && GetIfIndex() == 6) ||
			// 	(m_node->GetId() == 351 && GetIfIndex() == 5)){
			// if(Simulator::Now().GetTimeStep() > sample_time.GetTimeStep()){
			// 	if(should_sample == true){
					
			// 	}
				// std::ofstream fct_output_finish("small_30load_up_qlength.txt", std::ofstream::app);
				// fct_output_finish << Simulator::Now() << " " << m_queue->GetNBytes(3) / 1024.0 << std::endl;
				// fct_output_finish.close();
			//}
			
			// FILE *up_qlength = fopen("up_qlength_file.txt", "w");
			// fprintf(up_qlength, "%lu %f\n", Simulator::Now().GetTimeStep(), m_queue->GetNBytes(3) / 1024.0);
			// fflush(up_qlength);
			// if(m_queue->GetNBytes(3) > m_node->qlength){
			// 	m_node->qlength = m_queue->GetNBytes(3);
			// }
			//std::cout << "up qlength " << Simulator::Now() << " " << m_queue->GetNBytes(3) << std::endl;
		}
		// if(m_node->GetId() == 3 && m_ifIndex == 2){
		// 	if(m_queue->GetNBytes(3) > m_node->qlength){
		// 		m_node->qlength = m_queue->GetNBytes(3);
		// 	}
		// 	//std::cout << "down qlength " << Simulator::Now() << " " << m_queue->GetNBytes(3) << std::endl;
		// }
		DequeueAndTransmit();
		
		return true;
	}

	void QbbNetDevice::SendPfc(uint32_t qIndex, uint32_t type){
		Ptr<Packet> p = Create<Packet>(0);
		PauseHeader pauseh((type == 0 ? m_pausetime : 0), m_queue->GetNBytes(qIndex), qIndex);
		p->AddHeader(pauseh);
		Ipv4Header ipv4h;  // Prepare IPv4 header
		ipv4h.SetProtocol(0xFE);
		ipv4h.SetSource(m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal());
		ipv4h.SetDestination(Ipv4Address("255.255.255.255"));
		ipv4h.SetPayloadSize(p->GetSize());
		ipv4h.SetTtl(1);
		ipv4h.SetIdentification(UniformVariable(0, 65536).GetValue());
		p->AddHeader(ipv4h);
		AddHeader(p, 0x800);
		CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
		p->PeekHeader(ch);
		m_node->pfc_times++;
		//std::cout << "pfc is work " << m_node->GetId() << " " << m_ifIndex << std::endl;
		SwitchSend(0, p, ch);
	}
//in-network notification
	void QbbNetDevice::SendNotifyToUp(uint16_t left, uint16_t right)
	{
		Ptr<Packet> notifypacket1 = Create<Packet>(0);
		NotifyHeaderToUp notifyh1(left, right);
		notifypacket1->AddHeader(notifyh1);
		Ipv4Header ipv41;
		ipv41.SetProtocol(0xFA);
		ipv41.SetSource(m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal());
		ipv41.SetDestination(Ipv4Address("255.255.255.255"));
		ipv41.SetPayloadSize(notifypacket1->GetSize());
		ipv41.SetTtl(1);
		ipv41.SetIdentification(UniformVariable(0, 65536).GetValue());
		notifypacket1->AddHeader(ipv41);
		AddHeader(notifypacket1, 0x0800);
		CustomHeader ch1(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
		notifypacket1->PeekHeader(ch1);		
		SwitchSend(0, notifypacket1, ch1);
	}

	void QbbNetDevice::SendPauseToUp(uint16_t lgtype, uint8_t qindex)
	{
		Ptr<Packet> p = Create<Packet>(0);
		LGPauseHeader lgpauseh(lgtype, qindex);
		p->AddHeader(lgpauseh);
		Ipv4Header ipv4h;  // Prepare IPv4 header
		ipv4h.SetProtocol(0xF7);
		ipv4h.SetSource(m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal());
		ipv4h.SetDestination(Ipv4Address("255.255.255.255"));
		ipv4h.SetPayloadSize(p->GetSize());
		ipv4h.SetTtl(1);
		ipv4h.SetIdentification(UniformVariable(0, 65536).GetValue());
		p->AddHeader(ipv4h);
		AddHeader(p, 0x800);
		CustomHeader ch(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
		p->PeekHeader(ch);
		m_node->lgpause_times++;
		//std::cout << "lgpause is work " << Simulator::Now() << std::endl;
		SwitchSend(0, p, ch);
	}
	
	Ptr<Packet> QbbNetDevice::ProduceEmptyP()
	{
		Ptr<Packet> p = Create<Packet>(0);
		EmptyHeader emptyh;
		emptyh.SetSeq(0);
		emptyh.SetPendingAckFlags(0);
		p->AddHeader(emptyh);
		Ipv4Header ipv4h;
		ipv4h.SetProtocol(0xF9); //data dummy p
		ipv4h.SetSource(m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal());
		ipv4h.SetDestination(Ipv4Address("255.255.255.255"));
		ipv4h.SetPayloadSize(p->GetSize());
		ipv4h.SetTtl(1);
		ipv4h.SetIdentification(UniformVariable(0, 65536).GetValue());
		p->AddHeader(ipv4h);
		AddHeader(p, 0x800);	
		//m_queue->Enqueue(p, 7);
		return p;
	}

	Ptr<Packet> QbbNetDevice::ProduceEmptyAckP()
	{
		Ptr<Packet> p = Create<Packet>(0);
		EmptyHeader emptyh;
		emptyh.SetSeq(0);
		emptyh.SetPendingAckFlags(0);
		p->AddHeader(emptyh);
		Ipv4Header ipv4h;
		ipv4h.SetProtocol(0xF8); //ack dummy p
		ipv4h.SetSource(m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal());
		ipv4h.SetDestination(Ipv4Address("255.255.255.255"));
		ipv4h.SetPayloadSize(p->GetSize());
		ipv4h.SetTtl(1);
		ipv4h.SetIdentification(UniformVariable(0, 65536).GetValue());
		p->AddHeader(ipv4h);
		AddHeader(p, 0x800);	
		//m_queue->Enqueue(p, 7);
		return p;
	}
/*
	void QbbNetDevice::SendNotifyToSender(uint32_t nextseq, Ptr<Packet> p, CustomHeader &ch)
	{
		Ptr<Packet> notifypacket1 = Create<Packet>(0);
		NotifyHeader notifyh1(nextseq, ch.udp.sport, 0);
		notifypacket1->AddHeader(notifyh1);
		Ipv4Header ipv41;
		ipv41.SetProtocol(0xFB);
		ipv41.SetSource(Ipv4Address(ch.dip));
		//ipv41.SetSource(m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal());
		ipv41.SetDestination(Ipv4Address(ch.sip));
		ipv41.SetPayloadSize(notifypacket1->GetSize());
		ipv41.SetTtl(64);
		ipv41.SetIdentification(UniformVariable(0, 65536).GetValue());
		notifypacket1->AddHeader(ipv41);
		AddHeader(notifypacket1, 0x0800);
		CustomHeader ch1(CustomHeader::L2_Header | CustomHeader::L3_Header | CustomHeader::L4_Header);
		notifypacket1->PeekHeader(ch1);	

		//this->RdmaEnqueueHighPrioQ(notifypacket1);
		//this->TriggerTransmit();
		SwitchSend(0, notifypacket1, ch1);
	}
*/
	bool
		QbbNetDevice::Attach(Ptr<QbbChannel> ch)
	{
		NS_LOG_FUNCTION(this << &ch);
		m_channel = ch;
		m_channel->Attach(this);
		NotifyLinkUp();
		return true;
	}

	bool
		QbbNetDevice::TransmitStart(Ptr<Packet> p)
	{
		NS_LOG_FUNCTION(this << p);
		NS_LOG_LOGIC("UID is " << p->GetUid() << ")");
		//
		// This function is called to start the process of transmitting a packet.
		// We need to tell the channel that we've started wiggling the wire and
		// schedule an event that will be executed when the transmission is complete.
		//
		NS_ASSERT_MSG(m_txMachineState == READY, "Must be READY to transmit");
		m_txMachineState = BUSY;
		m_currentPkt = p;
		m_phyTxBeginTrace(m_currentPkt);
		Time txTime = Seconds(m_bps.CalculateTxTime(p->GetSize()));
		Time txCompleteTime = txTime + m_tInterframeGap;
		//std::cout << txTime << " " << p->GetSize() << " " << m_bps << std::endl;
		NS_LOG_LOGIC("Schedule TransmitCompleteEvent in " << txCompleteTime.GetSeconds() << "sec");
		Simulator::Schedule(txCompleteTime, &QbbNetDevice::TransmitComplete, this);

		bool result = m_channel->TransmitStart(p, this, txTime);
		if (result == false)
		{
			m_phyTxDropTrace(p);

		}
		return result;
	}

	bool QbbNetDevice::TramsmitStartToRecirculate (Ptr<Packet> p){
		NS_ASSERT_MSG(m_txMachineState == READY, "Must be READY to transmit");
		m_txMachineState = BUSY;
		Time txTime = Seconds(m_bps.CalculateTxTime(p->GetSize()));
		Time txCompleteTime = txTime + m_tInterframeGap;
		Simulator::Schedule(txCompleteTime, &QbbNetDevice::TransmitCompleteToRecirculate, this);

		Time recirTime = txTime + NanoSeconds(500);
		//std::cout << p->GetSize() << " " << txTime << " " << recirTime << std::endl;
		Simulator::Schedule(recirTime, &Node::EnReorderingQueue, m_node, p, 1);

		return true;

	}
	// void QbbNetDevice::EnqueueReorderingRecirculateQueue(Ptr<Packet> p, uint16_t port){
	// 	m_node
	// }

	Ptr<Channel>
		QbbNetDevice::GetChannel(void) const
	{
		return m_channel;
	}

   bool QbbNetDevice::IsQbb(void) const{
	   return true;
   }

   void QbbNetDevice::NewQp(Ptr<RdmaQueuePair> qp){
	   qp->m_nextAvail = Simulator::Now();
	   DequeueAndTransmit();
   }
   void QbbNetDevice::ReassignedQp(Ptr<RdmaQueuePair> qp){
	   DequeueAndTransmit();
   }
   void QbbNetDevice::TriggerTransmit(void){
	   DequeueAndTransmit();
   }

	void QbbNetDevice::SetQueue(Ptr<BEgressQueue> q){
		NS_LOG_FUNCTION(this << q);
		m_queue = q;
	}

	Ptr<BEgressQueue> QbbNetDevice::GetQueue(){
		return m_queue;
	}

	Ptr<RdmaEgressQueue> QbbNetDevice::GetRdmaQueue(){
		return m_rdmaEQ;
	}

	void QbbNetDevice::RdmaEnqueueHighPrioQ(Ptr<Packet> p){
		m_traceEnqueue(p, 0);
		m_rdmaEQ->EnqueueHighPrioQ(p);
	}

	void QbbNetDevice::TakeDown(){
		// TODO: delete packets in the queue, set link down
		if (m_node->GetNodeType() == 0){
			// clean the high prio queue
			m_rdmaEQ->CleanHighPrio(m_traceDrop);
			// notify driver/RdmaHw that this link is down
			m_rdmaLinkDownCb(this);
		}else { // switch
			// clean the queue
			for (uint32_t i = 0; i < qCnt; i++)
				m_paused[i] = false;
			while (1){
				Ptr<Packet> p = m_queue->DequeueRR(m_paused);
				if (p == 0)
					 break;
				m_traceDrop(p, m_queue->GetLastQueue());
			}
			// TODO: Notify switch that this link is down
		}
		m_linkUp = false;
	}

	void QbbNetDevice::UpdateNextAvail(Time t){
		if (!m_nextSend.IsExpired() && t < m_nextSend.GetTs()){
			Simulator::Cancel(m_nextSend);
			Time delta = t < Simulator::Now() ? Time(0) : t - Simulator::Now();
			m_nextSend = Simulator::Schedule(delta, &QbbNetDevice::DequeueAndTransmit, this);
		}
	}
} // namespace ns3
