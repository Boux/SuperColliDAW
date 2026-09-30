#pragma once

#include <pugl/pugl.h>

#include <string>

namespace supercollidaw {

class PuglClipboard {
public:
    explicit PuglClipboard(PuglView* view): mView(view) {}

    void install();
    bool requestPasteFor(const PuglEvent& event);
    PuglStatus acceptOffer(const PuglDataOfferEvent& offer);
    PuglStatus receive(const PuglDataEvent& data);

private:
    PuglView* mView;
    std::string mText;
};

}
