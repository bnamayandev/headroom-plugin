#include "Widgets.h"

#include "HeadroomLookAndFeel.h"

namespace headroom::gui {
void ignoreTextWithoutNumber(juce::Slider &slider) {
  slider.valueFromTextFunction = [&slider, parse = slider.valueFromTextFunction](
                                     const juce::String &text) {
    return text.containsAnyOf("0123456789") ? parse(text) : slider.getValue();
  };
}

Knob::Knob(juce::AudioProcessorValueTreeState &state,
           const juce::String &parameterId, const juce::String &titleText,
           Size sizeToUse)
    : slider(juce::Slider::RotaryHorizontalVerticalDrag,
             juce::Slider::TextBoxBelow),
      attachment(state, parameterId, slider), size(sizeToUse) {
  title.setText(titleText.toUpperCase(), juce::dontSendNotification);
  // Three to a pedal leaves no room for a wide face: those print as a terminal.
  title.setFont(size == Size::pedal ? readout(15.0f) : caps(10.5f, 0.18f));
  title.setColour(juce::Label::textColourId, colours::text);
  title.setJustificationType(juce::Justification::centred);
  title.setInterceptsMouseClicks(false, false);
  addAndMakeVisible(title);

  if (size == Size::pedal)
    setCompact(slider);
  slider.setRotaryParameters(juce::degreesToRadians(225.0f),
                             juce::degreesToRadians(495.0f), true);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80,
                         size == Size::pedal ? 14 : 18);
  slider.setColour(juce::Slider::textBoxOutlineColourId,
                   juce::Colours::transparentBlack);
  slider.setPopupDisplayEnabled(false, false, nullptr);
  if (auto *p = state.getParameter(parameterId))
    slider.setDoubleClickReturnValue(true,
                                     p->convertFrom0to1(p->getDefaultValue()));
  slider.setTooltip(titleText + " (double-click to reset)");
  ignoreTextWithoutNumber(slider);
  addAndMakeVisible(slider);
}

void Knob::setColours(juce::Colour ink, juce::Colour lit) {
  title.setColour(juce::Label::textColourId, ink);
  slider.setColour(juce::Slider::textBoxTextColourId, ink);
  slider.setColour(juce::Slider::rotarySliderFillColourId, lit);
}

void Knob::resized() {
  const bool pedal = size == Size::pedal;
  auto b = getLocalBounds();
  title.setBounds(b.removeFromTop(pedal ? 13 : 16));
  // Keep knobs a consistent size however big their panel is.
  const int textHeight = pedal ? 14 : 20;
  const int diameter =
      juce::jmin(b.getWidth(), b.getHeight() - textHeight, maxDiameter);
  // No wider than this component, which would cut off the sides of the value box.
  slider.setBounds(b.withSizeKeepingCentre(
      juce::jmin(b.getWidth(), juce::jmax(diameter, pedal ? 46 : 84)),
      diameter + textHeight));
}

Fader::Fader(juce::AudioProcessorValueTreeState &state,
             const juce::String &parameterId, const juce::String &titleText)
    : attachment(state, parameterId, slider) {
  for (auto *l : {&value, &title}) {
    l->setJustificationType(juce::Justification::centred);
    l->setInterceptsMouseClicks(false, false);
    addAndMakeVisible(l);
  }
  title.setText(titleText.toUpperCase(), juce::dontSendNotification);
  title.setFont(readout(17.0f));
  title.setColour(juce::Label::textColourId, colours::textSoft);
  value.setFont(readout(16.0f));

  // Compact, to fit a narrow column: "+3.5", "0.0", "-10.0".
  slider.onValueChange = [this] {
    const auto v = slider.getValue();
    const bool flat = std::abs(v) < 0.05;
    value.setText((v >= 0.05 ? "+" : "") + juce::String(flat ? 0.0 : v, 1),
                  juce::dontSendNotification);
    value.setColour(juce::Label::textColourId,
                    flat ? colours::muted : colours::accent());
  };
  slider.onValueChange();
  slider.setPopupDisplayEnabled(false, false, nullptr);
  if (auto *p = state.getParameter(parameterId)) {
    slider.setDoubleClickReturnValue(true,
                                     p->convertFrom0to1(p->getDefaultValue()));
    slider.setTooltip(p->getName(64) + " (double-click to reset)");
  }
  addAndMakeVisible(slider);
}

