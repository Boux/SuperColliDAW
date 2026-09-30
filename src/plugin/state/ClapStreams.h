#pragma once

#include <clap/clap.h>

#include <optional>
#include <string>
#include <string_view>

namespace supercollidaw {

bool writeAll(const clap_ostream* stream, std::string_view bytes);
std::optional<std::string> readAll(const clap_istream* stream);

}
