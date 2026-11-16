#include "Settings.h"

#include "HeadroomLookAndFeel.h"
#include "state/DataDirectory.h"

namespace headroom::gui {
namespace {
constexpr auto neonKey = "neon";
constexpr auto normaliseVolumeKey = "normaliseVolume";

juce::String nameOf(Neon neon) {
  switch (neon) {
  case Neon::yellow:
    return "yellow";
  case Neon::cyan:
    return "cyan";
  case Neon::magenta:
    break;
  }
  return "magenta";
}

Neon neonNamed(const juce::String &name) {
  for (const auto neon : SettingsPanel::neons)
    if (name == nameOf(neon))
      return neon;
  return colours::defaultNeon;
}

juce::PropertiesFile::Options fileOptions() {
  juce::PropertiesFile::Options options;
  options.storageFormat = juce::PropertiesFile::storeAsXML;
  options.millisecondsBeforeSaving = 500;
  return options;
}
} // namespace

Settings::Settings() : file(getFile(), fileOptions()) {
  // The free hue the editor used to take, "accentHue", is left alone: the
  // palette has only these neons in it.
  colours::setNeon(neonNamed(file.getValue(neonKey)));
}

juce::File Settings::getFile() {
  return getDataDirectory().getChildFile("settings.xml");
}

void Settings::setNeon(Neon neon) {
  colours::setNeon(neon);
  getFile().getParentDirectory().createDirectory();
  file.setValue(neonKey, nameOf(neon));
  sendChangeMessage();
}

bool Settings::isNormalisingVolume() const {
  return file.getBoolValue(normaliseVolumeKey, true);
}

void Settings::setNormalisingVolume(bool on) {
  if (on == isNormalisingVolume())
    return;
  getFile().getParentDirectory().createDirectory();
  file.setValue(normaliseVolumeKey, on);
  sendChangeMessage();
}

SettingsPanel::SettingsPanel(std::function<void()> openKeys, std::optional<AppActions> app)
    : hasAppSection(app.has_value()) {
  settings->addChangeListener(this);
  normaliseButton.setToggleState(settings->isNormalisingVolume(),
                                 juce::dontSendNotification);
  normaliseButton.setTooltip(
      "Plays every capture and IR at the same volume, measured as each one "
      "loads. Off, each plays as loud as it was made");
  normaliseButton.onClick = [this] {
    settings->setNormalisingVolume(normaliseButton.getToggleState());
  };
  addAndMakeVisible(normaliseButton);
  addAction(keysButton, "Your Claude, ChatGPT and Gemini API keys for the tone agent, and your TONE3000 sign-in",
            std::move(openKeys));
  if (app.has_value()) {
    addAction(audioButton, "Interface, inputs, sample rate and buffer size",
              std::move(app->audioSettings));
    addAction(saveButton, "Save the amp's whole state to a file",
              std::move(app->saveState));
    addAction(loadButton, "Load a state saved to a file",
              std::move(app->loadState));
    addAction(resetButton, "Reset the amp to its default state",
              std::move(app->resetState));
  }
  setSize(panelWidth, panelHeight + (hasAppSection ? appSectionHeight : 0));
}

SettingsPanel::~SettingsPanel() { settings->removeChangeListener(this); }

void SettingsPanel::changeListenerCallback(juce::ChangeBroadcaster *) {
  normaliseButton.setToggleState(settings->isNormalisingVolume(),
                                 juce::dontSendNotification);
  repaint();
}

void SettingsPanel::addAction(juce::TextButton &button,
                              const juce::String &tooltip,
                              std::function<void()> action) {
  button.setTooltip(tooltip);
  button.onClick = [this, action = std::move(action)] {
    if (auto *box = findParentComponentOfClass<juce::CallOutBox>())
      box->dismiss();
    juce::MessageManager::callAsync(action); // queued behind the dismissal
  };
  addAndMakeVisible(button);
}

void SettingsPanel::resized() {
  auto b = getLocalBounds().reduced(14, 12);
  if (hasAppSection) {
    audioCaption = b.removeFromTop(16);
    b.removeFromTop(8);
    audioButton.setBounds(b.removeFromTop(28));
    b.removeFromTop(14);
    stateCaption = b.removeFromTop(16);
    b.removeFromTop(8);
    auto row = b.removeFromTop(28);
    const int gap = 6, width = (row.getWidth() - 2 * gap) / 3;
    saveButton.setBounds(row.removeFromLeft(width));
    row.removeFromLeft(gap);
    loadButton.setBounds(row.removeFromLeft(width));
    row.removeFromLeft(gap);
    resetButton.setBounds(row);
    b.removeFromTop(20);
  }
  agentCaption = b.removeFromTop(16);
  b.removeFromTop(8);
  keysButton.setBounds(b.removeFromTop(28));
  b.removeFromTop(20);
  levelsCaption = b.removeFromTop(16);
  b.removeFromTop(8);
  normaliseButton.setBounds(b.removeFromTop(26));
  b.removeFromTop(20);
  caption = b.removeFromTop(16);
  b.removeFromTop(10);
  swatchRow = b.removeFromTop(30);
}

juce::Rectangle<float> SettingsPanel::swatch(size_t index) const {
  constexpr float gap = 8.0f;
  const float width = ((float)swatchRow.getWidth() - gap * (float)(neons.size() - 1)) /
                      (float)neons.size();
  return swatchRow.toFloat()
      .withX((float)swatchRow.getX() + (width + gap) * (float)index)
      .withWidth(width);
}

void SettingsPanel::paint(juce::Graphics &g) {
  using namespace colours;
  g.setColour(lineSoft);
  if (hasAppSection)
    g.fillRect(agentCaption.withY(agentCaption.getY() - 10).withHeight(1));
  g.fillRect(levelsCaption.withY(levelsCaption.getY() - 10).withHeight(1));
  g.fillRect(caption.withY(caption.getY() - 10).withHeight(1));
  g.setFont(gui::caption());
  g.setColour(muted);
  if (hasAppSection) {
    g.drawText("AUDIO", audioCaption, juce::Justification::centredLeft);
    g.drawText("STATE", stateCaption, juce::Justification::centredLeft);
  }
  g.drawText("TONE AGENT", agentCaption, juce::Justification::centredLeft);
  g.drawText("LEVELS", levelsCaption, juce::Justification::centredLeft);
  g.drawText("NEON", caption, juce::Justification::centredLeft);
  g.setColour(accent());
  g.drawText(nameOf(getNeon()).toUpperCase(), caption, juce::Justification::centredRight);

  // A key per neon: the chosen one lit solid, framed in white.
  for (size_t i = 0; i < neons.size(); ++i) {
    const auto shape = bevelBox(swatch(i).reduced(0.5f), 6.0f);
    const auto colour = neon(neons[i]);
    if (getNeon() == neons[i]) {
      fillNeon(g, shape, colour, 0.9f);
      g.setColour(colours::text);
      g.strokePath(shape, juce::PathStrokeType(1.5f));
    } else {
      g.setColour(colour.withAlpha(0.22f));
      g.fillPath(shape);
      g.setColour(colour.withAlpha(0.6f));
      g.strokePath(shape, juce::PathStrokeType(1.0f));
    }
  }
}

void SettingsPanel::mouseDown(const juce::MouseEvent &e) {
  for (size_t i = 0; i < neons.size(); ++i)
    if (swatch(i).contains(e.position)) {
      settings->setNeon(neons[i]);
      return;
    }
}

void SettingsPanel::mouseMove(const juce::MouseEvent &e) {
  bool hot = false;
  for (size_t i = 0; i < neons.size() && !hot; ++i)
    hot = swatch(i).contains(e.position);
  setMouseCursor(hot ? juce::MouseCursor::PointingHandCursor
                     : juce::MouseCursor::NormalCursor);
}
} // namespace headroom::gui
