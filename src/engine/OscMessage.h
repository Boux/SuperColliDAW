#pragma once

#include "sc_msg_iter.h"

#include <string>
#include <string_view>
#include <vector>

namespace supercollidaw {

struct OscMessage {
    std::string_view address;
    sc_msg_iter args;
};

bool hasAddress(std::string_view packet, std::string_view address);
OscMessage parseOscMessage(std::string_view packet);
std::string stringArg(sc_msg_iter& args, const char* fallback);
std::vector<std::string> remainingStrings(sc_msg_iter& args);

}
