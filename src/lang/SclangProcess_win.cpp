#include "SclangProcess.h"

#include <cstring>
#include <windows.h>

namespace supercollidaw {

// The ANSI block, because tiny-process-library starts the process with the ANSI CreateProcess.
std::vector<std::string> SclangProcess::environmentVariables() {
    char* block = GetEnvironmentStrings();
    std::vector<std::string> variables;
    for (const char* entry = block; *entry; entry += std::strlen(entry) + 1)
        variables.emplace_back(entry);
    FreeEnvironmentStringsA(block);
    return variables;
}

}
