#pragma once

#include "CodeDocument.h"
#include "FileDialog.h"
#include "ui/EditorActions.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace supercollidaw {

class CodeController {
public:
    struct Hooks {
        std::function<void(const std::string& code)> run;
        std::function<void(const std::string& line)> post;
        std::function<void()> markProjectDirty;
        std::function<void(const std::string& code)> showCode;
        std::function<void(const EditorStatus& status)> showStatus;
    };

    CodeController(std::string initialCode, Hooks hooks);

    const std::string& code() const { return mDocument.text(); }
    EditorStatus status() const;

    void edit(const std::string& code);
    void runAll(const std::string& code);
    void open();
    void save(const std::string& code);
    void saveAs(const std::string& code);
    void unlink(const std::string& code);
    void poll();

    PluginState state() const { return mDocument.state(); }
    void restore(const PluginState& state);

private:
    void startDialog(FileDialog::Kind kind);
    void pollDialog();
    void pollLinkedFile();
    void openChosen(const std::filesystem::path& path);
    void saveChosen(const std::filesystem::path& path);
    void documentChanged();

    CodeDocument mDocument;
    Hooks mHooks;
    std::unique_ptr<FileDialog> mDialog;
};

}
