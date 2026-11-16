#include "HeadroomLookAndFeel.h"

#include <HeadroomBinaryData.h>

namespace headroom::gui {
namespace colours {
namespace {
Neon chosen = defaultNeon;
} // namespace

juce::Colour neon(Neon which) {
  switch (which) {
  case Neon::yellow:
    return yellow;
  case Neon::cyan:
    return cyan;
  case Neon::magenta:
    break;
  }
  return magenta;
}
juce::Colour accent() { return neon(chosen); }
Neon getNeon() { return chosen; }
void setNeon(Neon which) { chosen = which; }
} // namespace colours

namespace {
juce::Typeface::Ptr load(const char *data, int size) {
  return juce::Typeface::createSystemTypefaceFor(data, (size_t)size);
}

juce::Font makeFont(const juce::Typeface::Ptr &face, float size, float tracking) {
  return juce::Font(juce::FontOptions(face).withPointHeight(size).withKerningFactor(
      tracking));
}

/** Accent at a given strength over transparent, like CSS color-mix(). */
juce::Colour tint(float amount) { return colours::accent().withAlpha(amount); }

bool isPrimary(const juce::Button &button) {
  return button.getProperties()["primary"];
}

bool isTab(const juce::Button &button) {
  return button.getProperties()["tab"];
}

bool isProse(const juce::Button &button) {
  return button.getProperties()["prose"];
}

bool isBipolar(const juce::Slider &slider) {
  return slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0 &&
         std::abs(slider.getMinimum() + slider.getMaximum()) < 1.0e-6;
}

juce::TextLayout layoutTooltip(const juce::String &text, juce::Colour colour) {
  juce::AttributedString s;
  s.setJustification(juce::Justification::centredLeft);
  s.append(text, body(12.0f), colour);
  juce::TextLayout layout;
  layout.createLayoutWithBalancedLineLengths(s, 360.0f);
  return layout;
}

/** Fills a path twice more, a pixel or so either side, in magenta and cyan: a
    misconverged tube's fringes, for whatever the pointer is over. */
void strokeFringes(juce::Graphics &g, const juce::Path &path, float width, float offset) {
  g.setColour(colours::magenta.withAlpha(0.55f));
  g.strokePath(path, juce::PathStrokeType(width),
               juce::AffineTransform::translation(-offset, 0.0f));
  g.setColour(colours::cyan.withAlpha(0.55f));
  g.strokePath(path, juce::PathStrokeType(width),
               juce::AffineTransform::translation(offset, 0.0f));
}

/** A vertical fader: a thin slot, the value lit along it from its centre (or
    bottom), and a cap riding in it with pointers either side, like a cursor. */
void drawFader(juce::Graphics &g, juce::Rectangle<float> bounds, float sliderPos,
               juce::Slider &slider) {
  using namespace colours;
  const bool enabled = slider.isEnabled();
  const bool hot = enabled && slider.isMouseOverOrDragging();
  const float x = std::floor(bounds.getCentreX()) + 0.5f;
  const auto top = (float)slider.getPositionOfValue(slider.getMaximum());
  const auto bottom = (float)slider.getPositionOfValue(slider.getMinimum());

  g.setColour(blue.withAlpha(0.9f));
  g.fillRect(juce::Rectangle<float>(x - 0.5f, top - 3.0f, 1.0f, bottom - top + 6.0f));

  const auto from = isBipolar(slider) ? (top + bottom) * 0.5f : bottom;
  if (std::abs(sliderPos - from) > 0.5f) {
    juce::Path lit;
    lit.startNewSubPath(x, from);
    lit.lineTo(x, sliderPos);
    strokeNeon(g, lit, enabled ? accent() : mutedSoft, 2.0f,
               enabled ? (hot ? 1.0f : 0.6f) : 0.0f);
  }

  const auto cap = juce::Rectangle<float>(24.0f, 11.0f).withCentre({x, std::round(sliderPos)});
  const auto edge = !enabled ? mutedSoft : hot ? accent() : cyan;
  g.setColour(glass);
  g.fillRect(cap);
  juce::Path frame;
  frame.addRectangle(cap);
  if (hot)
    strokeFringes(g, frame, 1.0f, 1.2f);
  strokeNeon(g, frame, edge, 1.0f, hot ? 0.7f : 0.0f);
  g.setColour(enabled ? colours::text : muted);
  g.fillRect(cap.withSizeKeepingCentre(cap.getWidth() - 8.0f, 1.5f));

  juce::Path pointers;
  const float y = cap.getCentreY();
  pointers.addTriangle(cap.getX() - 6.0f, y - 3.0f, cap.getX() - 6.0f, y + 3.0f,
                       cap.getX() - 2.0f, y);
  pointers.addTriangle(cap.getRight() + 6.0f, y - 3.0f, cap.getRight() + 6.0f, y + 3.0f,
                       cap.getRight() + 2.0f, y);
  g.setColour(edge);
  g.fillPath(pointers);
}

/** The symbols on small keys, as strokes rather than glyphs so they stay crisp
    and centred. False if this text isn't one of them. */
bool drawSymbol(juce::Graphics &g, const juce::String &text, juce::Rectangle<float> b) {
  const auto c = b.getCentre();
  const float s = 4.0f;
  juce::Path path;
  if (text == "<") {
    path.startNewSubPath(c.x + s * 0.5f, c.y - s);
    path.lineTo(c.x - s * 0.5f, c.y);
    path.lineTo(c.x + s * 0.5f, c.y + s);
  } else if (text == ">") {
    path.startNewSubPath(c.x - s * 0.5f, c.y - s);
    path.lineTo(c.x + s * 0.5f, c.y);
    path.lineTo(c.x - s * 0.5f, c.y + s);
  } else if (text == "-") {
    path.startNewSubPath(c.x - s, c.y);
    path.lineTo(c.x + s, c.y);
  } else if (text == "+") {
    path.startNewSubPath(c.x - s, c.y);
    path.lineTo(c.x + s, c.y);
    path.startNewSubPath(c.x, c.y - s);
    path.lineTo(c.x, c.y + s);
  } else if (text == juce::String::charToString(0x00d7)) {
    path.startNewSubPath(c.x - s * 0.8f, c.y - s * 0.8f);
    path.lineTo(c.x + s * 0.8f, c.y + s * 0.8f);
    path.startNewSubPath(c.x + s * 0.8f, c.y - s * 0.8f);
    path.lineTo(c.x - s * 0.8f, c.y + s * 0.8f);
  } else {
    return false;
  }
  g.strokePath(path, juce::PathStrokeType(1.6f, juce::PathStrokeType::mitered,
                                          juce::PathStrokeType::square));
  return true;
}
} // namespace

Typefaces::Typefaces()
    : headline(load(HeadroomBinaryData::MichromaRegular_ttf,
                    HeadroomBinaryData::MichromaRegular_ttfSize)),
      caps(load(HeadroomBinaryData::OrbitronBold_ttf,
                HeadroomBinaryData::OrbitronBold_ttfSize)),
      logo(load(HeadroomBinaryData::OrbitronBlack_ttf,
                HeadroomBinaryData::OrbitronBlack_ttfSize)),
      readout(load(HeadroomBinaryData::VT323Regular_ttf,
                   HeadroomBinaryData::VT323Regular_ttfSize)),
      body(load(HeadroomBinaryData::IBMPlexMonoRegular_ttf,
                HeadroomBinaryData::IBMPlexMonoRegular_ttfSize)) {}

juce::Font headline(float size, float tracking) {
  const juce::SharedResourcePointer<Typefaces> faces;
  return makeFont(faces->headline, size, tracking);
}

juce::Font caps(float size, float tracking) {
  const juce::SharedResourcePointer<Typefaces> faces;
  return makeFont(faces->caps, size, tracking);
}

juce::Font logoFont(float size) {
  const juce::SharedResourcePointer<Typefaces> faces;
  return makeFont(faces->logo, size, 0.0f);
}

juce::Font readout(float size, float tracking) {
  const juce::SharedResourcePointer<Typefaces> faces;
  return makeFont(faces->readout, size, tracking);
}

juce::Font body(float size) {
  const juce::SharedResourcePointer<Typefaces> faces;
  return makeFont(faces->body, size, 0.0f);
}

void setPrimary(juce::Button &button) {
  button.getProperties().set("primary", true);
}

void setTab(juce::Button &button) {
  button.getProperties().set("tab", true);
}

void setProse(juce::Button &button) {
  button.getProperties().set("prose", true);
}

void setCompact(juce::Slider &slider) {
  slider.getProperties().set("compact", true);
}

void setHeadline(juce::Label &label) {
  label.getProperties().set("headline", true);
}

HeadroomLookAndFeel::HeadroomLookAndFeel() {
  using namespace colours;
  const auto clear = juce::Colours::transparentBlack;
  setDefaultSansSerifTypeface(typefaces->body);

  setColour(juce::ResizableWindow::backgroundColourId, background);
  setColour(juce::Label::textColourId, text);
  setColour(juce::Slider::textBoxTextColourId, textSoft);
  setColour(juce::Slider::textBoxOutlineColourId, clear);
  setColour(juce::Slider::textBoxBackgroundColourId, clear);
  setColour(juce::Slider::rotarySliderOutlineColourId, blue.withAlpha(0.9f));
  setColour(juce::TextButton::buttonColourId, clear);
  setColour(juce::TextButton::textColourOffId, textSoft);
  setColour(juce::TextButton::textColourOnId, black);
  setColour(juce::ComboBox::backgroundColourId, clear);
  setColour(juce::ComboBox::outlineColourId, line);
  setColour(juce::ComboBox::textColourId, text);
  setColour(juce::ComboBox::arrowColourId, textSoft);
  setColour(juce::PopupMenu::backgroundColourId, black);
  setColour(juce::PopupMenu::textColourId, textSoft);
  setColour(juce::PopupMenu::headerTextColourId, muted);
  setColour(juce::PopupMenu::highlightedTextColourId, text);
  setColour(juce::PopupMenu::highlightedBackgroundColourId, blue.withAlpha(0.45f));
  setColour(juce::ToggleButton::textColourId, textSoft);
  setColour(juce::TextEditor::backgroundColourId, glass);
  setColour(juce::TextEditor::textColourId, text);
  setColour(juce::TextEditor::highlightedTextColourId, text);
  setColour(juce::TextEditor::outlineColourId, line);
  setColour(juce::AlertWindow::backgroundColourId, panel);
  setColour(juce::AlertWindow::textColourId, textSoft);
  setColour(juce::AlertWindow::outlineColourId, line);
  setColour(juce::TooltipWindow::backgroundColourId, black);
  setColour(juce::TooltipWindow::textColourId, text);
  setColour(juce::TooltipWindow::outlineColourId, line);
  setColour(juce::ScrollBar::thumbColourId, cyan.withAlpha(0.5f));
  setColour(juce::ScrollBar::trackColourId, clear);
  refreshAccent();
}

void HeadroomLookAndFeel::refreshAccent() {
  setColour(juce::Slider::textBoxHighlightColourId, tint(0.35f));
  setColour(juce::TextButton::buttonOnColourId, tint(0.18f));
  setColour(juce::TextEditor::highlightColourId, tint(0.35f));
  setColour(juce::TextEditor::focusedOutlineColourId, colours::accent());
  setColour(juce::CaretComponent::caretColourId, colours::accent());
  setColour(juce::HyperlinkButton::textColourId, colours::accent());
}

void HeadroomLookAndFeel::drawRotarySlider(juce::Graphics &g, int x, int y,
                                           int width, int height, float sliderPos,
                                           float startAngle, float endAngle,
                                           juce::Slider &slider) {
  using namespace colours;
  const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
  const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
  const auto centre = bounds.getCentre();
  const auto angle = startAngle + sliderPos * (endAngle - startAngle);
  const bool enabled = slider.isEnabled();
  const bool hot = enabled && slider.isMouseOverOrDragging();
  const bool small = radius < 30.0f;
  const auto lit = !enabled ? mutedSoft
                   : slider.isColourSpecified(juce::Slider::rotarySliderFillColourId)
                       ? slider.findColour(juce::Slider::rotarySliderFillColourId)
                       : accent();

  // The scale: a ring of ticks, lit up to the value, from the centre for
  // bipolar parameters (e.g. EQ, +/- dB).
  const float outer = radius - 2.0f, inner = outer - (small ? 3.5f : 6.0f);
  const int marks = small ? 15 : 27;
  const auto from = isBipolar(slider) ? (startAngle + endAngle) * 0.5f : startAngle;
  constexpr float nudge = 0.002f;
  const auto lower = juce::jmin(from, angle) - nudge, upper = juce::jmax(from, angle) + nudge;
  juce::Path unlitTicks, litTicks;
  for (int i = 0; i < marks; ++i) {
    const float a = startAngle + (endAngle - startAngle) * (float)i / (float)(marks - 1);
    auto &path = a >= lower && a <= upper ? litTicks : unlitTicks;
    path.startNewSubPath(centre.getPointOnCircumference(inner, a));
    path.lineTo(centre.getPointOnCircumference(outer, a));
  }
  const float tickWidth = small ? 1.4f : 1.8f;
  g.setColour(slider.findColour(juce::Slider::rotarySliderOutlineColourId));
  g.strokePath(unlitTicks, juce::PathStrokeType(tickWidth));
  strokeNeon(g, litTicks, lit, tickWidth, enabled ? (hot ? 1.0f : 0.6f) : 0.0f);

  // The knob: an octagon turned with the value, a smaller one inside it.
  const float face = inner - (small ? 3.0f : 5.0f);
  juce::Path rim, core;
  rim.addPolygon(centre, 8, face, angle);
  core.addPolygon(centre, 8, face * 0.55f, angle);
  g.setColour(glass);
  g.fillPath(rim);
  g.setColour(blue.withAlpha(enabled ? 0.9f : 0.45f));
  g.strokePath(core, juce::PathStrokeType(1.0f));
  if (hot)
    strokeFringes(g, rim, 1.0f, 1.2f);
  strokeNeon(g, rim, !enabled ? mutedSoft : cyan.withAlpha(hot ? 1.0f : 0.72f),
             small ? 1.0f : 1.2f, hot ? 0.8f : 0.25f);

  // The pointer, from near the middle out to the corner it turns with.
  juce::Path pointer;
  pointer.startNewSubPath(centre.getPointOnCircumference(face * 0.22f, angle));
  pointer.lineTo(centre.getPointOnCircumference(face * 0.96f, angle));
  if (hot)
    strokeFringes(g, pointer, 2.0f, 1.5f);
  strokeNeon(g, pointer, enabled ? colours::text : muted, small ? 1.6f : 2.0f,
             enabled ? 0.7f : 0.0f);
}

void HeadroomLookAndFeel::drawLinearSlider(juce::Graphics &g, int x, int y,
                                           int width, int height, float sliderPos,
                                           float minSliderPos, float maxSliderPos,
                                           juce::Slider::SliderStyle style,
                                           juce::Slider &slider) {
  if (style == juce::Slider::LinearVertical) {
    drawFader(g, juce::Rectangle<int>(x, y, width, height).toFloat(), sliderPos, slider);
    return;
  }
  if (style != juce::Slider::LinearBar) {
    LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos,
                                     minSliderPos, maxSliderPos, style, slider);
    return;
  }
  // A readout you drag: a field, the value a striped bar from the left with a
  // lit leading edge.
  using namespace colours;
  const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
  const bool hot = slider.isEnabled() && slider.isMouseOverOrDragging();
  g.setColour(glass);
  g.fillRect(bounds);
  const auto fill = bounds.reduced(1.0f).withRight(
      juce::jlimit(bounds.getX() + 1.0f, bounds.getRight() - 1.0f, sliderPos));
  if (fill.getWidth() > 0.0f) {
    g.setColour(blue.withAlpha(hot ? 0.34f : 0.26f));
    g.fillRect(fill);
    drawStripes(g, fill, blue.withAlpha(hot ? 0.4f : 0.3f), 7.0f, 2.5f);
    g.setColour(accent().withAlpha(0.85f));
    g.fillRect(fill.withLeft(fill.getRight() - 1.5f));
  }
  g.setColour(cyan.withAlpha(hot ? 0.9f : 0.45f));
  g.drawRect(bounds, 1.0f);
}

