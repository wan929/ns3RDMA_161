//wan

#ifndef MIME_UDP_HEADER_H
#define MIME_UDP_HEADER_H

#include <stdint.h>
#include "ns3/header.h"
#include "ns3/buffer.h"
#include <cstdio>

namespace ns3 {
 
class MineUdpHeader
{
public:
  enum Mode{
		NOTIFICATION = 1,
		NONE
	};
  static Mode mode;

  MineUdpHeader ();

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
  uint16_t m_round;
};

}; // namespace ns3

#endif /* MIME_UDP_HEADER */
