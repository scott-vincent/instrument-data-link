#pragma once

namespace jetbridge {

static const int kPacketDataSize = 128;
static const char* kPublicDownlinkChannel = "theomessin.jetbridge.downlink";
static const char* kPublicUplinkChannel = "theomessin.jetbridge.uplink";

class Packet {
 public:
  int id;
  char data[kPacketDataSize] = {};
  Packet(const char data[]);
};

enum ClientDataDefinitions {
  kPacketDefinition = 5321,
};

enum ClientDataAreas {
  kPublicDownlinkArea = 5321,
  kPublicUplinkArea = 5322,
};

enum DataRequest {
  kUplinkRequest = 5321,
  kDownlinkRequest = 5322,
};

}  // namespace jetbridge