void HeadroomLookAndFeel::drawToggleButton(juce::Graphics &g,
                                           juce::ToggleButton &button,
                                           bool highlighted, bool) {
  using namespace colours;
  auto bounds = button.getLocalBounds().toFloat();
  const bool on = button.getToggleState();
  const bool enabled = button.isEnabled();
  const bool hot = enabled && highlighted;

  // A slide switch: a block at the right of its track, lit, when it's on.
  const auto track = bounds.removeFromLeft(30.0f).withSizeKeepingCentre(30.0f, 16.0f);
  bounds.removeFromLeft(10.0f);
  g.setColour(glass);
  g.fillRect(track);
  const auto slot = track.reduced(3.0f);
  if (on) {
    juce::Path block;
    block.addRectangle(slot.withTrimmedLeft(slot.getWidth() * 0.5f));
    fillNeon(g, block, enabled ? accent() : mutedSoft, enabled ? 0.8f : 0.0f);
  } else {
    g.setColour((enabled ? cyan : mutedSoft).withAlpha(hot ? 0.6f : 0.32f));
    g.fillRect(slot.withWidth(slot.getWidth() * 0.5f));
  }
  g.setColour(!enabled ? mutedSoft : on ? accent() : cyan.withAlpha(hot ? 0.9f : 0.55f));
  g.drawRect(track, 1.0f);

  const auto label = button.getButtonText().toUpperCase();
  const auto font = caps(10.0f, 0.16f);
  const auto colour = !enabled ? mutedSoft : on || hot ? colours::text : textSoft;
  if (hot) {
    drawAberrated(g, label, font, bounds, juce::Justification::centredLeft, colour, 1.0f);
    return;
  }
  g.setColour(colour);
  g.setFont(font);
  g.drawFittedText(label, bounds.toNearestInt(), juce::Justification::centredLeft, 1);
}

