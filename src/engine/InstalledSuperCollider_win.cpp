#include "InstalledSuperCollider.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <regex>
#include <windows.h>

namespace supercollidaw {

namespace fs = std::filesystem;

namespace {

struct WindowsInstall {
    std::vector<int> version;
    fs::path dir;
};

std::vector<int> versionNumbers(const std::string& text) {
    static const std::regex number("[0-9]+");
    std::vector<int> numbers;
    std::transform(std::sregex_iterator(text.begin(), text.end(), number), std::sregex_iterator(), std::back_inserter(numbers),
        [](const std::smatch& match) { return std::stoi(match.str()); });
    return numbers;
}

// The installer records each version's folder as the default value of HKCU\Software\SuperCollider\<version>.
std::vector<WindowsInstall> registryInstalls() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\SuperCollider", 0, KEY_READ, &key) != ERROR_SUCCESS)
        return {};
    std::vector<WindowsInstall> installs;
    std::array<wchar_t, 256> version;
    std::array<wchar_t, MAX_PATH> dir;
    for (DWORD index = 0, length = version.size(); RegEnumKeyExW(key, index, version.data(), &length, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS;
         ++index, length = version.size()) {
        DWORD bytes = sizeof(dir);
        if (RegGetValueW(key, version.data(), nullptr, RRF_RT_REG_SZ, nullptr, dir.data(), &bytes) == ERROR_SUCCESS)
            installs.push_back({ versionNumbers(fs::path(version.data()).string()), fs::path(dir.data()) });
    }
    RegCloseKey(key);
    return installs;
}

}

const char kSclangName[] = "sclang.exe";

std::vector<fs::path> platformInstallDirs() {
    std::vector<WindowsInstall> installs = registryInstalls();
    std::ranges::stable_sort(installs, std::ranges::greater(), &WindowsInstall::version);
    std::vector<fs::path> dirs;
    std::ranges::transform(installs, std::back_inserter(dirs), &WindowsInstall::dir);
    return dirs;
}

std::vector<fs::path> systemPluginDirs() { return {}; }

}
