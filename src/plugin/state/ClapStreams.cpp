#include "ClapStreams.h"

#include <array>

namespace supercollidaw {

bool writeAll(const clap_ostream* stream, std::string_view bytes) {
    while (!bytes.empty()) {
        const int64_t written = stream->write(stream, bytes.data(), bytes.size());
        if (written <= 0)
            return false;
        bytes.remove_prefix(static_cast<size_t>(written));
    }
    return true;
}

std::optional<std::string> readAll(const clap_istream* stream) {
    std::string bytes;
    std::array<char, 4096> buffer;
    for (int64_t read = stream->read(stream, buffer.data(), buffer.size()); read != 0; read = stream->read(stream, buffer.data(), buffer.size())) {
        if (read < 0)
            return std::nullopt;
        bytes.append(buffer.data(), static_cast<size_t>(read));
    }
    return bytes;
}

}