void Fader::resized() {
  auto b = getLocalBounds();
  value.setBounds(b.removeFromTop(16));
  title.setBounds(b.removeFromBottom(16));
  slider.setBounds(b.reduced(0, 2));
}

void LevelMeter::pushPeak(float peakGain, float secondsSinceLast) {
  const float db = juce::Decibels::gainToDecibels(peakGain, -100.0f);
  const float fallDb = 30.0f * secondsSinceLast; // 30 dB/s release
  levelDb = db > levelDb ? db : juce::jmax(db, levelDb - fallDb);
  if (peakGain >= 1.0f)
    clipped = true;
  repaint();
}

void LevelMeter::paint(juce::Graphics &g) {
  using namespace colours;
  constexpr float minDb = -60.0f, maxDb = 6.0f, hotDb = -6.0f, warmDb = -18.0f;
  constexpr float pitch = 4.0f, segment = 2.0f; // one bar every 4 px
  auto bounds = getLocalBounds().toFloat();
  auto clipLed = bounds.removeFromTop(6.0f);
  bounds.removeFromTop(4.0f);

  drawLed(g, clipLed.withSizeKeepingCentre(6.0f, 6.0f), clipped, magenta);

  g.setColour(glass);
  g.fillRect(bounds);
  g.setColour(blue.withAlpha(0.8f));
  g.drawRect(bounds, 1.0f);
  const auto ladder = bounds.reduced(2.0f);
  const int count = (int)(ladder.getHeight() / pitch);
  for (int i = 0; i < count; ++i) {
    const float top = ladder.getBottom() - pitch * (float)(i + 1) + (pitch - segment);
    const float db = juce::jmap((float)i + 0.5f, 0.0f, (float)count, minDb, maxDb);
    const auto bar = juce::Rectangle<float>(ladder.getX(), top, ladder.getWidth(), segment);
    const auto colour = db >= hotDb ? magenta : db >= warmDb ? yellow : cyan;
    g.setColour(db <= levelDb ? colour : colour.withAlpha(0.1f));
    g.fillRect(bar);
  }
}

Spectrum::Spectrum(double low, double high) : lowHz(low), highHz(high) {
  levelsDb.fill(floorDb);
  setInterceptsMouseClicks(false, false);
}

void Spectrum::push(const float *data, int n) noexcept {
  for (int i = 0; i < n; ++i) {
    history[(size_t)historyWrite] = std::isfinite(data[i]) ? data[i] : 0.0f;
    historyWrite = (historyWrite + 1) & (fftSize - 1);
  }
  fresh = fresh || n > 0;
}

void Spectrum::refresh(double sampleRate, float secondsSinceLast) {
  const float fall = 48.0f * secondsSinceLast; // dB per refresh
  if (!fresh || sampleRate <= 0.0) {
    // Nothing playing: let the curve sink away rather than freeze.
    bool moving = false;
    for (auto &l : levelsDb) {
      moving = moving || l > floorDb;
      l = juce::jmax(floorDb, l - fall);
    }
    if (moving)
      repaint();
    return;
  }
  fresh = false;

  // Oldest sample first, windowed.
  const auto split = (size_t)historyWrite;
  std::copy(history.begin() + (long)split, history.end(), fftData.begin());
  std::copy(history.begin(), history.begin() + (long)split,
            fftData.begin() + (long)(fftSize - split));
  window.multiplyWithWindowingTable(fftData.data(), (size_t)fftSize);
  fft.performFrequencyOnlyForwardTransform(fftData.data(), true);

  // A full-scale sine reads 0 dB (Hann halves the peak bin), tilted +3 dB per
  // octave about 1 kHz so a guitar's falling top end still reads clearly.
  const double binHz = sampleRate / fftSize;
  const int lastBin = fftSize / 2;
  const auto binDb = [&](double bin) {
    const auto i = juce::jlimit(0, lastBin - 1, (int)bin);
    const auto t = (float)juce::jlimit(0.0, 1.0, bin - i);
    const float mag = fftData[(size_t)i] + t * (fftData[(size_t)i + 1] -
                                                fftData[(size_t)i]);
    return juce::Decibels::gainToDecibels(mag * 4.0f / fftSize, -200.0f);
  };
  const double octaves = std::log2(highHz / lowHz);
  const double halfStep = 0.5 * octaves / (numPoints - 1);
  for (int p = 0; p < numPoints; ++p) {
    const double octave = octaves * p / (numPoints - 1);
    const double hz = lowHz * std::exp2(octave);
    const double from = lowHz * std::exp2(octave - halfStep) / binHz;
    const double to = lowHz * std::exp2(octave + halfStep) / binHz;
    float db = binDb(hz / binHz);
    for (int b = (int)std::ceil(from); b <= (int)to && b <= lastBin; ++b)
      db = juce::jmax(db, binDb(b)); // wide at the top: the loudest bin
    db += 3.0f * (float)std::log2(hz / 1000.0);

    auto &l = levelsDb[(size_t)p];
    l = db > l ? l + 0.6f * (db - l) : juce::jmax(db, l - fall);
    l = juce::jlimit(floorDb, topDb, l);
  }
  repaint();
}

