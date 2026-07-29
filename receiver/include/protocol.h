#pragma once
#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>
#include "config.h"

namespace pfx::protocol {

constexpr uint32_t PACKET_MAGIC = 0x50465852UL;  // "PFXR"

enum class PacketType : uint8_t {
  Control = 0x31,
  Acknowledgement = 0xA1,
};

#pragma pack(push, 1)
struct Packet {
  uint32_t magic;
  uint32_t systemId;
  uint16_t sequence;
  uint8_t type;
  uint8_t relayMask;
  uint32_t authTag;
  uint16_t crc;
};
#pragma pack(pop)

static_assert(sizeof(Packet) == 18, "Unexpected RF packet packing");

inline uint16_t crc16Ccitt(const uint8_t* data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000U) ? static_cast<uint16_t>((crc << 1) ^ 0x1021U)
                            : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

// Rejects accidental cross-control and malformed packets. This is not
// cryptographic authentication and is not a certified firing protocol.
inline uint32_t keyedTag(const Packet& packet) {
  uint32_t hash = 2166136261UL ^ config::SHARED_KEY;
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&packet);
  constexpr size_t authenticatedLength = offsetof(Packet, authTag);
  for (size_t i = 0; i < authenticatedLength; ++i) {
    hash ^= bytes[i];
    hash *= 16777619UL;
  }
  hash ^= config::SHARED_KEY;
  hash *= 16777619UL;
  return hash;
}

inline Packet makePacket(PacketType type, uint16_t sequence, uint8_t relayMask) {
  Packet packet{};
  packet.magic = PACKET_MAGIC;
  packet.systemId = config::SYSTEM_ID;
  packet.sequence = sequence;
  packet.type = static_cast<uint8_t>(type);
  packet.relayMask = relayMask & 0x3FU;
  packet.authTag = keyedTag(packet);
  packet.crc = crc16Ccitt(reinterpret_cast<const uint8_t*>(&packet), offsetof(Packet, crc));
  return packet;
}

inline bool validate(const Packet& packet, PacketType expectedType) {
  if (packet.magic != PACKET_MAGIC || packet.systemId != config::SYSTEM_ID) return false;
  if (packet.type != static_cast<uint8_t>(expectedType)) return false;
  if ((packet.relayMask & 0xC0U) != 0) return false;
  if (packet.authTag != keyedTag(packet)) return false;
  const uint16_t expectedCrc =
      crc16Ccitt(reinterpret_cast<const uint8_t*>(&packet), offsetof(Packet, crc));
  return packet.crc == expectedCrc;
}

}  // namespace pfx::protocol
