#pragma once

#include "ui/Completion.h"
#include "ui/SignatureHelp.h"

struct sc_msg_iter;

namespace supercollidaw {

inline constexpr char kCompletionsAddress[] = "/supercollidaw/completions";
inline constexpr char kSignaturesAddress[] = "/supercollidaw/signatures";

Completion readCompletion(sc_msg_iter& args);
SignatureHelp readSignatureHelp(sc_msg_iter& args);

}