void HeadroomLookAndFeel::drawButtonBackground(juce::Graphics &g,
                                               juce::Button &button,
                                               const juce::Colour &, bool highlighted,
                                               bool down) {
  using namespace colours;
  const auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
  const bool enabled = button.isEnabled();
  const bool hot = enabled && highlighted;
  const auto shape = bevelBox(bounds, juce::jmin(6.0f, bounds.getHeight() * 0.25f));

  if (isTab(button)) { // the selected tab is lit solid; the others are only words
    if (button.getToggleState())
      fillNeon(g, shape, cyan, 0.7f);
    else if (hot) {
      g.setColour(blue.withAlpha(0.35f));
      g.fillPath(shape);
    }
    return;
  }

  const auto colour = !enabled ? mutedSoft : isPrimary(button) ? accent() : cyan;
  if (button.getToggleState()) { // selected: lit solid
    fillNeon(g, shape, colour.withMultipliedAlpha(down ? 0.75f : 1.0f), hot ? 1.0f : 0.5f);
    return;
  }
  g.setColour(glass);
  g.fillPath(shape);
  if (hot || down) {
    g.setColour(colour.withAlpha(down ? 0.3f : 0.14f));
    g.fillPath(shape);
  }
  if (hot) // the frame splits into its colours, as on a misconverged tube
    strokeFringes(g, shape, 1.0f, 1.2f);
  strokeNeon(g, shape, colour.withMultipliedAlpha(hot ? 1.0f : enabled ? 0.72f : 0.6f), 1.0f,
             hot ? 0.6f : 0.0f);
}

