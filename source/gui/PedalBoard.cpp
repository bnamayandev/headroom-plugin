#include "PedalBoard.h"

#include "HeadroomLookAndFeel.h"
#include "state/Parameters.h"

namespace headroom::gui {
namespace {
constexpr int captionHeight = 24, gap = 10, rowGap = 8, ampBlockWidth = 54;

const std::array<Pedal::Spec, 12> &pedalSpecs() {
  namespace p = params;
  static const std::array<Pedal::Spec, 12> specs{{
      {"wah", p::wahOn,
       {{{p::wahPosition, "Pedal"}, {p::wahAuto, "Auto"},
         {p::wahResonance, "Peak"}}},
       "After a Dunlop Cry Baby. Pedal is the rocker, heel to toe; Peak at noon "
       "is the stock pedal. Auto isn't on a real one: it opens the filter as you "
       "pick harder, for an envelope filter.",
       0xfff5ff3b, Pedal::Art::wah},
      {"compressor", p::compOn,
       {{{p::compSustain, "Sens"}, {p::compBlend, "Blend"},
         {p::compLevel, "Output"}}},
       "After an MXR Dyna Comp: its Sensitivity and Output. Blend isn't on the "
       "original; all the way up is the original.",
       0xfff5ff3b, Pedal::Art::compressor},
      {"fuzz", p::fuzzOn,
       {{{p::fuzzAmount, "Sustain"}, {p::fuzzTone, "Tone"},
         {p::fuzzLevel, "Volume"}}},
       "An EHX Big Muff Pi circuit, its knobs where a real one's are. Tone scoops "
       "the mids at noon, dark and wooly to the left, cutting to the right.",
       0xffff2bd6, Pedal::Art::fuzz},
      {"overdrive", p::odOn,
       {{{p::odDrive, "Drive"}, {p::odTone, "Tone"}, {p::odLevel, "Level"}}},
       "An Ibanez Tube Screamer circuit, its knobs where a real one's are. Thins "
       "the lows and pushes the mids. Low drive and high level tightens a dirty "
       "amp; on a clean amp it's a crunch tone of its own.",
       0xffff2bd6, Pedal::Art::overdrive},
      {"distortion", p::distOn,
       {{{p::distAmount, "Dist"}, {p::distTone, "Filter"},
         {p::distLevel, "Volume"}}},
       "A ProCo RAT circuit, its knobs where a real one's are: nearly clean with "
       "Dist down, heavy from noon up. Filter darkens clockwise, as on a real one.",
       0xffff2bd6, Pedal::Art::distortion},
      {"boost", p::boostOn,
       {{{p::boostGain, "Gain"}, {p::boostBass, "Bass"},
         {p::boostTreble, "Treble"}}},
       "After an MXR Micro Amp: up to 26 dB of clean gain, to hit the amp harder "
       "or lift a solo. Bass and Treble aren't on the original; at noon they're "
       "flat.",
       0xffff2bd6, Pedal::Art::boost},
      {"phaser", p::phaserOn,
       {{{p::phaserRate, "Speed"}, {p::phaserDepth, "Depth"},
         {p::phaserFeedback, "Feedback"}}},
       "After an MXR Phase 90: Speed is its one knob. Depth all the way up is the "
       "original's sweep. Feedback sharpens it into a vocal resonance.",
       0xff00f0ff, Pedal::Art::phaser},
      {"flanger", p::flangerOn,
       {{{p::flangerRate, "Speed"}, {p::flangerDepth, "Width"},
         {p::flangerFeedback, "Regen"}}},
       "After an MXR M117. A slow speed and high regen for the classic whoosh.",
       0xff00f0ff, Pedal::Art::flanger},
      {"chorus", p::chorusOn,
       {{{p::chorusRate, "Rate"}, {p::chorusDepth, "Depth"},
         {p::chorusMix, "Mix"}}},
       "After a Boss CE-2. Mix isn't on the original: noon is the original, and "
       "all the way up its vibrato.",
       0xff00f0ff, Pedal::Art::chorus},
      {"tremolo", p::tremOn,
       {{{p::tremRate, "Rate"}, {p::tremDepth, "Depth"},
         {p::tremShape, "Wave"}}},
       "After a Boss TR-2. Wave goes from a triangle, a smooth throb, to a square, "
       "a hard chop.",
       0xff00f0ff, Pedal::Art::tremolo},
      {"delay", p::delayOn,
       {{{p::delayTime, "Delay"}, {p::delayFeedback, "Regen"},
         {p::delayMix, "Mix"}}},
       "After an MXR Carbon Copy: repeats get darker the longer the delay. It goes "
       "well past the original's 600 ms.",
       0xfff5ff3b, Pedal::Art::delay},
      {"reverb", p::reverbOn,
       {{{p::reverbDecay, "Decay"}, {p::reverbTone, "Tone"},
         {p::reverbMix, "Mix"}}},
       "Plate reverb. Not modelled on a particular pedal.",
       0xfff5ff3b, Pedal::Art::reverb},
}};
  return specs;
}

/** A trace of what the pedal does to a signal: `shape` takes how far along it
    is, 0..1, and gives the height, -1..1. */
juce::Path waveform(juce::Rectangle<float> area, const std::function<float(float)> &shape,
                    int points = 140) {
  juce::Path path;
  for (int i = 0; i <= points; ++i) {
    const float t = (float)i / (float)points;
    const juce::Point<float> p(area.getX() + t * area.getWidth(),
                               area.getCentreY() - juce::jlimit(-1.0f, 1.0f, shape(t)) *
                                                       area.getHeight() * 0.5f);
    if (i == 0)
      path.startNewSubPath(p);
    else
      path.lineTo(p);
  }
  return path;
}

/** What the pedal does to a signal, traced on its scope; `glow` blooms it. */
void drawArt(juce::Graphics &g, juce::Rectangle<float> area, Pedal::Art art, juce::Colour ink,
             float glow) {
  using Art = Pedal::Art;
  constexpr float tau = juce::MathConstants<float>::twoPi;
  const auto draw = [&](const std::function<float(float)> &shape, float alpha = 0.9f) {
    strokeNeon(g, waveform(area, shape), ink.withMultipliedAlpha(alpha), 1.2f, glow * alpha);
  };
  switch (art) {
  case Art::wah: // a resonant peak, swept
    for (const auto &[peak, alpha] : {std::pair{0.25f, 0.35f}, std::pair{0.5f, 0.55f}, std::pair{0.75f, 0.95f}})
      draw([peak = peak](float t) {
             const float d = (t - peak) / 0.07f;
             return -0.82f + 1.75f * peak * std::exp(-d * d) + 0.45f * peak * std::exp(-d * d * 0.08f);
           }, alpha);
    break;
  case Art::compressor: // peaks held down to one height
    draw([=](float t) {
      const float env = 1.0f - 0.8f * t;
      return juce::jlimit(-0.42f, 0.42f, env * 1.5f * std::sin(tau * 9.0f * t)) +
             (t < 0.12f ? (0.12f - t) * 3.0f * std::sin(tau * 9.0f * t) : 0.0f);
    });
    break;
  case Art::fuzz: // a square wave, sputtering at the top
    draw([=](float t) {
      const float wave = std::sin(tau * 5.0f * t);
      return (wave > 0.0f ? 0.72f : -0.72f) + 0.16f * std::sin(tau * 83.0f * t);
    });
    break;
  case Art::overdrive: // a sine's peaks gently rounded off
    draw([=](float t) { return 0.82f * std::tanh(2.6f * std::sin(tau * 3.5f * t)); });
    break;
  case Art::distortion: // a sine's peaks cut flat
    draw([=](float t) { return juce::jlimit(-0.62f, 0.62f, 2.8f * std::sin(tau * 3.5f * t)); });
    break;
  case Art::boost: // the same wave, only bigger
    draw([=](float t) { return (0.16f + 0.82f * t * t) * std::sin(tau * 7.0f * t); });
    break;
  case Art::phaser: // a wave and its shifted copy
    draw([=](float t) { return 0.7f * std::sin(tau * 3.0f * t); }, 0.45f);
    draw([=](float t) { return 0.7f * std::sin(tau * 3.0f * t + 1.2f + 2.0f * t); });
    break;
  case Art::flanger: // a sweep, up and back
    draw([=](float t) {
      const float sweep = std::sin(juce::MathConstants<float>::pi * t);
      return 0.75f * std::sin(tau * (2.0f + 9.0f * sweep * sweep) * t);
    });
    break;
  case Art::chorus: // voices slightly apart
    for (const auto &[cycles, alpha] : {std::pair{3.0f, 0.35f}, std::pair{3.35f, 0.55f}, std::pair{3.7f, 0.9f}})
      draw([=, cycles = cycles](float t) { return 0.72f * std::sin(tau * cycles * t); }, alpha);
    break;
  case Art::tremolo: // the level pulsing
    draw([=](float t) {
      const float env = 0.5f + 0.5f * std::sin(tau * 2.5f * t - 1.0f);
      return (0.08f + 0.85f * env) * std::sin(tau * 22.0f * t);
    });
    break;
  case Art::delay: // the same note coming back quieter
    draw([=](float t) {
      const float position = t * 4.4f, phase = position - std::floor(position);
      const float height = std::pow(0.62f, std::floor(position));
      return height * std::exp(-phase * 5.0f) * std::sin(tau * 3.2f * phase);
    });
    break;
  case Art::reverb: // a dense tail dying away
    draw([=](float t) {
      return std::exp(-3.6f * t) *
             (0.62f * std::sin(tau * 21.0f * t) + 0.38f * std::sin(tau * 34.0f * t + 1.0f));
    });
    break;
  }
}
} // namespace

Pedal::Pedal(juce::AudioProcessorValueTreeState &state, const Spec &s)
    : spec(s) {
  for (size_t i = 0; i < knobs.size(); ++i) {
    knobs[i] = std::make_unique<Knob>(state, spec.knobs[i].first, spec.knobs[i].second,
                                      Knob::Size::pedal);
    addAndMakeVisible(*knobs[i]);
  }
  footswitch.setLight(juce::Colour(spec.neon));
  footswitch.setTooltip(juce::String(spec.tooltip) + " (click to switch)");
  footswitch.onClick = [this] {
    onAttachment->setValueAsCompleteGesture(isOn() ? 0.0f : 1.0f);
  };
  addAndMakeVisible(footswitch);
  onAttachment = std::make_unique<juce::ParameterAttachment>(
      *state.getParameter(spec.onId), [this](float v) { setOn(v >= 0.5f); });
  onAttachment->sendInitialUpdate();
}

void Pedal::setOn(bool on) {
  footswitch.setToggleState(on, juce::dontSendNotification);
  // Bypassed, its markings dim but stay readable, and its scale lights white.
  const juce::Colour light(spec.neon);
  for (auto &k : knobs)
    k->setColours(on ? light : light.withAlpha(0.6f), on ? light : colours::text.withAlpha(0.7f));
  repaint();
}

void Pedal::paintCard(juce::Graphics &g) const {
  using namespace colours;
  const bool on = isOn();
  const juce::Colour light(spec.neon);
  const auto body = getLocalBounds().toFloat().reduced(4.0f);
  const auto shape = bevelBox(body, 10.0f);
  g.setColour(panel);
  g.fillPath(shape);
  if (on) { // lit faintly from within
    g.setColour(light.withAlpha(0.05f));
    g.fillPath(shape);
  }
  strokeNeon(g, shape, light.withAlpha(on ? 0.95f : 0.3f), on ? 1.2f : 1.0f,
             on ? 0.7f : 0.0f);

  // The name on a plate, lit solid while it's on.
  const auto plate = nameArea.toFloat();
  const auto plateShape = bevelBox(plate, 5.0f);
  const auto name = juce::String(spec.name).toUpperCase();
  const auto nameFont = headline(12.5f, 0.0f);
  const auto letters = textPath(name, nameFont, plate.reduced(6.0f, 0.0f),
                                juce::Justification::centred, 1, 0.75f);
  if (on) {
    fillNeon(g, plateShape, light, 0.8f);
    g.setColour(black);
    g.fillPath(letters);
  } else {
    g.setColour(light.withAlpha(0.45f));
    g.strokePath(plateShape, juce::PathStrokeType(1.0f));
    g.setColour(light.withAlpha(0.6f));
    g.fillPath(letters);
  }

  // The scope, tracing what it does to a signal.
  const auto scope = artArea.toFloat();
  g.setColour(glass);
  g.fillRect(scope);
  g.setColour(blue.withAlpha(0.35f));
  g.fillRect(scope.withSizeKeepingCentre(scope.getWidth() - 2.0f, 1.0f));
  for (int i = 1; i < 6; ++i)
    g.fillRect(juce::Rectangle<float>(scope.getX() + scope.getWidth() * (float)i / 6.0f,
                                      scope.getY() + 1.0f, 1.0f, scope.getHeight() - 2.0f));
  g.setColour(blue.withAlpha(0.8f));
  g.drawRect(scope, 1.0f);
  drawArt(g, scope.reduced(8.0f, 4.0f), spec.art, on ? light : light.withAlpha(0.45f),
          on ? 0.8f : 0.0f);
}

void Pedal::paint(juce::Graphics &g) {
  card.draw(g, getLocalBounds(), isOn() ? 1 : 0,
            [this](juce::Graphics &c) { paintCard(c); });
  drawLed(g, led, isOn(), juce::Colour(spec.neon));
}

void Pedal::resized() {
  auto b = getLocalBounds().reduced(7, 0);
  b.removeFromTop(11);
  auto knobRow = b.removeFromTop(72);
  const int column = knobRow.getWidth() / (int)knobs.size();
  for (auto &knob : knobs) {
    knob->setMaxDiameter(column);
    knob->setBounds(knobRow.removeFromLeft(column));
  }
  b.removeFromTop(5);
  nameArea = b.removeFromTop(24).reduced(2, 0);
  b.removeFromTop(6);
  artArea = b.removeFromTop(30).reduced(2, 0);
  b.removeFromTop(7);
  led = juce::Rectangle<float>(8.0f, 8.0f).withCentre(
      {(float)b.getCentreX(), (float)b.getY() + 4.0f});
  b.removeFromTop(10);
  b.removeFromBottom(4);
  footswitch.setBounds(b.withSizeKeepingCentre(56, juce::jmin(56, b.getHeight())));
}

PedalBoard::PedalBoard(juce::AudioProcessorValueTreeState &state) {
  for (size_t i = 0; i < pedals.size(); ++i) {
    pedals[i] = std::make_unique<Pedal>(state, pedalSpecs()[i]);
    addAndMakeVisible(*pedals[i]);
  }
}

int PedalBoard::getNumActive() const noexcept {
  int n = 0;
  for (auto &p : pedals)
    n += p->isOn() ? 1 : 0;
  return n;
}

void PedalBoard::resized() {
  auto b = getLocalBounds();
  const int rowHeight = (b.getHeight() - rowGap) / 2 - captionHeight;
  const int width = (b.getWidth() - (int)(perRow - 1) * gap) / (int)perRow;
  for (size_t row = 0; row < 2; ++row) {
    if (row == 1)
      b.removeFromTop(rowGap);
    auto caption = b.removeFromTop(captionHeight);
    if (row == 1)
      ampSlot = caption.removeFromLeft(ampBlockWidth).withTrimmedBottom(5);
    auto pedalRow = b.removeFromTop(rowHeight);
    for (size_t i = 0; i < perRow; ++i) {
      pedals[row * perRow + i]->setBounds(pedalRow.removeFromLeft(width));
      pedalRow.removeFromLeft(gap);
    }
  }
}

void PedalBoard::paint(juce::Graphics &g) {
  board.draw(g, getLocalBounds(), 0, [this](juce::Graphics &c) {
    using namespace colours;
    // Row captions; the amp and cab sit between the two rows.
    const auto captionRow = [&c](juce::Rectangle<int> area, const char *title) {
      const auto font = caption();
      c.setFont(font);
      c.setColour(textSoft);
      const auto row = area.toFloat();
      c.drawText(title, row, juce::Justification::centredLeft, false);
      // Chevrons after it, pointing the way the signal goes.
      const float x = row.getX() + juce::GlyphArrangement::getStringWidth(font, title) + 10.0f;
      juce::Path chevrons;
      for (int i = 0; i < 3; ++i) {
        const float at = x + 7.0f * (float)i;
        chevrons.startNewSubPath(at, row.getCentreY() - 3.5f);
        chevrons.lineTo(at + 3.5f, row.getCentreY());
        chevrons.lineTo(at, row.getCentreY() + 3.5f);
      }
      c.setColour(cyan.withAlpha(0.7f));
      c.strokePath(chevrons, juce::PathStrokeType(1.2f));
    };
    captionRow(juce::Rectangle<int>(0, 0, getWidth(), captionHeight - 5), "INTO THE AMP");
    captionRow(ampSlot.withX(ampSlot.getRight() + 10).withRight(getWidth()), "AFTER THE CAB");

    // A wire from each pedal to the next, with the signal's direction on it.
    for (size_t i = 0; i + 1 < pedals.size(); ++i) {
      if ((i + 1) % perRow == 0)
        continue;
      const auto left = pedals[i]->getBounds().toFloat();
      const float y = left.getCentreY() - 4.0f;
      juce::Path wire;
      wire.startNewSubPath(left.getRight() - 4.0f, y);
      wire.lineTo(left.getRight() + (float)gap + 3.0f, y);
      strokeNeon(c, wire, cyan.withAlpha(0.75f), 1.0f, 0.5f);
      const float tip = left.getRight() + (float)gap * 0.5f + 3.0f;
      juce::Path arrow;
      arrow.addTriangle(tip - 5.0f, y - 3.5f, tip - 5.0f, y + 3.5f, tip, y);
      c.setColour(cyan);
      c.fillPath(arrow);
    }

    // The amp, as a striped plate heading the second row.
    const auto amp = ampSlot.toFloat();
    const auto plate = bevelBox(amp, 5.0f);
    c.setColour(glass);
    c.fillPath(plate);
    {
      const juce::Graphics::ScopedSaveState save(c);
      c.reduceClipRegion(plate);
      drawStripes(c, amp, blue.withAlpha(0.7f), 8.0f, 3.0f);
    }
    strokeNeon(c, plate, cyan, 1.0f, 0.5f);
    drawAberrated(c, "AMP", caps(10.5f, 0.2f), amp, juce::Justification::centred,
                  colours::text, 1.0f);
  });
}
} // namespace headroom::gui
