#pragma once

#include "ParameterEvent.h"
#include "ParameterSpec.h"
#include "plugin/state/PluginState.h"

#include <array>
#include <atomic>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace supercollidaw {

class ParameterBank {
public:
    static constexpr uint32_t kCount = 32;

    struct Info {
        std::string name;
        bool visible;
        bool declared;
    };

    struct Changes {
        bool info = false;
        bool values = false;
    };

    void setValue(uint32_t index, double value);
    void setModulation(uint32_t index, double amount);
    float effectiveValue(uint32_t index) const;

    double value(uint32_t index) const { return mSlots[index].value.load(); }
    Info info(uint32_t index) const;
    double defaultValue(uint32_t index) const;
    std::string valueText(uint32_t index, double value) const;
    std::optional<double> parseText(uint32_t index, std::string_view text) const;

    Changes apply(const std::vector<ParameterEvent>& events);

    std::vector<ParameterState> state() const;
    void restore(const std::vector<ParameterState>& parameters);

private:
    struct Slot {
        std::atomic<double> value = 0.0;
        std::atomic<double> modulation = 0.0;
        std::atomic<bool> touched = false;
        bool declared = false;
        bool visible = false;
        std::string name;
        ParameterSpec spec;
    };

    void apply(const SynthDefDefined& event, Changes& changes);
    void apply(const SynthDefFreed& event, Changes& changes);
    void apply(const ParameterDeclared& event, Changes& changes);
    void apply(const ParametersReset& event, Changes& changes);
    bool updateVisibility();

    std::array<Slot, kCount> mSlots;
    std::map<std::string, std::set<uint32_t>> mBusesByDef;
};

}