void HeadroomLookAndFeel::drawButtonText(juce::Graphics &g,
                                         juce::TextButton &button, bool highlighted,
                                         bool down) {
  using namespace colours;
  const bool enabled = button.isEnabled();
  const bool on = button.getToggleState();
  const bool hot = enabled && highlighted && !on;
  const auto area = button.getLocalBounds().toFloat();

  if (isTab(button)) {
    const auto label = button.getButtonText().toUpperCase();
    const auto font = tabFont();
    if (hot) {
      drawAberrated(g, label, font, area, juce::Justification::centred, colours::text, 1.0f);
      return;
    }
    g.setColour(on ? black : textSoft);
    g.setFont(font);
    g.drawText(label, area, juce::Justification::centred, false);
    return;
  }

  const auto colour = !enabled ? mutedSoft
                      : on     ? black
                      : isPrimary(button) ? accent()
                      : hot               ? colours::text
                                          : textSoft;
  g.setColour(colour);
  if (drawSymbol(g, button.getButtonText(), area.translated(0.0f, down ? 1.0f : 0.0f)))
    return;
  if (isProse(button)) {
    g.setFont(body(12.5f));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(12, 2),
                     juce::Justification::centred, 1);
    return;
  }
  const auto font = getTextButtonFont(button, button.getHeight());
  const auto label = button.getButtonText().toUpperCase();
  const auto box = area.reduced(5.0f, 2.0f).translated(0.0f, down ? 1.0f : 0.0f);
  if (hot) {
    drawAberrated(g, textPath(label, font, box, juce::Justification::centred), colour, 1.0f);
    return;
  }
  g.setFont(font);
  g.drawFittedText(label, box.toNearestInt(), juce::Justification::centred, 1);
}

