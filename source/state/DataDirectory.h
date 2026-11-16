#pragma once

#include <juce_core/juce_core.h>

namespace headroom {
/** The amp's folder in the user's app data: presets, tunings, settings, the agent's
   settings, the TONE3000 library and (Linux, Windows) the log.

    The amp used to be called Evil Amplifier. The first call in a process moves
   what's in the folder of that name over, so an upgrade keeps all of it. */
juce::File getDataDirectory();

/** Moves the contents of legacy, the old folder, into dir without replacing anything
   already there, and renames the old log and preset files. */
void moveLegacyDataDirectory(const juce::File &legacy, const juce::File &dir);
} // namespace headroom
