#include "DataDirectory.h"

#include "PresetManager.h"

#include <mutex>

namespace headroom {
namespace {
// What the folder, its log and its presets were called before the amp became Headroom.
constexpr const char *legacyFolderName = "EvilAmplifier";
constexpr const char *legacyLogName = "EvilAmplifier.log";
constexpr const char *legacyPresetExtension = ".evilpreset";

/** Moves what's in from into to, folder by folder, never over anything already there.
   A DAW and the standalone can start together, so another process may be moving the
   same files: whichever loses just finds them gone. */
void moveContents(const juce::File &from, const juce::File &to) {
  to.createDirectory();
  for (const auto &child : from.findChildFiles(juce::File::findFilesAndDirectories, false,
                                               "*", juce::File::FollowSymlinks::no)) {
    const auto target = to.getChildFile(child.getFileName());
    if (!target.exists())
      child.moveFileTo(target);
    else if (child.isDirectory() && !child.isSymbolicLink() && target.isDirectory())
      moveContents(child, target);
  }
  from.deleteFile(); // only goes if it's empty now
}
} // namespace

void moveLegacyDataDirectory(const juce::File &legacy, const juce::File &dir) {
  if (!legacy.isDirectory())
    return;
  if (dir.exists() || !legacy.moveFileTo(dir))
    moveContents(legacy, dir);

  if (const auto log = dir.getChildFile(legacyLogName); log.existsAsFile())
    if (const auto renamed = dir.getChildFile("Headroom.log"); !renamed.exists())
      log.moveFileTo(renamed);

  const auto presets = dir.getChildFile("Presets");
  for (const auto &preset : presets.findChildFiles(
           juce::File::findFiles, false, juce::String("*") + legacyPresetExtension))
    if (const auto renamed = preset.withFileExtension(PresetManager::fileExtension);
        !renamed.exists())
      preset.moveFileTo(renamed);
}

juce::File getDataDirectory() {
  const auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("Headroom");
  static std::once_flag moved;
  std::call_once(moved, [&dir] {
    moveLegacyDataDirectory(dir.getSiblingFile(legacyFolderName), dir);
  });
  return dir;
}
} // namespace headroom
