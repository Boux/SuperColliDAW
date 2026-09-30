#pragma once

#include <string>

namespace supercollidaw {

// A SuperCollider ControlSpec: the parameter stays 0..1 in the host, the spec only maps it for display and for SC.
struct ParameterSpec {
    double minValue = 0.0;
    double maxValue = 1.0;
    std::string warp = "lin";
    double curve = 0.0;
    double step = 0.0;
    double defaultValue = 0.0;
    std::string units;
};

double mapToSpec(const ParameterSpec& spec, double normalized);
double unmapFromSpec(const ParameterSpec& spec, double value);

}
