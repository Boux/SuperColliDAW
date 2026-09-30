#include "ParameterExtension.h"

#include "ParameterEventReader.h"
#include "plugin/Plugin.h"

#include <cstring>
#include <string>

namespace supercollidaw {

namespace {

void copyText(const std::string& text, char* out, uint32_t size) {
    if (size == 0)
        return;
    std::strncpy(out, text.c_str(), size - 1);
    out[size - 1] = '\0';
}

bool getInfo(const clap_plugin* plugin, uint32_t index, clap_param_info* info) {
    if (index >= ParameterBank::kCount)
        return false;
    const ParameterBank& bank = Plugin::parameters(plugin);
    const ParameterBank::Info parameter = bank.info(index);
    *info = {};
    info->id = index;
    info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_MODULATABLE | (parameter.visible ? 0 : CLAP_PARAM_IS_HIDDEN);
    copyText(parameter.name, info->name, CLAP_NAME_SIZE);
    info->min_value = 0.0;
    info->max_value = 1.0;
    info->default_value = bank.defaultValue(index);
    return true;
}

bool getValue(const clap_plugin* plugin, clap_id id, double* value) {
    if (id >= ParameterBank::kCount)
        return false;
    *value = Plugin::parameters(plugin).value(id);
    return true;
}

bool valueToText(const clap_plugin* plugin, clap_id id, double value, char* display, uint32_t size) {
    if (id >= ParameterBank::kCount)
        return false;
    copyText(Plugin::parameters(plugin).valueText(id, value), display, size);
    return true;
}

bool textToValue(const clap_plugin* plugin, clap_id id, const char* display, double* value) {
    if (id >= ParameterBank::kCount)
        return false;
    const std::optional<double> parsed = Plugin::parameters(plugin).parseText(id, display);
    if (parsed)
        *value = *parsed;
    return parsed.has_value();
}

}

const clap_plugin_params kParameterExtension = {
    .count = [](const clap_plugin*) { return ParameterBank::kCount; },
    .get_info = getInfo,
    .get_value = getValue,
    .value_to_text = valueToText,
    .text_to_value = textToValue,
    .flush = [](const clap_plugin* plugin, const clap_input_events* in, const clap_output_events*) {
        ParameterEventReader(Plugin::parameters(plugin), in).applyAll();
    },
};

}
