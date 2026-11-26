#pragma once

#include "Neon.h"
#include "agent/Session.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace headroom::gui {
/** The agent tab: a chat with a model that researches tones and sets the amp up,
    using the player's own API keys, which are entered under SETTINGS. */
class AgentPage final : public juce::Component, private juce::ChangeListener {
public:
  AgentPage(agent::Session &, agent::AgentSettings &);
  ~AgentPage() override;

  void paint(juce::Graphics &) override;
  void resized() override;

  /** At the editor's text rate, while this page shows. */
  void refresh();
  /** At the editor's frame rate, while this page shows: moves the globe on the
      empty screen and the lights that say it's working. */
  void animate(float secondsSinceLast);
  /** The tab came into view. */
  void pageShown();

  /** Opens the API keys, for the notice that says to add one. Set by the
      editor, which owns the settings popover the keys live in. */
  std::function<void()> onOpenKeys;

private:
  class Transcript;

  void changeListenerCallback(juce::ChangeBroadcaster *) override;
  void refreshProviderControls();
  void sendComposed();
  /** Brings the transcript up to date, following it down if it was at the end. */
  void updateTranscript();
  void setComposer(const juce::String &text);
  void openKeys();
  /** Says what's missing, with a way to the keys, until it's added or sent. */
  void showMissingKey(bool tone3000);
  void updateNotice();
  void hideNotice();
  /** How much of the title row the status takes, with the light before it. */
  float statusWidth() const;
  juce::Rectangle<float> statusArea() const;

  agent::Session &session;
  agent::AgentSettings &settings;

  juce::ComboBox providerBox, modelBox;
  juce::TextButton newChatButton{"new chat"},
      browseButton{"browse tone3000"};
  juce::ImageComponent tone3000Logo, t3kMark;
  juce::HyperlinkButton poweredBy;

  // Before the transcript, which reads it as it's made.
  enum class Missing { nothing, apiKey, tone3000 } missing = Missing::nothing;
  std::unique_ptr<Transcript> transcript;
  juce::Viewport viewport;
  juce::Label noticeLabel;
  juce::TextButton noticeButton{"open api keys"};

  juce::TextEditor composer;
  juce::TextButton sendButton{"send"};

  juce::Rectangle<int> panelArea, screenArea, noticeArea;
  juce::Rectangle<float> headerRule;
  PaintCache frame;
  juce::String statusText;
  bool wasBusy = false;
  float clock = 0.0f; // seconds, for what moves; wraps
};
} // namespace headroom::gui
