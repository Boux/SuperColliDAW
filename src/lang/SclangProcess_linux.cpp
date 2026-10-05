#include "SclangProcess.h"

extern char** environ;

namespace supercollidaw {

std::vector<std::string> SclangProcess::environmentVariables() {
    std::vector<std::string> variables;
    for (char** entry = environ; *entry; ++entry)
        variables.emplace_back(*entry);
    return variables;
}

}
