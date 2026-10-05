#include "InstalledSuperCollider.h"

#include "SC_Filesystem.hpp"
#include "SC_StringParser.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <numeric>
#include <vector>

#if defined(_WIN32)
#    include <array>
#    include <iterator>
#    include <regex>
#    include <windows.h>
#endif

namespace supercollidaw {

namespace fs = std::filesystem;

namespace {

#if defined(_WIN32)
constexpr char kSclangName[] = "sclang.exe";
#else
constexpr char kSclangName[] = "sclang";
#endif

#if defined(_WIN32)

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

std::vector<fs::path> platformInstallDirs() {
    std::vector<WindowsInstall> installs = registryInstalls();
    std::ranges::stable_sort(installs, std::ranges::greater(), &WindowsInstall::version);
    std::vector<fs::path> dirs;
    std::ranges::transform(installs, std::back_inserter(dirs), &WindowsInstall::dir);
    return dirs;
}

#elif defined(__APPLE__)

std::vector<fs::path> platformInstallDirs() { return { "/Applications/SuperCollider.app/Contents/MacOS" }; }

#else

std::vector<fs::path> platformInstallDirs() { return {}; }

#endif

std::vector<fs::path> systemPluginDirs() {
#if defined(__linux__)
    return { "/usr/lib/SuperCollider/plugins", "/usr/lib64/SuperCollider/plugins", "/usr/local/lib/SuperCollider/plugins" };
#else
    return {};
#endif
}

std::vector<fs::path> extensionDirs() {
    using DirName = SC_Filesystem::DirName;
    auto& filesystem = SC_Filesystem::instance();
    return { filesystem.getDirectory(DirName::SystemExtension), filesystem.getDirectory(DirName::UserExtension) };
}

std::vector<fs::path> executableSearchDirs() {
    std::vector<fs::path> dirs;
    const char* path = std::getenv("PATH");
    for (SC_StringParser parser(path ? path : "", SC_STRPARSE_PATHDELIMITER); !parser.AtEnd();)
        dirs.emplace_back(parser.NextToken());
    const std::vector<fs::path> installs = platformInstallDirs();
    dirs.insert(dirs.end(), installs.begin(), installs.end());
    return dirs;
}

}

std::optional<std::string> installedSclangPath() {
    const std::vector<fs::path> dirs = executableSearchDirs();
    const auto found = std::find_if(dirs.begin(), dirs.end(), [](const fs::path& dir) {
        std::error_code ec;
        return fs::is_regular_file(dir / kSclangName, ec);
    });
    if (found == dirs.end())
        return std::nullopt;
    return (*found / kSclangName).string();
}

std::string ugenPluginPath(const fs::path& bundledDir) {
    std::vector<fs::path> candidates = { bundledDir };
    const std::vector<fs::path> system = systemPluginDirs();
    const std::vector<fs::path> extensions = extensionDirs();
    candidates.insert(candidates.end(), system.begin(), system.end());
    candidates.insert(candidates.end(), extensions.begin(), extensions.end());

    std::vector<fs::path> existing;
    std::for_each(candidates.begin(), candidates.end(), [&](const fs::path& dir) {
        std::error_code ec;
        const fs::path canonical = fs::canonical(dir, ec);
        if (ec || !fs::is_directory(canonical, ec))
            return;
        if (std::find(existing.begin(), existing.end(), canonical) == existing.end())
            existing.push_back(canonical);
    });

    const std::string delimiter(1, SC_STRPARSE_PATHDELIMITER);
    return std::accumulate(existing.begin(), existing.end(), std::string(), [&](std::string path, const fs::path& dir) {
        return path.empty() ? dir.string() : path + delimiter + dir.string();
    });
}

}
