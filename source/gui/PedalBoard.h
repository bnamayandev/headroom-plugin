#pragma once

#include "Widgets.h"

#include <array>
#include <memory>

namespace headroom::gui {
/** One stomp box as a neon card: three knobs across the top, its name on a
   plate and what it does to a signal on a little scope, then its light and
   footswitch. Its neon says what kind of pedal it is; while it's on, the plate,
   the scope and the frame light up in it. */
class Pedal final : public juce::Component {
public:
  /** What its scope shows, as a waveform or a response. */
  enum class Art {
    wah,
    compressor,
    fuzz,
    overdrive,
    distortion,
    boost,
    phaser,
    flanger,
    chorus,
    tremolo,
    delay,
    reverb
  };

  struct Spec {
    const char *name;
    const char *onId;
    std::array<std::pair<const char *, const char *>, 3> knobs; // id, title
    const char *tooltip;
    juce::uint32 neon; // yellow for filters, dynamics and time; magenta for
                       // drive; cyan for modulation
    Art art;
  };

  Pedal(juce::AudioProcessorValueTreeState &state, const Spec &spec);

  void paint(juce::Graphics &) override;
  void resized() override;
  bool isOn() const noexcept { return footswitch.getToggleState(); }

private:
  void setOn(bool on);
  void paintCard(juce::Graphics &) const;

  Spec spec;
  std::array<std::unique_ptr<Knob>, 3> knobs;
  Footswitch footswitch;
  std::unique_ptr<juce::ParameterAttachment> onAttachment;
  juce::Rectangle<int> nameArea, artArea;
  juce::Rectangle<float> led;
  PaintCache card;
};

/** The pedal page: pedals in signal order, left to right, on a board in two
   rows: the ones into the amp above, the ones after the cab below. */
class PedalBoard final : public juce::Component {
public:
  explicit PedalBoard(juce::AudioProcessorValueTreeState &state);

  void paint(juce::Graphics &) override;
  void resized() override;

  int getNumActive() const noexcept;

private:
  static constexpr size_t perRow = 6;

  std::array<std::unique_ptr<Pedal>, 2 * perRow> pedals;
  juce::Rectangle<int> ampSlot;
  PaintCache board;
};
} // namespace headroom::gui
