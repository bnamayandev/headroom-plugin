#pragma once

#include "PluginProcessor.h"
#include "gui/AgentPage.h"
#include "gui/KeysPanel.h"
#include "gui/HeadroomLookAndFeel.h"
#include "gui/PedalBoard.h"
#include "gui/Settings.h"
#include "gui/TunerPage.h"
#include "gui/Widgets.h"

namespace headroom {
class HeadroomEditor final : public juce::AudioProcessorEditor,
                             private juce::Timer,
                             private juce::ChangeListener {
public:
  explicit HeadroomEditor(HeadroomProcessor &);
  ~HeadroomEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;

  static constexpr int designWidth = 980, designHeight = 648;
  static constexpr int headerHeight = 64, footerHeight = 48;

private:
  /** Everything is laid out once in a fixed 980x648 design space on this
     canvas, which is then scaled uniformly to whatever size the host or window
     manager gives the editor. */
  struct Canvas final : juce::Component {
    explicit Canvas(HeadroomEditor &e) : owner(e) {}
    void paint(juce::Graphics &g) override { owner.paintCanvas(g); }
    void resized() override { owner.layoutCanvas(); }
    void paintOverChildren(juce::Graphics &g) override { owner.paintGlass(g); }
    void mouseDown(const juce::MouseEvent &e) override {
      owner.canvasMouseDown(e);
    }
    void mouseDrag(const juce::MouseEvent &e) override {
      owner.canvasMouseDrag(e);
    }
    HeadroomEditor &owner;
  };

  void paintCanvas(juce::Graphics &);
  void paintFixedParts(juce::Graphics &);
  void paintFooter(juce::Graphics &);
  void paintBeatLights(juce::Graphics &);
  void paintLogo(juce::Graphics &);
  void paintLiveBadge(juce::Graphics &);
  /** The tube's glass, over everything the canvas holds. */
  void paintGlass(juce::Graphics &);
  /** Whether the logo's picture is tearing, as it does every few seconds. */
  bool isGlitching() const;
  void layoutCanvas();
  void canvasMouseDown(const juce::MouseEvent &);
  void canvasMouseDrag(const juce::MouseEvent &);
  bool isStandalone() const {
    return audioProcessor.wrapperType ==
           juce::AudioProcessor::wrapperType_Standalone;
  }

  void timerCallback() override;
  void changeListenerCallback(juce::ChangeBroadcaster *) override;

  void refreshModelStatus();
  void refreshAmpDisplays(float outputPeak);
  void applyPresetColour();
  void refreshStats();
  void refreshIrStatus();
  void refreshPresetList();
  void chooseModelFile();
  void chooseIrFile();
  void showBuiltinModels();
  void showBuiltinIrs();
  void showSettings();
  void showKeys();
  juce::Component::SafePointer<gui::KeysPanel> keysPanel;
  void applyAccent();
  void savePresetDialog();
  void stepPreset(int delta);
  void showPage(int page);
  void refreshPedalsTab();
  void refreshMetronome();
  void tapTempo();
  void stepTranspose(int delta);

  HeadroomProcessor &audioProcessor;
  juce::SharedResourcePointer<gui::Settings> settings; // sets the accent first
  gui::HeadroomLookAndFeel lookAndFeel;
  Canvas canvas{*this};
  juce::TooltipWindow tooltips{this, 600};

  // Header
  juce::TextButton ampTab{"amp"}, pedalsTab{"pedals"}, tunerTab{"tuner"},
      agentTab{"agent"};
  juce::ComboBox presetBox;
  juce::TextButton prevPreset{"<"}, nextPreset{">"}, savePreset{"save tone"};
  juce::TextButton settingsButton{"settings"};

  // The standalone app's window has no title bar: the header stands in for it.
  juce::TextButton minimiseButton{"-"},
      closeButton{juce::String::charToString(0x00d7)};
  juce::ComponentDragger windowDragger;
  bool draggingWindow = false;

  // Pages: the amp controls live on ampPage (laid out in canvas coordinates),
  // the pedals on the board.
  juce::Component ampPage;
  gui::PedalBoard pedalBoard;
  gui::TunerPage tunerPage;
  gui::AgentPage agentPage;

  // Input
  gui::Knob inputKnob, gateKnob;
  juce::ToggleButton gateButton{"gate"};
  juce::AudioProcessorValueTreeState::ButtonAttachment gateAttachment;
  gui::LevelMeter inputMeter;

  // Amp
  gui::WireCube cube;
  juce::Label modelName, modelDetails;
  juce::TextButton loadModelButton{"load .nam..."},
      builtinModelButton{"built-in"}, unloadModelButton{"unload"};

  // Cab
  gui::Speaker speaker;
  juce::Label irName, irDetails;
  juce::TextButton loadIrButton{"load ir..."}, builtinIrButton{"built-in"};
  juce::ToggleButton cabButton{"cabinet on"};
  juce::AudioProcessorValueTreeState::ButtonAttachment cabAttachment;

  // EQ + output. The analyser spans the fader columns, an octave each, so
  // every band's fader sits over its centre frequency.
  gui::Spectrum eqSpectrum{
      Equaliser::frequencies.front() / juce::MathConstants<double>::sqrt2,
      Equaliser::frequencies.back() * juce::MathConstants<double>::sqrt2};
  std::array<std::unique_ptr<gui::Fader>, Equaliser::numBands> eqFaders;
  gui::Knob outputKnob;
  gui::LevelMeter outputMeter;

  // Footer: metronome, transposer, stats
  juce::TextButton metronomeButton{"start"}, tapButton{"tap"};
  juce::Slider bpmSlider{juce::Slider::LinearBar, juce::Slider::TextBoxLeft},
      metronomeLevelSlider{juce::Slider::LinearBar, juce::Slider::TextBoxLeft};
  juce::AudioProcessorValueTreeState::SliderAttachment bpmAttachment,
      metronomeLevelAttachment;
  juce::ComboBox beatsBox;
  juce::AudioProcessorValueTreeState::ComboBoxAttachment beatsAttachment;
  juce::TextButton transposeDown{"-"}, transposeUp{"+"};
  juce::Label transposeLabel;
  std::unique_ptr<juce::ParameterAttachment> transposeAttachment;
  juce::Label cpuLabel, latencyLabel;

  juce::Rectangle<int> logoArea, stripesBand, tabTray, inputPanel, ampPanel, ampDisplay,
      ampCaption, cabPanel, eqPanel, outputPanel, metronomeCaption, beatLeds,
      transposeCaption, statsArea;
  juce::Rectangle<float> gateLed, liveBadge;
  gui::PaintCache fixedParts, logoCache, glassCache;
  float animationTime = 0.0f; // seconds, for what moves on its own
  bool logoGlitched = false;  // as last painted
  bool modelLive = false;
  std::vector<double> tapTimes;
  std::uint32_t lastClickCount = 0;
  double lastClickTime = 0.0;
  std::unique_ptr<juce::FileChooser> chooser;
  juce::File lastModelDirectory, lastIrDirectory;
  int textRefreshCounter = 0;
  juce::String creditedIrSource; // its credit is read from disk only when it changes
  agent::Tone3000::Credit irCredit;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeadroomEditor)
};
} // namespace headroom
