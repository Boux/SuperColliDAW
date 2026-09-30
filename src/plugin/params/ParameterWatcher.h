#pragma once

#include "ParameterEvent.h"

#include <functional>
#include <mutex>
#include <string_view>
#include <vector>

struct sc_msg_iter;

namespace supercollidaw {

class ParameterWatcher {
public:
    explicit ParameterWatcher(std::function<void()> onEvents): mOnEvents(std::move(onEvents)) {}

    bool observe(std::string_view packet);
    void reset();
    std::vector<ParameterEvent> takeEvents();

private:
    void observePacket(std::string_view packet);
    void observeMessage(std::string_view message);
    void onDefinitionReceived(sc_msg_iter& args);
    void onDefinitionFile(sc_msg_iter& args);
    void onDefinitionFreed(sc_msg_iter& args);
    void observeDefinitions(std::string_view scgf);
    void push(ParameterEvent event);

    std::function<void()> mOnEvents;
    std::mutex mMutex;
    std::vector<ParameterEvent> mEvents;
};

}
