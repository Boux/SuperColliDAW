#pragma once

#include "EditorSettings.h"

namespace supercollidaw {

// Edits settings in place; returns true when a change is complete and should be saved. textSize is the code text's size in pixels.
bool drawSettingsForm(EditorSettings& settings, float textSize);

}
