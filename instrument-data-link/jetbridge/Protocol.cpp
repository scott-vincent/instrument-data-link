#include "Protocol.h"

#include <cstdlib>
#include <cstring>
#include <ctime>

jetbridge::Packet::Packet(const char data[]) {
	this->id = rand();

  // Copy the passed data to the packet data
  std::memcpy(this->data, data, sizeof(this->data));
}
