#include "KeysPanel.h"

#include "HeadroomLookAndFeel.h"
#include "Widgets.h"

namespace headroom::gui {
namespace {
using agent::AgentSettings;
using agent::ProviderId;

constexpr std::array<ProviderId, 3> providers{ProviderId::anthropic, ProviderId::openai,
                                              ProviderId::gemini};
} // namespace

KeysPanel::KeysPanel(agent::Session &sess, agent::AgentSettings &sett)
    : session(sess), settings(sett) {
  const std::array<std::pair<const char *, const char *>, 3> where{
      {{"Claude", "https://console.anthropic.com/settings/keys"},
       {"ChatGPT", "https://platform.openai.com/api-keys"},
       {"Gemini", "https://aistudio.google.com/apikey"}}};
  for (size_t i = 0; i < providers.size(); ++i) {
    auto &row = rows[i];
    const auto id = providers[i];
    row.name = where[i].first;
    styleSecret(row.field);
    row.field.setTextToShowWhenEmpty("paste your " + row.name + " API key",
                                     colours::mutedSoft);
    row.field.onTextChange = [this, id, &row] {
      if (detached)
        return;
      settings.setApiKey(id, row.field.getText());
      repaint();
    };
    row.link.setButtonText("get a key");
    row.link.setURL(juce::URL(where[i].second));
    styleLink(row.link);
    addAndMakeVisible(row.field);
    addAndMakeVisible(row.link);
  }

  auto &t3k = rows[3];
  t3k.name = "TONE3000";
  styleSecret(t3k.field);
  t3k.field.setTextToShowWhenEmpty("publishable key: t3k_pub_...", colours::mutedSoft);
  t3k.field.onTextChange = [this, &t3k] {
    if (detached)
      return;
    const auto typed = t3k.field.getText().trim();
    secretKeyRefused = agent::Tone3000::looksLikeSecretKey(typed);
    if (!secretKeyRefused)
      settings.set(AgentSettings::tone3000Key, typed);
    refreshTone3000();
  };
  t3k.link.setButtonText("get a key");
  t3k.link.setURL(juce::URL("https://www.tone3000.com/settings"));
  styleLink(t3k.link);
  addAndMakeVisible(t3k.field);
  addAndMakeVisible(t3k.link);

  addAndMakeVisible(connectButton);
  gui::setPrimary(connectButton);
  connectButton.onClick = [this] {
    if (detached)
      return;
    if (waitingForCode())
      session.stop();
    else if (connected())
      session.disconnectTone3000();
    else
      session.connectTone3000();
    refreshTone3000();
  };
  connectButton.setTooltip("Sign in once with a code, on any device. No browser "
                           "opens, and it stays signed in.");

  codeLink.setButtonText("open it here");
  styleLink(codeLink);
  codeLink.setTooltip("Only if you want to: this opens the sign-in page with the "
                      "code filled in");
  addChildComponent(codeLink);

  styleAsText(notes, sans(14.0f), colours::textSoft);
  notes.setText(
      "Requests go straight from this computer to the company you choose, on "
      "your account and at your cost. Keys are saved only in " +
      AgentSettings::getDefaultFile().getFullPathName() +
      ", which only your user can read. They're never put in presets or "
      "sessions.\n\n"
      "TONE3000 is the community library of NAM captures and IRs. You sign in "
      "once with your own account there, by typing a code on any device, and "
      "the amp keeps itself signed in. Each capture stays credited to whoever "
      "made it.");
  addAndMakeVisible(notes);

  fillFromSettings();
  session.addChangeListener(this);
  settings.addChangeListener(this);
  setSize(panelWidth, panelHeight);
}

KeysPanel::~KeysPanel() { detach(); }

void KeysPanel::detach() {
  if (detached)
    return;
  detached = true;
  session.removeChangeListener(this);
  settings.removeChangeListener(this);
  setEnabled(false);
}

void KeysPanel::changeListenerCallback(juce::ChangeBroadcaster *) { fillFromSettings(); }

void KeysPanel::fillFromSettings() {
  if (detached)
    return;
  for (size_t i = 0; i < providers.size(); ++i)
    if (!rows[i].field.hasKeyboardFocus(false))
      rows[i].field.setText(settings.getApiKey(providers[i]), false);
  if (!rows[3].field.hasKeyboardFocus(false)) {
    rows[3].field.setText(settings.get(AgentSettings::tone3000Key), false);
    secretKeyRefused = false; // what was refused is gone from the field
  }
  refreshTone3000();
}

void KeysPanel::refreshTone3000() {
  if (detached)
    return;
  const auto code = session.getSignInCode();
  connectButton.setButtonText(waitingForCode() ? "cancel"
                              : connected()    ? "sign out"
                                               : "sign in");
  connectButton.setEnabled(connected() || waitingForCode() ||
                           settings.get(AgentSettings::tone3000Key).isNotEmpty());
  codeLink.setVisible(code.linkWithCode.isNotEmpty() &&
                      agent::isSafeLink(code.linkWithCode));
  if (codeLink.isVisible())
    codeLink.setURL(juce::URL(code.linkWithCode));
  resized();
  repaint();
}

void KeysPanel::resized() {
  screenArea = getLocalBounds().reduced(14, 0).withTrimmedTop(32).withTrimmedBottom(14);
  auto b = screenArea.reduced(14, 10);
  for (auto &row : rows) {
    auto line = b.removeFromTop(30);
    row.labelArea = line.removeFromLeft(96);
    row.link.setBounds(line.removeFromRight(76));
    line.removeFromRight(8);
    row.field.setBounds(line);
    b.removeFromTop(8);
  }
  auto status = b.removeFromTop(28);
  status.removeFromLeft(96); // under the fields
  connectButton.setBounds(status.removeFromLeft(112));
  status.removeFromLeft(12);
  if (codeLink.isVisible())
    codeLink.setBounds(status.removeFromRight(110));
  statusArea = status;
  // What it means gets a row of its own, as it can run to two lines.
  b.removeFromTop(4);
  messageArea = b.removeFromTop(40).withTrimmedLeft(96);
  b.removeFromTop(10);
  notes.setBounds(b);
}

void KeysPanel::paint(juce::Graphics &g) {
  frame.draw(g, getLocalBounds(), 0, [this](juce::Graphics &c) {
    drawPanel(c, getLocalBounds(), "keys");
    drawRecess(c, screenArea.toFloat(), 4.0f);
  });
  g.setFont(caption());
  for (const auto &row : rows) {
    g.setColour(colours::muted);
    g.drawText(row.name.toUpperCase(), row.labelArea, juce::Justification::centredLeft,
               false);
  }
  if (detached)
    return;
  const auto code = session.getSignInCode();
  g.setFont(sans(13.5f));
  if (code.userCode.isNotEmpty()) {
    const auto font = readout(18.0f, 0.06f);
    g.setFont(font);
    g.setColour(colours::accent());
    g.drawText(code.userCode, statusArea, juce::Justification::centredLeft, false);
    g.setFont(sans(13.5f));
    g.setColour(colours::textSoft);
    g.drawFittedText("enter this code at " +
                         code.verificationUri.fromFirstOccurrenceOf("://", false, false) +
                         ", on any device",
                     messageArea, juce::Justification::topLeft, 2);
    return;
  }
  g.setColour(secretKeyRefused ? colours::accent() : colours::textSoft);
  g.drawFittedText(secretKeyRefused
                       ? "That's a secret key, which must stay on a server. Use your "
                         "publishable one."
                   : connected() ? "signed in, and it stays that way"
                                 : "not signed in: only the built-in capture is available",
                   messageArea, juce::Justification::topLeft, 2);
}

bool KeysPanel::connected() const {
  return settings.get(AgentSettings::tone3000Refresh).isNotEmpty() ||
         settings.get(AgentSettings::tone3000Access).isNotEmpty();
}

bool KeysPanel::waitingForCode() const { return session.getSignInCode().userCode.isNotEmpty(); }
} // namespace headroom::gui
