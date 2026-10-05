#include "OscMessage.h"

namespace supercollidaw {

bool hasAddress(std::string_view packet, std::string_view address) {
    return packet.size() > address.size() && packet.starts_with(address) && packet[address.size()] == '\0';
}

OscMessage parseOscMessage(std::string_view packet) {
    const char* address = packet.data();
    const char* args = OSCstrskip(address);
    const int argsSize = static_cast<int>(packet.data() + packet.size() - args);
    return { std::string_view(address), sc_msg_iter(argsSize, args) };
}

// sc_msg_iter::gets returns null, not the default, once the arguments run out.
std::string stringArg(sc_msg_iter& args, const char* fallback) {
    const char* value = args.gets(fallback);
    return value ? value : fallback;
}

std::vector<std::string> remainingStrings(sc_msg_iter& args) {
    std::vector<std::string> strings;
    while (args.nextTag('\0') == 's')
        strings.push_back(stringArg(args, ""));
    return strings;
}

}
