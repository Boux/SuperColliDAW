#include "TestSynth.h"

#include "engine/Engine.h"

#include "scsynthsend.h"

#include <vector>

extern const unsigned char supercollidaw_test_scsyndef[];
extern const unsigned long supercollidaw_test_scsyndef_size;

namespace supercollidaw {

namespace {

constexpr int kTestSynthNodeId = 1000;
constexpr int kAddToHead = 0;
constexpr int kRootNodeId = 0;

}

void startTestSynth(Engine& engine) {
    small_scpacket synthNew;
    synthNew.adds("/s_new");
    synthNew.maketags(5);
    synthNew.addtag(',');
    synthNew.addtag('s');
    synthNew.adds("supercollidaw_test");
    synthNew.addtag('i');
    synthNew.addi(kTestSynthNodeId);
    synthNew.addtag('i');
    synthNew.addi(kAddToHead);
    synthNew.addtag('i');
    synthNew.addi(kRootNodeId);

    std::vector<uint8> synthDef(supercollidaw_test_scsyndef, supercollidaw_test_scsyndef + supercollidaw_test_scsyndef_size);
    small_scpacket defRecv;
    defRecv.adds("/d_recv");
    defRecv.maketags(3);
    defRecv.addtag(',');
    defRecv.addtag('b');
    defRecv.addb(synthDef.data(), synthDef.size());
    defRecv.addtag('b');
    defRecv.addb(reinterpret_cast<uint8*>(synthNew.data()), synthNew.size());

    engine.sendPacket(defRecv.data(), static_cast<int>(defRecv.size()));
}

}
