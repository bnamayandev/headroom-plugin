#pragma once

#include "HeadroomLookAndFeel.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>
#include <optional>

namespace headroom::gui {
/** App-wide preferences (not part of presets or the host's session), shared by
    every open editor through a SharedResourcePointer and kept in
    Headroom/settings.xml beside the user presets. Broadcasts a change
    when any of them is set. */
class Settings final : public juce::ChangeBroadcaster {
public:
  Settings();

  /** Makes this the neon colours::accent() gives, saves it and tells the
      open editors to repaint. */
  void setNeon(Neon);

  /** Whether every capture and IR plays at the same volume, as measured when it
      loads, rather than at the level it was made at. On unless turned off. */
  bool isNormalisingVolume() const;
  void setNormalisingVolume(bool);

  static juce::File getFile();

private:
  juce::PropertiesFile file;
};

/** The settings popover: the API keys, whether to normalise the volume, and
    which neon what's live glows in. In the standalone app, also the audio
    device and saved states, which a host otherwise looks after. */
class SettingsPanel final : public juce::Component,
                            private juce::ChangeListener {
public:
  /** Each runs after the popover has closed, so it may delete the editor. */
  struct AppActions {
    std::function<void()> audioSettings, saveState, loadState, resetState;
  };

  /** `openKeys` runs, like the app actions, once the popover has closed. */
  explicit SettingsPanel(std::function<void()> openKeys,
                         std::optional<AppActions> = std::nullopt);
  ~SettingsPanel() override;

  static constexpr int panelWidth = 300, panelHeight = 222,
                       appSectionHeight = 138;
  /** The neons on offer; the first is the default. */
  static constexpr std::array<Neon, 3> neons{Neon::magenta, Neon::yellow, Neon::cyan};

  void paint(juce::Graphics &) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &) override;
  void mouseMove(const juce::MouseEvent &) override;

private:
  void changeListenerCallback(juce::ChangeBroadcaster *) override;
  juce::Rectangle<float> swatch(size_t index) const;
  void addAction(juce::TextButton &, const juce::String &tooltip,
                 std::function<void()> action);

  // Its own reference: resetting the app deletes the editor, and the editor's
  // reference with it, while this panel is still closing.
  juce::SharedResourcePointer<Settings> settings;
  const bool hasAppSection;
  juce::TextButton keysButton{"api keys & tone3000 sign-in..."},
      audioButton{"audio/midi settings..."},
      saveButton{"save..."}, loadButton{"load..."}, resetButton{"reset"};
  juce::ToggleButton normaliseButton{"normalise volume"};
  juce::Rectangle<int> audioCaption, stateCaption, agentCaption, levelsCaption, caption,
      swatchRow;
};
} // namespace headroom::gui