void HeadroomLookAndFeel::drawComboBox(juce::Graphics &g, int width, int height,
                                       bool isButtonDown, int, int, int, int,
                                       juce::ComboBox &box) {
  using namespace colours;
  const auto bounds = juce::Rectangle<int>(width, height).toFloat().reduced(0.5f);
  const bool enabled = box.isEnabled();
  const bool hot = enabled && box.isMouseOver(true);
  const auto shape = bevelBox(bounds, juce::jmin(6.0f, bounds.getHeight() * 0.25f));
  g.setColour(glass);
  g.fillPath(shape);
  if (hot) {
    g.setColour(cyan.withAlpha(0.06f));
    g.fillPath(shape);
  }
  g.setColour(box.hasKeyboardFocus(true) ? accent()
              : !enabled                 ? mutedSoft
                                         : cyan.withAlpha(hot ? 0.9f : 0.45f));
  g.strokePath(shape, juce::PathStrokeType(1.0f));

  // The arrow, in a cell of its own.
  const auto cell = juce::Rectangle<float>((float)width - 22.0f, 0.0f, 22.0f, (float)height);
  g.setColour(cyan.withAlpha(0.22f));
  g.fillRect(cell.withWidth(1.0f).reduced(0.0f, 6.0f));
  const auto c = cell.getCentre().translated(0.5f, 0.0f);
  juce::Path arrow;
  arrow.addTriangle(c.x - 4.0f, c.y - 2.0f, c.x + 4.0f, c.y - 2.0f, c.x, c.y + 2.5f);
  g.setColour(!enabled ? mutedSoft : hot || isButtonDown ? accent() : textSoft);
  g.fillPath(arrow);
}

