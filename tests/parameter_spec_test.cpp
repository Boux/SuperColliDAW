#include "plugin/params/ParameterSpec.h"

#include <cmath>
#include <cstdio>

namespace {

using supercollidaw::mapToSpec;
using supercollidaw::ParameterSpec;
using supercollidaw::unmapFromSpec;

int gFailures = 0;

// Expected values come from sclang 3.14.1: spec.map(0.25), spec.map(0.8), spec.unmap(spec.map(0.3)).
void check(const char* name, const ParameterSpec& spec, double at25, double at80) {
    const bool mapped = std::fabs(mapToSpec(spec, 0.25) - at25) < 1e-6 && std::fabs(mapToSpec(spec, 0.8) - at80) < 1e-6;
    const bool roundTrip = std::fabs(unmapFromSpec(spec, mapToSpec(spec, 0.3)) - 0.3) < 1e-9;
    std::printf("%s: %s maps like SuperCollider%s\n", mapped && roundTrip ? "PASS" : "FAIL", name, roundTrip ? "" : " (round trip failed)");
    gFailures += mapped && roundTrip ? 0 : 1;
}

}

int main() {
    check("exp", { .minValue = 20, .maxValue = 20000, .warp = "exp" }, 112.468265038, 5023.77286302);
    check("curve", { .minValue = 0, .maxValue = 10, .warp = "curve", .curve = 4 }, 0.320586032801, 4.39054896159);
    check("amp", { .minValue = 0, .maxValue = 1, .warp = "amp" }, 0.0625, 0.64);
    check("db", { .minValue = -60, .maxValue = 0, .warp = "db" }, -23.9530788081, -3.87151608102);
    check("cos", { .minValue = 0, .maxValue = 1, .warp = "cos" }, 0.146446609407, 0.904508497187);
    check("step", { .minValue = 0, .maxValue = 10, .warp = "lin", .step = 1 }, 3, 8);
    std::printf("%d failure(s)\n", gFailures);
    return gFailures == 0 ? 0 : 1;
}
