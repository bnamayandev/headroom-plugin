#pragma once

#include "Hardware.h"
#include "agent/Session.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace headroom::gui {
/** The API keys for the agent's three providers and the TONE3000 key with its
    sign-in, on a rack panel. It keeps itself in step with the settings and the
    session, so it can be shown anywhere (the settings popover opens it in a
    callout of its own). Both must outlive it. */
class KeysPanel final : public juce::Component, private juce::ChangeListener {
public:
  KeysPanel(agent::Session &, agent::AgentSettings &);
  ~KeysPanel() override;

  static constexpr int panelWidth = 580, panelHeight = 440;

  /** Lets go of the session and the settings, for when they're about to go: the
      callout holding this can outlive the editor that opened it. It does
      nothing from then on. */
  void detach();

  void paint(juce::Graphics &) override;
  void resized() override;

private:
  struct KeyRow {
    juce::String name;
    juce::TextEditor field;
    juce::HyperlinkButton link;
    juce::Rectangle<int> labelArea;
  };

  void changeListenerCallback(juce::ChangeBroadcaster *) override;
  void fillFromSettings();
  void refreshTone3000();
  bool connected() const;
  bool waitingForCode() const;

  agent::Session &session;
  agent::AgentSettings &settings;
  std::array<KeyRow, 4> rows;
  juce::TextButton connectButton{"sign in"};
  juce::HyperlinkButton codeLink;
  juce::TextEditor notes;
  juce::Rectangle<int> statusArea, messageArea, screenArea;
  PaintCache frame;
  bool secretKeyRefused = false, detached = false;
};
} // namespace headroom::gui
