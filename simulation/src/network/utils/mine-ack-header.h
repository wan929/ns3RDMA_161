//wan

#ifndef MIME_ACK_HEADER_H
#define MIME_ACK_HEADER_H

#include <stdint.h>
#include "ns3/header.h"
#include "ns3/buffer.h"
#include <cstdio>

namespace ns3 {
 
class MineAckHeader
{
public:
  enum Mode{
		NOTIFICATION = 1,
		NONE
	};
  static Mode mode;

  MineAckHeader ();

//in-network notification
  void SetSequence (uint16_t sequence);
  uint16_t GetSequence (void) const;
  void SetRound (uint16_t round);
  uint16_t GetRound (void) const;

  static uint32_t GetStaticSize();
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);

//in-network notification 
  uint16_t m_sequence;
};

}; // namespace ns3

#endif /* MIME_ACK_HEADER */