void Spectrum::paint(juce::Graphics &g) {
  using namespace colours;
  const auto bounds = getLocalBounds().toFloat();
  // The EQ's scale runs between where a fader's cap can reach.
  constexpr float trackInset = 12.0f;

  graticule.draw(g, getLocalBounds(), juce::roundToInt(scaleDb * 10.0),
                 [this, bounds](juce::Graphics &c) {
    drawScreen(c, bounds);
    const auto columns = (float)juce::jmax(1, (int)bands.size());
    c.setColour(blue.withAlpha(0.3f));
    for (float i = 1.0f; i < columns; i += 1.0f)
      c.fillRect(bounds.getX() + std::round(bounds.getWidth() * i / columns),
                 bounds.getY() + 1.0f, 1.0f, bounds.getHeight() - 2.0f);
    const auto scale = bounds.reduced(0.0f, trackInset);
    c.setFont(readout(14.0f));
    for (int step = -2; step <= 2; ++step) {
      const float y = std::round(scale.getY() + scale.getHeight() * ((float)step + 2.0f) * 0.25f);
      c.setColour(blue.withAlpha(step == 0 ? 0.75f : 0.3f));
      c.fillRect(bounds.getX() + 1.0f, y, bounds.getWidth() - 2.0f, 1.0f);
      if (step % 2 == 0) {
        const int db = juce::roundToInt(scaleDb * (float)-step * 0.5f);
        c.setColour(muted);
        c.drawText((db > 0 ? "+" : "") + juce::String(db),
                   juce::Rectangle<float>(bounds.getX() + 6.0f, y - 13.0f, 30.0f, 12.0f),
                   juce::Justification::centredLeft, false);
      }
    }
  });

  const auto inner = bounds.reduced(1.0f);
  const bool hearing = std::any_of(levelsDb.begin(), levelsDb.end(),
                                   [](float level) { return level > floorDb + 0.5f; });
  const juce::Graphics::ScopedSaveState save(g);
  g.reduceClipRegion(inner.getSmallestIntegerContainer());

  const auto point = [&](int p) {
    return juce::Point<float>(
        juce::jmap((float)p, 0.0f, (float)(numPoints - 1), inner.getX(),
                   inner.getRight()),
        juce::jmap(levelsDb[(size_t)p], floorDb, topDb, inner.getBottom(),
                   inner.getY()));
  };

  if (hearing) {
    // Smooth: quadratic segments through the midpoints between points.
    juce::Path curve;
    curve.startNewSubPath(point(0));
    for (int p = 1; p < numPoints - 1; ++p) {
      const auto here = point(p), next = point(p + 1);
      curve.quadraticTo(here, (here + next) * 0.5f);
    }
    curve.lineTo(point(numPoints - 1));

    auto fill = curve;
    fill.lineTo(bounds.getBottomRight());
    fill.lineTo(bounds.getBottomLeft());
    fill.closeSubPath();
    g.setGradientFill(juce::ColourGradient(accent().withAlpha(0.34f), 0.0f, inner.getY(),
                                           accent().withAlpha(0.02f), 0.0f, inner.getBottom(),
                                           false));
    g.fillPath(fill);
    strokeNeon(g, curve, accent(), 1.2f, 0.8f);
  }

  // The EQ's shape, through the fader caps and out flat past the ends: the
  // data colour, unless that's the neon the spectrum is in.
  if (!bands.empty()) {
    const auto scale = bounds.reduced(0.0f, trackInset);
    const auto bandPoint = [&](size_t i) {
      return juce::Point<float>(
          bounds.getX() + bounds.getWidth() * ((float)i + 0.5f) / (float)bands.size(),
          scale.getBottom() - scale.getHeight() * bands[i]);
    };
    juce::Path shape;
    shape.startNewSubPath(bounds.getX(), bandPoint(0).y);
    shape.lineTo(bandPoint(0));
    for (size_t i = 1; i + 1 < bands.size(); ++i) {
      const auto here = bandPoint(i), next = bandPoint(i + 1);
      shape.quadraticTo(here, (here + next) * 0.5f);
    }
    shape.lineTo(bandPoint(bands.size() - 1));
    shape.lineTo(bounds.getRight(), bandPoint(bands.size() - 1).y);
    const auto ink = getNeon() == Neon::yellow ? text : yellow;
    strokeNeon(g, shape, ink.withAlpha(0.8f), 1.1f, 0.5f);
  }
}

