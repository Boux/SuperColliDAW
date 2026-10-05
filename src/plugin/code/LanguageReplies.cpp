#include "LanguageReplies.h"

#include "engine/OscMessage.h"

namespace supercollidaw {

namespace {

Parameter readParameter(sc_msg_iter& args) { return { stringArg(args, ""), stringArg(args, "") }; }

Signature readSignature(sc_msg_iter& args) {
    Signature signature{ stringArg(args, ""), {} };
    const int count = args.geti();
    for (int i = 0; i < count; ++i)
        signature.parameters.push_back(readParameter(args));
    return signature;
}

}

Completion readCompletion(sc_msg_iter& args) { return { stringArg(args, ""), remainingStrings(args) }; }

SignatureHelp readSignatureHelp(sc_msg_iter& args) {
    SignatureHelp help{ stringArg(args, ""), static_cast<size_t>(args.geti()), {} };
    while (args.nextTag('\0') == 's')
        help.signatures.push_back(readSignature(args));
    return help;
}

}
