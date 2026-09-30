#include "ParameterSpec.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace supercollidaw {

namespace {

struct Warp {
    double (*map)(const ParameterSpec& spec, double x);
    double (*unmap)(const ParameterSpec& spec, double y);
};

double range(const ParameterSpec& spec) { return spec.maxValue - spec.minValue; }

double dbamp(double db) { return std::pow(10.0, db / 20.0); }

double ampdb(double amp) { return 20.0 * std::log10(amp); }

double linearMap(const ParameterSpec& spec, double x) { return x * range(spec) + spec.minValue; }

double linearUnmap(const ParameterSpec& spec, double y) { return (y - spec.minValue) / range(spec); }

double exponentialMap(const ParameterSpec& spec, double x) { return std::pow(spec.maxValue / spec.minValue, x) * spec.minValue; }

double exponentialUnmap(const ParameterSpec& spec, double y) { return std::log(y / spec.minValue) / std::log(spec.maxValue / spec.minValue); }

double curveOf(const ParameterSpec& spec) { return std::fabs(spec.curve) < 0.001 ? 0.001 : spec.curve; }

double curveMap(const ParameterSpec& spec, double x) {
    const double grow = std::exp(curveOf(spec));
    const double a = range(spec) / (1.0 - grow);
    return spec.minValue + a - a * std::pow(grow, x);
}

double curveUnmap(const ParameterSpec& spec, double y) {
    const double grow = std::exp(curveOf(spec));
    const double a = range(spec) / (1.0 - grow);
    return std::log((spec.minValue + a - y) / a) / curveOf(spec);
}

double cosineMap(const ParameterSpec& spec, double x) { return linearMap(spec, 0.5 - std::cos(M_PI * x) * 0.5); }

double cosineUnmap(const ParameterSpec& spec, double y) { return std::acos(1.0 - linearUnmap(spec, y) * 2.0) / M_PI; }

double sineMap(const ParameterSpec& spec, double x) { return linearMap(spec, std::sin(0.5 * M_PI * x)); }

double sineUnmap(const ParameterSpec& spec, double y) { return std::asin(linearUnmap(spec, y)) / (0.5 * M_PI); }

double fader(double x, double span) { return span >= 0.0 ? x * x : 1.0 - (1.0 - x) * (1.0 - x); }

double inverseFader(double y, double span) { return span >= 0.0 ? std::sqrt(y) : 1.0 - std::sqrt(1.0 - y); }

double ampMap(const ParameterSpec& spec, double x) { return fader(x, range(spec)) * range(spec) + spec.minValue; }

double ampUnmap(const ParameterSpec& spec, double y) { return inverseFader((y - spec.minValue) / range(spec), range(spec)); }

double dbMap(const ParameterSpec& spec, double x) {
    const double span = dbamp(spec.maxValue) - dbamp(spec.minValue);
    return ampdb(fader(x, span) * span + dbamp(spec.minValue));
}

double dbUnmap(const ParameterSpec& spec, double y) {
    const double span = dbamp(spec.maxValue) - dbamp(spec.minValue);
    return inverseFader((dbamp(y) - dbamp(spec.minValue)) / span, range(spec));
}

const std::unordered_map<std::string, Warp> kWarps = {
    { "lin", { linearMap, linearUnmap } },   { "exp", { exponentialMap, exponentialUnmap } },
    { "curve", { curveMap, curveUnmap } },   { "cos", { cosineMap, cosineUnmap } },
    { "sin", { sineMap, sineUnmap } },       { "amp", { ampMap, ampUnmap } },
    { "db", { dbMap, dbUnmap } },           { "linear", { linearMap, linearUnmap } },
    { "exponential", { exponentialMap, exponentialUnmap } },
};

const Warp& warpOf(const ParameterSpec& spec) {
    const auto warp = kWarps.find(spec.warp);
    return warp == kWarps.end() ? kWarps.at("lin") : warp->second;
}

double roundToStep(double value, double step) { return step > 0.0 ? std::floor(value / step + 0.5) * step : value; }

}

double mapToSpec(const ParameterSpec& spec, double normalized) {
    return roundToStep(warpOf(spec).map(spec, std::clamp(normalized, 0.0, 1.0)), spec.step);
}

double unmapFromSpec(const ParameterSpec& spec, double value) {
    const double low = std::min(spec.minValue, spec.maxValue);
    const double high = std::max(spec.minValue, spec.maxValue);
    return std::clamp(warpOf(spec).unmap(spec, std::clamp(roundToStep(value, spec.step), low, high)), 0.0, 1.0);
}

}
