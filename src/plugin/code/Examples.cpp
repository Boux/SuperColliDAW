#include "Examples.h"

#include "LinkedFile.h"

#include <algorithm>
#include <optional>

namespace supercollidaw {

namespace fs = std::filesystem;

namespace {

std::vector<fs::path> exampleFiles(const fs::path& dir) {
    std::error_code ec;
    std::vector<fs::path> files;
    for (const fs::directory_entry& entry : fs::directory_iterator(dir, ec))
        files.push_back(entry.path());
    std::erase_if(files, [](const fs::path& file) { return file.extension() != ".scd"; });
    std::ranges::sort(files);
    return files;
}

std::string titleOf(const std::string& code, const fs::path& file) {
    const std::string firstLine = code.substr(0, code.find('\n'));
    const size_t start = firstLine.starts_with("//") ? firstLine.find_first_not_of("/ ") : std::string::npos;
    return start == std::string::npos ? file.stem().string() : firstLine.substr(start);
}

}

std::vector<Example> loadExamples(const fs::path& dir) {
    std::vector<Example> examples;
    for (const fs::path& file : exampleFiles(dir)) {
        const std::optional<std::string> code = LinkedFile(file).read();
        if (code)
            examples.push_back({ titleOf(*code, file), *code });
    }
    return examples;
}

}
