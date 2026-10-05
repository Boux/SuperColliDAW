#pragma once

#include <string>
#include <vector>

namespace supercollidaw {

struct Parameter {
    std::string name;
    std::string defaultValue;
};

struct Signature {
    std::string label;
    std::vector<Parameter> parameters;
};

struct SignatureHelp {
    std::string callee;
    size_t total = 0;
    std::vector<Signature> signatures;
};

}
