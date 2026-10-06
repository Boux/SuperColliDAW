#pragma once

#include <clap/clap.h>

#include <algorithm>
#include <cstring>
#include <string>

inline const clap_ostream* appendingStream(std::string& bytes) {
    static clap_ostream stream;
    stream = {
        .ctx = &bytes,
        .write = [](const clap_ostream* s, const void* buffer, uint64_t size) -> int64_t {
            static_cast<std::string*>(s->ctx)->append(static_cast<const char*>(buffer), size);
            return static_cast<int64_t>(size);
        },
    };
    return &stream;
}

struct ReadCursor {
    const std::string& bytes;
    size_t position = 0;
};

inline const clap_istream* readingStream(ReadCursor& cursor) {
    static clap_istream stream;
    stream = {
        .ctx = &cursor,
        .read = [](const clap_istream* s, void* buffer, uint64_t size) -> int64_t {
            auto* c = static_cast<ReadCursor*>(s->ctx);
            const size_t count = std::min<size_t>(size, c->bytes.size() - c->position);
            std::memcpy(buffer, c->bytes.data() + c->position, count);
            c->position += count;
            return static_cast<int64_t>(count);
        },
    };
    return &stream;
}
