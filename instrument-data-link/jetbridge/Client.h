#pragma once

#include <Windows.h>

#include <future>
#include <map>

#include "Protocol.h"

namespace jetbridge {

class Client {
 private:
  void* simconnect = 0;

 public:
  Client(void* simconnect);
  void request(const char data[]);
};

}  // namespace jetbridge
