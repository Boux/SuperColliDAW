#include "Plugin.h"
#include "PluginPaths.h"

#include <cstring>
#include <mutex>

namespace {

using supercollidaw::Engine;
using supercollidaw::Plugin;

std::mutex gEntryMutex;
int gEntryInitCount = 0;

bool entryInit(const char* pluginPath) {
    std::lock_guard lock(gEntryMutex);
    if (gEntryInitCount++ == 0)
        supercollidaw::setPluginBinaryPath(pluginPath);
    return true;
}

void entryDeinit() {
    std::lock_guard lock(gEntryMutex);
    if (gEntryInitCount == 0)
        return;
    if (--gEntryInitCount == 0)
        Engine::unloadPlugins();
}

const clap_plugin_factory kFactory = {
    .get_plugin_count = [](const clap_plugin_factory*) -> uint32_t { return 1; },
    .get_plugin_descriptor = [](const clap_plugin_factory*, uint32_t index) -> const clap_plugin_descriptor* {
        return index == 0 ? &Plugin::kDescriptor : nullptr;
    },
    .create_plugin = [](const clap_plugin_factory*, const clap_host* host, const char* pluginId) -> const clap_plugin* {
        if (!clap_version_is_compatible(host->clap_version) || std::strcmp(pluginId, Plugin::kDescriptor.id))
            return nullptr;
        return (new Plugin(host))->clapPlugin();
    },
};

}

extern "C" CLAP_EXPORT const clap_plugin_entry clap_entry = {
    .clap_version = CLAP_VERSION_INIT,
    .init = entryInit,
    .deinit = entryDeinit,
    .get_factory = [](const char* factoryId) -> const void* {
        return std::strcmp(factoryId, CLAP_PLUGIN_FACTORY_ID) ? nullptr : &kFactory;
    },
};
