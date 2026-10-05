#pragma once

#include "engine/OscMessage.h"

#include <functional>
#include <mutex>
#include <string_view>
#include <utility>
#include <vector>

namespace supercollidaw {

template <typename Reply>
class ReplyInbox {
public:
    using Read = Reply (*)(sc_msg_iter& args);

    ReplyInbox(std::string_view address, Read read, std::function<void()> onReply): mAddress(address), mRead(read), mOnReply(std::move(onReply)) {}

    bool observe(std::string_view packet) {
        if (!hasAddress(packet, mAddress))
            return false;
        OscMessage message = parseOscMessage(packet);
        add(mRead(message.args));
        mOnReply();
        return true;
    }

    std::vector<Reply> take() {
        std::lock_guard lock(mMutex);
        return std::exchange(mReplies, {});
    }

private:
    static constexpr size_t kMaxReplies = 16;

    void add(Reply reply) {
        std::lock_guard lock(mMutex);
        if (mReplies.size() == kMaxReplies)
            mReplies.erase(mReplies.begin());
        mReplies.push_back(std::move(reply));
    }

    std::string_view mAddress;
    Read mRead;
    std::function<void()> mOnReply;
    std::mutex mMutex;
    std::vector<Reply> mReplies;
};

}
