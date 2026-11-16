#include "StandaloneApp.h"

#include <juce_core/juce_core.h>

#if defined(JucePlugin_Build_Standalone) && JucePlugin_Build_Standalone
#include "gui/HeadroomLookAndFeel.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

namespace headroom::standalone {
namespace {
/** Where JUCE's standalone keeps the settings of an app with this name. */
juce::PropertiesFile::Options settingsOptions(const juce::String &name) {
  juce::PropertiesFile::Options options;
  options.applicationName = name;
  options.filenameSuffix = ".settings";
  options.osxLibrarySubFolder = "Application Support";
#if JUCE_LINUX || JUCE_BSD
  options.folderName = "~/.config";
#endif
  return options;
}

/** JUCE's standalone window without its title bar: the editor's header does
    that job. JUCE keeps a thin border around the content for resizing, painted
    here in the editor's background. */
class Window final : public juce::StandaloneFilterWindow {
public:
  Window(const juce::String &title,
         std::unique_ptr<juce::StandalonePluginHolder> holder)
      : StandaloneFilterWindow(title, gui::colours::background,
                               std::move(holder)) {
    // The Options button is private to JUCE's window, and the only text button
    // directly on it. Hidden, not just shrunk, so it can't take keyboard focus.
    for (auto *child : getChildren())
      if (auto *options = dynamic_cast<juce::TextButton *>(child))
        options->setVisible(false);

    const auto content = getContentComponent()->getBounds();
    setTitleBarButtonsRequired(0, false);
    setTitleBarHeight(0);
    setContentComponentSize(content.getWidth(), content.getHeight());
  }
};

/** Stands in for JUCE's StandaloneFilterApp, which is final and creates its
    own window. Uses the same settings file, so saved state carries over. */
class App final : public juce::JUCEApplication {
public:
  App() {
    const auto options = settingsOptions(juce::CharPointer_UTF8(JucePlugin_Name));
    // The audio setup and last state, saved under the amp's old name until it was
    // renamed Headroom.
    const auto file = options.getDefaultFile();
    const auto legacy = settingsOptions("Evil Amplifier").getDefaultFile();
    if (!file.exists() && legacy.existsAsFile() && file.getParentDirectory().createDirectory())
      legacy.moveFileTo(file);
    properties.setStorageParameters(options);
  }

  const juce::String getApplicationName() override {
    return juce::CharPointer_UTF8(JucePlugin_Name);
  }
  const juce::String getApplicationVersion() override {
    return JucePlugin_VersionString;
  }
  bool moreThanOneInstanceAllowed() override { return true; }
  void anotherInstanceStarted(const juce::String &) override {}

  void initialise(const juce::String &) override {
    if (juce::Desktop::getInstance().getDisplays().displays.isEmpty()) {
      quit();
      return;
    }
    window = std::make_unique<Window>(
        getApplicationName(), std::make_unique<juce::StandalonePluginHolder>(
                                  properties.getUserSettings(), false));
    window->setVisible(true);
  }

  void shutdown() override {
    window = nullptr;
    properties.saveIfNeeded();
  }

  void systemRequestedQuit() override {
    if (window != nullptr)
      window->pluginHolder->savePluginState();

    if (juce::ModalComponentManager::getInstance()->cancelAllModalComponents())
      juce::Timer::callAfterDelay(100, [] {
        if (auto *app = getInstance())
          app->systemRequestedQuit();
      });
    else
      quit();
  }

  Window *getWindow() const { return window.get(); }

private:
  juce::ApplicationProperties properties;
  std::unique_ptr<Window> window;
};

Window *findWindow() {
  auto *app = dynamic_cast<App *>(juce::JUCEApplicationBase::getInstance());
  return app != nullptr ? app->getWindow() : nullptr;
}
} // namespace

void showAudioSettings() {
  if (auto *holder = juce::StandalonePluginHolder::getInstance())
    holder->showAudioSettingsDialog();
}

void saveState() {
  if (auto *holder = juce::StandalonePluginHolder::getInstance())
    holder->askUserToSaveState();
}

void loadState() {
  if (auto *holder = juce::StandalonePluginHolder::getInstance())
    holder->askUserToLoadState();
}

void resetToDefaultState() {
  if (auto *window = findWindow())
    window->resetToDefaultState();
}

void minimiseWindow() {
  if (auto *window = findWindow())
    window->setMinimised(true);
}

void closeWindow() {
  if (auto *window = findWindow())
    window->closeButtonPressed();
}
} // namespace headroom::standalone

// With JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP set, JUCE's wrapper calls this.
JUCE_CREATE_APPLICATION_DEFINE(headroom::standalone::App)

#else

namespace headroom::standalone {
void showAudioSettings() {}
void saveState() {}
void loadState() {}
void resetToDefaultState() {}
void minimiseWindow() {}
void closeWindow() {}
} // namespace headroom::standalone

#endif