void HeadroomLookAndFeel::drawLabel(juce::Graphics &g, juce::Label &label) {
  if (!(bool)label.getProperties()["headline"] || label.isBeingEdited()) {
    LookAndFeel_V4::drawLabel(g, label);
    return;
  }
  g.fillAll(label.findColour(juce::Label::backgroundColourId));
  const auto font = getLabelFont(label);
  const auto area = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds()).toFloat();
  const int lines = juce::jmax(1, (int)(area.getHeight() / font.getHeight()));
  const auto letters = textPath(label.getText(), font, area, label.getJustificationType(),
                                lines, label.getMinimumHorizontalScale());
  if (letters.isEmpty())
    return;
  const auto colour = label.findColour(juce::Label::textColourId)
                          .withMultipliedAlpha(label.isEnabled() ? 1.0f : 0.5f);
  juce::DropShadow(colour.withMultipliedAlpha(0.45f),
                   juce::jlimit(2, 12, juce::roundToInt(font.getHeight() * 0.3f)), {})
      .drawForPath(g, letters);
  drawAberrated(g, letters, colour, juce::jmax(1.0f, font.getHeight() * 0.06f));
}

void HeadroomLookAndFeel::drawPopupMenuBackground(juce::Graphics &g, int width,
                                                  int height) {
  using namespace colours;
  g.fillAll(black);
  g.setColour(cyan.withAlpha(0.65f));
  g.drawRect(0, 0, width, height, 1);
  g.setColour(accent());
  g.fillRect(1, 1, width - 2, 2); // a lit strip along its top
}

void HeadroomLookAndFeel::drawPopupMenuItem(
    juce::Graphics &g, const juce::Rectangle<int> &area, bool isSeparator, bool isActive,
    bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String &text,
    const juce::String &shortcutKeyText, const juce::Drawable *icon,
    const juce::Colour *textColour) {
  using namespace colours;
  if (isSeparator) {
    const auto r = area.reduced(10, 0);
    g.setColour(cyan.withAlpha(0.25f));
    g.fillRect(r.withY(r.getCentreY()).withHeight(1));
    return;
  }
  auto r = area.reduced(1);
  auto colour = textColour != nullptr ? *textColour : findColour(juce::PopupMenu::textColourId);
  if (isHighlighted && isActive) {
    g.setColour(findColour(juce::PopupMenu::highlightedBackgroundColourId));
    g.fillRect(r);
    g.setColour(accent());
    g.fillRect(r.withWidth(3));
    colour = findColour(juce::PopupMenu::highlightedTextColourId);
  } else if (!isActive) {
    colour = colour.withMultipliedAlpha(0.45f);
  }

  r.removeFromLeft(8);
  const auto mark = r.removeFromLeft(14).toFloat();
  if (icon != nullptr)
    icon->drawWithin(g, mark,
                     juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize,
                     1.0f);
  else if (isTicked) {
    g.setColour(accent());
    g.fillRect(juce::Rectangle<float>(6.0f, 6.0f).withCentre(mark.getCentre()));
  }
  r.removeFromRight(8);
  g.setColour(colour);
  if (hasSubMenu) {
    const auto arrow = r.removeFromRight(8).toFloat();
    juce::Path p;
    p.addTriangle(arrow.getX(), arrow.getCentreY() - 4.0f, arrow.getX(),
                  arrow.getCentreY() + 4.0f, arrow.getRight(), arrow.getCentreY());
    g.fillPath(p);
  }

  auto font = getPopupMenuFont();
  const float maxHeight = (float)r.getHeight() / 1.3f;
  if (font.getHeight() > maxHeight)
    font = font.withHeight(maxHeight);
  g.setFont(font);
  if (shortcutKeyText.isNotEmpty()) {
    g.setColour(colour.withMultipliedAlpha(0.6f));
    g.drawText(shortcutKeyText, r, juce::Justification::centredRight, true);
    g.setColour(colour);
  }
  g.drawFittedText(text, r, juce::Justification::centredLeft, 1);
}