void Footswitch::setLight(juce::Colour colour) {
  light = colour;
  repaint();
}

void Footswitch::paintButton(juce::Graphics &g, bool highlighted, bool down) {
  using namespace colours;
  constexpr float pi = juce::MathConstants<float>::pi;
  const bool on = getToggleState();
  const auto size = (float)juce::jmin(getWidth(), getHeight());
  const auto centre = getLocalBounds().toFloat().getCentre();
  const float nut = size * 0.43f;

  // The nut: a hexagon, its frame splitting into colours under the pointer.
  juce::Path hex;
  hex.addPolygon(centre, 6, nut, pi / 6.0f);
  g.setColour(glass);
  g.fillPath(hex);
  if (highlighted) {
    g.setColour(magenta.withAlpha(0.55f));
    g.strokePath(hex, juce::PathStrokeType(1.0f), juce::AffineTransform::translation(-1.2f, 0.0f));
    g.setColour(cyan.withAlpha(0.55f));
    g.strokePath(hex, juce::PathStrokeType(1.0f), juce::AffineTransform::translation(1.2f, 0.0f));
  }
  strokeNeon(g, hex, light.withAlpha(on || highlighted ? 1.0f : 0.5f), 1.2f, on ? 0.8f : 0.0f);

  // The switch, sitting lower while it's held: lit solid while the pedal's on.
  juce::Path stomp;
  stomp.addPolygon(centre, 8, nut * (down ? 0.5f : 0.58f), pi / 8.0f);
  if (on) {
    fillNeon(g, stomp, light, 1.0f);
    return;
  }
  g.setColour(light.withAlpha(highlighted ? 0.2f : 0.1f));
  g.fillPath(stomp);
  g.setColour(light.withAlpha(0.6f));
  g.strokePath(stomp, juce::PathStrokeType(1.0f));
}

/** Selectable text that sizes to fit: a read-only editor with its chrome off. */
void styleAsText(juce::TextEditor &editor, const juce::Font &font, juce::Colour colour) {
  editor.setMultiLine(true, true);
  editor.setReadOnly(true);
  editor.setCaretVisible(false);
  editor.setScrollbarsShown(false);
  editor.setBorder(juce::BorderSize<int>(0));
  editor.setIndents(0, 0);
  for (const auto id : {juce::TextEditor::backgroundColourId,
                        juce::TextEditor::outlineColourId,
                        juce::TextEditor::focusedOutlineColourId,
                        juce::TextEditor::shadowColourId})
    editor.setColour(id, juce::Colours::transparentBlack);
  editor.setColour(juce::TextEditor::textColourId, colour);
  editor.setFont(font);
}

void styleLink(juce::HyperlinkButton &link, float size) {
  link.setFont(body(size), false, juce::Justification::centredLeft);
  link.setColour(juce::HyperlinkButton::textColourId, colours::accent());
}

void styleSecret(juce::TextEditor &editor) {
  editor.setPasswordCharacter((juce::juce_wchar)0x2022);
  editor.setFont(readout(19.0f));
  editor.setJustification(juce::Justification::centredLeft);
  editor.setIndents(8, 0);
  editor.setSelectAllWhenFocused(true);
}
} // namespace headroom::gui
