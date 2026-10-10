#include "SclangProcess.h"

#include <crt_externs.h>

namespace supercollidaw {

// A bundle cannot link environ, so macOS hands it out through _NSGetEnviron().
std::vector<std::string> SclangProcess::environmentVariables() {
    std::vector<std::string> variables;
    for (char** entry = *_NSGetEnviron(); *entry; ++entry)
        variables.emplace_back(*entry);
    return variables;
}

}