void HeadroomLookAndFeel::drawCallOutBoxBackground(juce::CallOutBox &,
                                                   juce::Graphics &g,
                                                   const juce::Path &path,
                                                   juce::Image &) {
  juce::DropShadow(juce::Colours::black.withAlpha(0.85f), 16, {0, 4}).drawForPath(g, path);
  g.setColour(colours::panel);
  g.fillPath(path);
  strokeNeon(g, path, colours::cyan.withAlpha(0.85f), 1.0f, 0.6f);
}

void HeadroomLookAndFeel::drawAlertBox(juce::Graphics &g, juce::AlertWindow &alert,
                                       const juce::Rectangle<int> &,
                                       juce::TextLayout &textLayout) {
  using namespace colours;
  const auto bounds = alert.getLocalBounds();
  g.fillAll(alert.findColour(juce::AlertWindow::backgroundColourId));
  drawStripes(g, bounds.toFloat().withHeight(5.0f), accent().withAlpha(0.75f), 9.0f, 4.0f);
  g.setColour(cyan.withAlpha(0.75f));
  g.drawRect(bounds, 1);

  int iconSpace = 0;
  if (const auto type = alert.getAlertType(); type != juce::MessageBoxIconType::NoIcon) {
    // A sign in wireframe: a triangle to warn, a square otherwise.
    const auto icon = juce::Rectangle<float>(18.0f, 28.0f, 46.0f, 40.0f);
    const bool warning = type == juce::MessageBoxIconType::WarningIcon;
    juce::Path sign;
    if (warning)
      sign.addTriangle(icon.getCentreX(), icon.getY(), icon.getRight(), icon.getBottom(),
                       icon.getX(), icon.getBottom());
    else
      sign.addRectangle(icon.withSizeKeepingCentre(icon.getHeight(), icon.getHeight()));
    const auto colour = warning ? yellow : cyan;
    strokeNeon(g, sign, colour, 1.5f, 0.8f);
    g.setColour(colour);
    g.setFont(headline(17.0f));
    g.drawText(warning ? "!" : type == juce::MessageBoxIconType::InfoIcon ? "i" : "?",
               icon.withTrimmedTop(warning ? 12.0f : 0.0f), juce::Justification::centred,
               false);
    iconSpace = 80;
  }
  textLayout.draw(g, juce::Rectangle<int>(bounds.getX() + iconSpace, 30, bounds.getWidth(),
                                          bounds.getHeight() - getAlertWindowButtonHeight() - 20)
                         .toFloat());
}

void HeadroomLookAndFeel::drawTextEditorOutline(juce::Graphics &g, int width, int height,
                                                juce::TextEditor &editor) {
  if (dynamic_cast<juce::AlertWindow *>(editor.getParentComponent()) != nullptr ||
      !editor.isEnabled())
    return;
  const auto bounds = juce::Rectangle<int>(width, height).toFloat();
  if (editor.hasKeyboardFocus(true) && !editor.isReadOnly()) {
    const auto colour = editor.findColour(juce::TextEditor::focusedOutlineColourId);
    g.setColour(colour.withMultipliedAlpha(0.22f));
    g.drawRect(bounds.reduced(1.0f), 2.0f); // its glow, inwards
    g.setColour(colour);
    g.drawRect(bounds, 1.0f);
    return;
  }
  g.setColour(editor.findColour(juce::TextEditor::outlineColourId));
  g.drawRect(bounds, 1.0f);
}

void HeadroomLookAndFeel::drawScrollbar(juce::Graphics &g, juce::ScrollBar &bar, int x, int y,
                                        int width, int height, bool vertical,
                                        int thumbStart, int thumbSize, bool isMouseOver,
                                        bool isMouseDown) {
  using namespace colours;
  const juce::Rectangle<float> track((float)x, (float)y, (float)width, (float)height);
  constexpr float thickness = 3.0f;
  g.setColour(cyan.withAlpha(0.12f));
  g.fillRect(vertical ? track.withSizeKeepingCentre(1.0f, track.getHeight())
                      : track.withSizeKeepingCentre(track.getWidth(), 1.0f));
  if (thumbSize <= 0)
    return;
  const auto thumb =
      vertical ? juce::Rectangle<float>(track.getX(), (float)thumbStart, track.getWidth(),
                                        (float)thumbSize)
                     .withSizeKeepingCentre(thickness, (float)thumbSize)
               : juce::Rectangle<float>((float)thumbStart, track.getY(), (float)thumbSize,
                                        track.getHeight())
                     .withSizeKeepingCentre((float)thumbSize, thickness);
  g.setColour(isMouseOver || isMouseDown ? accent() : bar.findColour(juce::ScrollBar::thumbColourId));
  g.fillRect(thumb);
}

