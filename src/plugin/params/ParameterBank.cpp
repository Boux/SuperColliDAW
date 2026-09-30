#include "ParameterBank.h"

#include <algorithm>
#include <charconv>
#include <cstdio>

namespace supercollidaw {

namespace {

std::string defaultName(uint32_t index) { return "In.kr(" + std::to_string(index) + ")"; }

std::string formatNumber(double value) {
    char text[32];
    std::snprintf(text, sizeof(text), "%.4g", value);
    return text;
}

void markUsed(std::array<bool, ParameterBank::kCount>& used, const std::set<uint32_t>& buses) {
    for (auto bus = buses.begin(); bus != buses.end() && *bus < ParameterBank::kCount; ++bus)
        used[*bus] = true;
}

std::optional<double> parseNumber(std::string_view text) {
    const auto start = text.find_first_not_of(' ');
    if (start == std::string_view::npos)
        return std::nullopt;
    double value = 0.0;
    const auto [end, error] = std::from_chars(text.data() + start, text.data() + text.size(), value);
    return error == std::errc() ? std::optional<double>(value) : std::nullopt;
}

}

void ParameterBank::setValue(uint32_t index, double value) {
    if (index >= kCount)
        return;
    mSlots[index].value = std::clamp(value, 0.0, 1.0);
    mSlots[index].touched = true;
}

void ParameterBank::setModulation(uint32_t index, double amount) {
    if (index < kCount)
        mSlots[index].modulation = amount;
}

float ParameterBank::effectiveValue(uint32_t index) const {
    return static_cast<float>(std::clamp(mSlots[index].value.load() + mSlots[index].modulation.load(), 0.0, 1.0));
}

ParameterBank::Info ParameterBank::info(uint32_t index) const {
    const Slot& slot = mSlots[index];
    return { slot.declared ? slot.name : defaultName(index), slot.visible, slot.declared };
}

double ParameterBank::defaultValue(uint32_t index) const {
    const Slot& slot = mSlots[index];
    return slot.declared ? unmapFromSpec(slot.spec, slot.spec.defaultValue) : 0.0;
}

std::string ParameterBank::valueText(uint32_t index, double value) const {
    const Slot& slot = mSlots[index];
    if (!slot.declared)
        return formatNumber(value);
    const std::string number = formatNumber(mapToSpec(slot.spec, value));
    return slot.spec.units.empty() ? number : number + " " + slot.spec.units;
}

std::optional<double> ParameterBank::parseText(uint32_t index, std::string_view text) const {
    const std::optional<double> number = parseNumber(text);
    if (!number)
        return std::nullopt;
    const Slot& slot = mSlots[index];
    return slot.declared ? unmapFromSpec(slot.spec, *number) : std::clamp(*number, 0.0, 1.0);
}

ParameterBank::Changes ParameterBank::apply(const std::vector<ParameterEvent>& events) {
    Changes changes;
    for (const ParameterEvent& event : events)
        std::visit([&](const auto& e) { apply(e, changes); }, event);
    changes.info = updateVisibility() || changes.info;
    return changes;
}

void ParameterBank::apply(const SynthDefDefined& event, Changes&) { mBusesByDef[event.name] = event.controlBusesRead; }

void ParameterBank::apply(const SynthDefFreed& event, Changes&) { mBusesByDef.erase(event.name); }

void ParameterBank::apply(const ParameterDeclared& event, Changes& changes) {
    if (event.index >= kCount)
        return;
    Slot& slot = mSlots[event.index];
    slot.declared = true;
    slot.name = event.name.empty() ? defaultName(event.index) : event.name;
    slot.spec = event.spec;
    changes.info = true;
    if (slot.touched)
        return;
    slot.value = unmapFromSpec(slot.spec, slot.spec.defaultValue);
    changes.values = true;
}

void ParameterBank::apply(const ParametersReset&, Changes& changes) {
    mBusesByDef.clear();
    for (Slot& slot : mSlots)
        slot.declared = false;
    changes.info = true;
}

bool ParameterBank::updateVisibility() {
    std::array<bool, kCount> used{};
    for (const auto& [name, buses] : mBusesByDef)
        markUsed(used, buses);
    bool changed = false;
    for (uint32_t index = 0; index < kCount; ++index) {
        const bool visible = used[index] || mSlots[index].declared;
        changed = changed || visible != mSlots[index].visible;
        mSlots[index].visible = visible;
    }
    return changed;
}

std::vector<ParameterState> ParameterBank::state() const {
    std::vector<ParameterState> parameters(kCount);
    for (uint32_t index = 0; index < kCount; ++index) {
        const Slot& slot = mSlots[index];
        parameters[index] = { index, slot.value.load(), slot.declared, slot.visible, slot.name, slot.spec };
    }
    return parameters;
}

void ParameterBank::restore(const std::vector<ParameterState>& parameters) {
    for (const ParameterState& parameter : parameters) {
        if (parameter.index >= kCount)
            continue;
        Slot& slot = mSlots[parameter.index];
        slot.value = std::clamp(parameter.value, 0.0, 1.0);
        slot.touched = true;
        slot.declared = parameter.declared;
        slot.visible = parameter.visible;
        slot.name = parameter.name;
        slot.spec = parameter.spec;
    }
}

}
