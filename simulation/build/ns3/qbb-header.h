//yibo

#ifndef QBB_HEADER_H
#define QBB_HEADER_H

#include <stdint.h>
#include "ns3/header.h"
#include "ns3/buffer.h"
#include "ns3/int-header.h"
#include "ns3/mine-ack-header.h"

namespace ns3 {

/**
 * \ingroup Pause
 * \brief Header for the Congestion Notification Message
 *
 * This class has two fields: The five-tuple flow id and the quantized
 * congestion level. This can be serialized to or deserialzed from a byte
 * buffer.
 */
 
class qbbHeader : public Header
{
public:
 
  enum {
	  FLAG_CNP = 0,
    FLAG_LAST = 7,
    FLAG_PENDINGACK = 8,
    FLAG_COPYRECIRCULATE = 9,
    FLAG_REORDERINGRECIRCULATE = 10,
    FLAG_SWITCHNACK = 15
  };
  qbbHeader (uint16_t pg);
  qbbHeader ();
  virtual ~qbbHeader ();

//Setters
  /**
   * \param pg The PG
   */
  void SetPG (uint16_t pg);
  void SetSeq(uint32_t seq);
  void SetSport(uint32_t _sport);
  void SetDport(uint32_t _dport);
  void SetTs(uint64_t ts);
  void SetCnp();
  void SetLast();
  void SetPendingAck();
  void SetCopyRecirculate();
  void SetReorderingRecirculate();
  void SetSwitchNACK();
  void SetSequence(uint16_t sequence);
  void SetlatestRxSeqNo(uint16_t latestRxSeqNo);
  void SetIntHeader(const IntHeader &_ih);

//Getters
  /**
   * \return The pg
   */
  uint16_t GetPG () const;
  uint32_t GetSeq() const;
  uint16_t GetSport() const;
  uint16_t GetDport() const;
  uint64_t GetTs() const;
  uint8_t GetCnp() const;
  uint8_t GetLast() const;
  uint8_t GetPendingAck() const;
  uint8_t GetCopyRecirculate() const;
  uint8_t GetReorderingRecirculate() const;
  uint8_t GetSwitchNACK() const;
  uint16_t GetSequence() const;
  uint16_t GetlatestRxSeqNo() const;

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  static uint32_t GetBaseSize(); // size without INT

private:
  uint16_t sport, dport;
  uint16_t flags;
  uint16_t m_pg;
  uint32_t m_seq; // the qbb sequence number.
  uint16_t m_sequence;
  uint16_t m_latestRxSeqNo;
  //MineAckHeader mah;
  IntHeader ih;
  
};

}; // namespace ns3

#endif /* QBB_HEADER */