void HeadroomLookAndFeel::drawCornerResizer(juce::Graphics &g, int w, int h,
                                            bool isMouseOver,
                                            bool isMouseDragging) {
  g.setColour(isMouseDragging ? colours::accent()
              : isMouseOver   ? colours::cyan
                              : colours::mutedSoft);
  for (float i = 0.3f; i < 1.0f; i += 0.3f)
    g.drawLine((float)w * i, (float)h, (float)w, (float)h * i, 1.0f);
}

void HeadroomLookAndFeel::drawTooltip(juce::Graphics &g, const juce::String &text,
                                      int width, int height) {
  const auto bounds = juce::Rectangle<int>(width, height).toFloat();
  g.fillAll(colours::black);
  g.setColour(colours::cyan.withAlpha(0.6f));
  g.drawRect(bounds, 1.0f);
  g.setColour(colours::accent());
  g.fillRect(bounds.withWidth(2.0f));
  layoutTooltip(text, colours::text).draw(g, bounds.reduced(10.0f, 6.0f).withTrimmedLeft(2.0f));
}

juce::Rectangle<int>
HeadroomLookAndFeel::getTooltipBounds(const juce::String &text,
                                      juce::Point<int> screenPos,
                                      juce::Rectangle<int> parentArea) {
  const auto layout = layoutTooltip(text, colours::text);
  const int w = (int)std::ceil(layout.getWidth()) + 22;
  const int h = (int)std::ceil(layout.getHeight()) + 12;
  return juce::Rectangle<int>(screenPos.x > parentArea.getCentreX()
                                  ? screenPos.x - (w + 12)
                                  : screenPos.x + 24,
                              screenPos.y > parentArea.getCentreY()
                                  ? screenPos.y - (h + 6)
                                  : screenPos.y + 6,
                              w, h)
      .constrainedWithin(parentArea);
}

juce::Font HeadroomLookAndFeel::getTextButtonFont(juce::TextButton &, int) {
  return caps(10.0f, 0.12f);
}

juce::Font HeadroomLookAndFeel::getComboBoxFont(juce::ComboBox &) {
  return readout(19.0f);
}

juce::Font HeadroomLookAndFeel::getPopupMenuFont() { return readout(19.0f); }

juce::Font HeadroomLookAndFeel::getLabelFont(juce::Label &label) {
  return label.getFont();
}

juce::Font HeadroomLookAndFeel::getAlertWindowTitleFont() { return headline(15.0f); }

juce::Font HeadroomLookAndFeel::getAlertWindowMessageFont() { return body(13.0f); }

juce::Font HeadroomLookAndFeel::getAlertWindowFont() { return caps(10.0f, 0.12f); }

juce::Label *HeadroomLookAndFeel::createSliderTextBox(juce::Slider &slider) {
  auto *label = LookAndFeel_V4::createSliderTextBox(slider);
  const bool compact = slider.getProperties()["compact"];
  label->setFont(readout(compact ? 15.0f : 19.0f));
  label->setColour(juce::Label::textColourId,
                   slider.findColour(juce::Slider::textBoxTextColourId));
  label->setColour(juce::Label::outlineColourId,
                   juce::Colours::transparentBlack);
  label->setColour(juce::Label::backgroundColourId,
                   juce::Colours::transparentBlack);
  // Typing happens in place: the same text in the same spot, on a field, or on
  // the bar a bar slider already draws. JUCE's default fill is the transparent
  // colour made opaque.
  label->setColour(juce::Label::textWhenEditingColourId, colours::text);
  label->setColour(juce::Label::backgroundWhenEditingColourId,
                   slider.isBar() ? juce::Colours::transparentBlack
                                  : colours::glass.withAlpha(0.92f));
  label->setColour(juce::Label::outlineWhenEditingColourId,
                   juce::Colours::transparentBlack);
  label->onEditorShow = [label, compact] {
    auto *editor = label->getCurrentTextEditor();
    if (editor == nullptr)
      return;
    // JUCE fills the editor while it's 10 pixels wide, which leaves the text
    // scrolled to the left. Undo that first: once centred, the caret at the start
    // is already in view and wouldn't scroll back. No top indent, as it would push
    // the text below the middle of a box this short.
    // A pedal's box is barely wider than its text, so no side indent there either.
    editor->setCaretPosition(0);
    editor->setIndents(compact ? 0 : editor->getLeftIndent(), 0);
    editor->setJustification(label->getJustificationType());
    editor->selectAll();
  };
  return label;
}
} // namespace headroom::gui
