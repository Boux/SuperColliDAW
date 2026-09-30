#include "ParameterWatcher.h"

#include "engine/SynthDefControls.h"

#include "sc_msg_iter.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <unordered_map>
#include <utility>

namespace supercollidaw {

namespace {

constexpr std::string_view kBundleTag("#bundle\0", 8);
constexpr size_t kBundleHeaderSize = 16;
constexpr char kDeclareAddress[] = "/supercollidaw/param";

struct Message {
    std::string_view address;
    sc_msg_iter args;
};

Message parseMessage(std::string_view message) {
    const char* address = message.data();
    const char* args = OSCstrskip(address);
    const int argsSize = static_cast<int>(message.data() + message.size() - args);
    return { std::string_view(address), sc_msg_iter(argsSize, args) };
}

// sc_msg_iter::gets returns null, not the default, once the arguments run out.
std::string stringArg(sc_msg_iter& args, const char* fallback) {
    const char* value = args.gets(fallback);
    return value ? value : fallback;
}

std::string readBlob(sc_msg_iter& args) {
    std::string blob(args.getbsize(), '\0');
    args.getb(blob.data(), blob.size());
    return blob;
}

std::string readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

ParameterDeclared readDeclaration(sc_msg_iter& args) {
    ParameterDeclared declaration{ static_cast<uint32_t>(args.geti()), stringArg(args, ""), {} };
    ParameterSpec& spec = declaration.spec;
    spec.minValue = args.getf();
    spec.maxValue = args.getf(1.f);
    spec.warp = stringArg(args, "lin");
    spec.curve = args.getf();
    spec.step = args.getf();
    spec.defaultValue = args.getf();
    spec.units = stringArg(args, "");
    return declaration;
}

}

bool ParameterWatcher::observe(std::string_view packet) {
    if (packet.size() >= sizeof(kDeclareAddress) && !std::memcmp(packet.data(), kDeclareAddress, sizeof(kDeclareAddress))) {
        Message message = parseMessage(packet);
        push(readDeclaration(message.args));
        return true;
    }
    observePacket(packet);
    return false;
}

void ParameterWatcher::reset() {
    std::lock_guard lock(mMutex);
    mEvents.clear();
    mEvents.emplace_back(ParametersReset{});
}

std::vector<ParameterEvent> ParameterWatcher::takeEvents() {
    std::lock_guard lock(mMutex);
    return std::exchange(mEvents, {});
}

void ParameterWatcher::observePacket(std::string_view packet) {
    if (!packet.starts_with(kBundleTag)) {
        observeMessage(packet);
        return;
    }
    for (size_t pos = kBundleHeaderSize; pos + 4 <= packet.size();) {
        const size_t size = static_cast<size_t>(OSCint(packet.data() + pos));
        if (pos + 4 + size > packet.size())
            return;
        observePacket(packet.substr(pos + 4, size));
        pos += 4 + size;
    }
}

void ParameterWatcher::observeMessage(std::string_view packet) {
    using Handler = void (ParameterWatcher::*)(sc_msg_iter&);
    static const std::unordered_map<std::string_view, Handler> kHandlers = {
        { "/d_recv", &ParameterWatcher::onDefinitionReceived },
        { "/d_load", &ParameterWatcher::onDefinitionFile },
        { "/d_free", &ParameterWatcher::onDefinitionFreed },
    };
    if (packet.empty() || packet.front() != '/')
        return;
    Message message = parseMessage(packet);
    const auto handler = kHandlers.find(message.address);
    if (handler != kHandlers.end())
        (this->*handler->second)(message.args);
}

void ParameterWatcher::onDefinitionReceived(sc_msg_iter& args) {
    observeDefinitions(readBlob(args));
    if (args.nextTag('\0') == 'b')
        observePacket(readBlob(args));
}

void ParameterWatcher::onDefinitionFile(sc_msg_iter& args) { observeDefinitions(readFile(stringArg(args, ""))); }

void ParameterWatcher::onDefinitionFreed(sc_msg_iter& args) {
    while (args.nextTag('\0') == 's')
        push(SynthDefFreed{ stringArg(args, "") });
}

void ParameterWatcher::observeDefinitions(std::string_view scgf) {
    const auto defs = scanSynthDefs(scgf);
    if (!defs)
        return;
    for (const SynthDefControls& def : *defs)
        push(SynthDefDefined{ def.name, def.controlBusesRead });
}

void ParameterWatcher::push(ParameterEvent event) {
    {
        std::lock_guard lock(mMutex);
        mEvents.push_back(std::move(event));
    }
    mOnEvents();
}

}
