#include "Neon.h"

#include "HeadroomLookAndFeel.h"

#include <array>

namespace headroom::gui {
namespace {
const juce::Colour shadow = juce::Colours::black;

void clipTo(juce::Graphics &g, juce::Rectangle<float> area) {
  juce::Path clip;
  clip.addRectangle(area);
  g.reduceClipRegion(clip);
}
} // namespace

void PaintCache::draw(juce::Graphics &g, juce::Point<float> at, int width,
                      int height, juce::int64 key, const Painter &paint) {
  const float scale = juce::jlimit(
      0.25f, 8.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
  const auto signal = colours::accent().getARGB();
  if (!image.isValid() || width != cachedWidth || height != cachedHeight ||
      !juce::approximatelyEqual(scale, cachedScale) || key != cachedKey ||
      signal != cachedSignal) {
    image = juce::Image(juce::Image::ARGB,
                        juce::jmax(1, (int)std::ceil((float)width * scale)),
                        juce::jmax(1, (int)std::ceil((float)height * scale)), true);
    juce::Graphics painter(image);
    painter.addTransform(juce::AffineTransform::scale(scale));
    paint(painter);
    cachedWidth = width;
    cachedHeight = height;
    cachedScale = scale;
    cachedSignal = signal;
    cachedKey = key;
  }
  const juce::Graphics::ScopedSaveState save(g);
  g.setOpacity(1.0f);
  g.setImageResamplingQuality(juce::Graphics::mediumResamplingQuality);
  g.drawImageTransformed(image, juce::AffineTransform::scale(1.0f / scale)
                                    .translated(at.x, at.y));
}

juce::Rectangle<float> circle(juce::Point<float> centre, float radius) {
  return juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre);
}

juce::Path bevelBox(juce::Rectangle<float> b, float cut) {
  cut = juce::jlimit(0.0f, juce::jmin(b.getWidth(), b.getHeight()) * 0.5f, cut);
  juce::Path box;
  box.startNewSubPath(b.getX() + cut, b.getY());
  box.lineTo(b.getRight(), b.getY());
  box.lineTo(b.getRight(), b.getBottom() - cut);
  box.lineTo(b.getRight() - cut, b.getBottom());
  box.lineTo(b.getX(), b.getBottom());
  box.lineTo(b.getX(), b.getY() + cut);
  box.closeSubPath();
  return box;
}

void strokeNeon(juce::Graphics &g, const juce::Path &path, juce::Colour colour,
                float width, float glow) {
  glow = juce::jlimit(0.0f, 2.0f, glow);
  if (glow > 0.0f) {
    g.setColour(colour.withMultipliedAlpha(0.09f * glow));
    g.strokePath(path, juce::PathStrokeType(width + 5.0f, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
    g.setColour(colour.withMultipliedAlpha(0.2f * glow));
    g.strokePath(path, juce::PathStrokeType(width + 2.0f, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
  }
  g.setColour(colour);
  g.strokePath(path, juce::PathStrokeType(width, juce::PathStrokeType::mitered,
                                          juce::PathStrokeType::square));
}

void fillNeon(juce::Graphics &g, const juce::Path &shape, juce::Colour colour, float glow) {
  glow = juce::jlimit(0.0f, 2.0f, glow);
  if (glow > 0.0f) {
    g.setColour(colour.withMultipliedAlpha(0.1f * glow));
    g.strokePath(shape, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
    g.setColour(colour.withMultipliedAlpha(0.22f * glow));
    g.strokePath(shape, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
  }
  g.setColour(colour);
  g.fillPath(shape);
}

juce::Path textPath(const juce::String &text, const juce::Font &font,
                    juce::Rectangle<float> area, juce::Justification justification,
                    int maxLines, float minimumScale) {
  juce::GlyphArrangement glyphs;
  glyphs.addFittedText(font, text, area.getX(), area.getY(), area.getWidth(),
                       area.getHeight(), justification, maxLines, minimumScale);
  juce::Path letters;
  glyphs.createPath(letters);
  return letters;
}

void drawAberrated(juce::Graphics &g, const juce::Path &letters, juce::Colour colour,
                   float offset) {
  const float alpha = colour.getFloatAlpha();
  g.setColour(colours::magenta.withAlpha(0.72f * alpha));
  g.fillPath(letters, juce::AffineTransform::translation(-offset, 0.0f));
  g.setColour(colours::cyan.withAlpha(0.72f * alpha));
  g.fillPath(letters, juce::AffineTransform::translation(offset, 0.0f));
  g.setColour(colour);
  g.fillPath(letters);
}

void drawAberrated(juce::Graphics &g, const juce::String &text, const juce::Font &font,
                   juce::Rectangle<float> area, juce::Justification justification,
                   juce::Colour colour, float offset) {
  drawAberrated(g, textPath(text, font, area, justification), colour, offset);
}

void drawScanlines(juce::Graphics &g, juce::Rectangle<float> area, float strength) {
  const float scale = juce::jmax(1.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
  const float pitch = 3.0f / scale, thickness = 1.0f / scale;
  juce::RectangleList<float> lines;
  for (float y = std::ceil(area.getY() / pitch) * pitch; y < area.getBottom(); y += pitch)
    lines.addWithoutMerging({area.getX(), y, area.getWidth(), thickness});
  g.setColour(shadow.withAlpha(strength));
  g.fillRectList(lines);
}

void drawScreen(juce::Graphics &g, juce::Rectangle<float> area) {
  using namespace colours;
  g.setColour(glass);
  g.fillRect(area);
  g.setColour(blue.withAlpha(0.8f));
  g.drawRect(area, 1.0f);

  const auto r = area.reduced(0.5f);
  const float arm = juce::jmin(9.0f, r.getWidth() * 0.2f, r.getHeight() * 0.2f);
  juce::Path brackets;
  for (const auto &[corner, dx, dy] :
       {std::tuple{r.getTopLeft(), 1.0f, 1.0f}, std::tuple{r.getTopRight(), -1.0f, 1.0f},
        std::tuple{r.getBottomLeft(), 1.0f, -1.0f},
        std::tuple{r.getBottomRight(), -1.0f, -1.0f}}) {
    brackets.startNewSubPath(corner.translated(0.0f, dy * arm));
    brackets.lineTo(corner);
    brackets.lineTo(corner.translated(dx * arm, 0.0f));
  }
  g.setColour(cyan.withAlpha(0.75f));
  g.strokePath(brackets, juce::PathStrokeType(1.0f, juce::PathStrokeType::mitered,
                                              juce::PathStrokeType::butt));
}

void drawLed(juce::Graphics &g, juce::Rectangle<float> led, bool lit, juce::Colour colour) {
  if (lit) {
    const auto c = led.getCentre();
    const float reach = led.getWidth() * 2.4f;
    g.setGradientFill(juce::ColourGradient(colour.withAlpha(0.5f), c, colour.withAlpha(0.0f),
                                           c.translated(reach, 0.0f), true));
    g.fillEllipse(circle(c, reach));
    g.setColour(colour);
    g.fillRect(led);
  } else {
    g.setColour(colour.withAlpha(0.1f));
    g.fillRect(led);
    g.setColour(colour.withAlpha(0.42f));
    g.drawRect(led, 1.0f);
  }
}

void drawStripes(juce::Graphics &g, juce::Rectangle<float> area, juce::Colour colour,
                 float pitch, float width, float phase) {
  if (area.isEmpty() || pitch <= 0.0f)
    return;
  const juce::Graphics::ScopedSaveState save(g);
  clipTo(g, area);
  const float rise = area.getHeight();
  const float shift = phase - pitch * std::floor(phase / pitch);
  juce::Path stripes;
  for (float x = area.getX() - rise - pitch + shift; x < area.getRight(); x += pitch) {
    stripes.startNewSubPath(x, area.getBottom());
    stripes.lineTo(x + rise, area.getY());
    stripes.lineTo(x + rise + width, area.getY());
    stripes.lineTo(x + width, area.getBottom());
    stripes.closeSubPath();
  }
  g.setColour(colour);
  g.fillPath(stripes);
}

void drawFloorGrid(juce::Graphics &g, juce::Rectangle<float> floor, juce::Colour colour,
                   float travel) {
  if (floor.getHeight() < 2.0f)
    return;
  const juce::Graphics::ScopedSaveState save(g);
  clipTo(g, floor);
  const float horizon = floor.getY(), bottom = floor.getBottom();
  const float centre = floor.getCentreX();

  // Lines running back to the vanishing point, a cell apart along the front
  // edge, and enough of them to reach the sides at the horizon.
  juce::Path lines;
  const float cell = juce::jmax(floor.getHeight() * 0.62f, floor.getWidth() / 14.0f);
  const int each = (int)std::ceil(4.0f * floor.getWidth() / cell);
  for (int i = -each; i <= each; ++i) {
    lines.startNewSubPath(centre, horizon);
    lines.lineTo(centre + (float)i * cell, bottom);
  }
  // Lines across, closing up towards the horizon.
  constexpr float ratio = 0.66f;
  const float t = travel - std::floor(travel);
  for (int k = 0; k < 24; ++k) {
    const float y = horizon + floor.getHeight() * std::pow(ratio, (float)k - t);
    if (y - horizon < 0.7f)
      break;
    lines.startNewSubPath(floor.getX(), y);
    lines.lineTo(floor.getRight(), y);
  }
  g.setGradientFill(juce::ColourGradient(colour.withAlpha(0.0f), 0.0f, horizon, colour, 0.0f,
                                         bottom, false));
  g.strokePath(lines, juce::PathStrokeType(1.0f));
}

void drawTunnel(juce::Graphics &g, juce::Rectangle<float> area, juce::Point<float> centre,
                juce::Colour colour, float travel) {
  const juce::Graphics::ScopedSaveState save(g);
  clipTo(g, area);
  const float aspect = area.getWidth() / juce::jmax(1.0f, area.getHeight());
  const float reach = juce::jmax(centre.y - area.getY(), area.getBottom() - centre.y,
                                 (centre.x - area.getX()) / aspect,
                                 (area.getRight() - centre.x) / aspect);
  constexpr float ratio = 1.3f, innermost = 7.0f;
  const float t = travel - std::floor(travel);

  // The rails along its corners, then each square across it.
  juce::Path rails;
  for (const auto &[dx, dy] : {std::pair{-1.0f, -1.0f}, std::pair{1.0f, -1.0f},
                               std::pair{1.0f, 1.0f}, std::pair{-1.0f, 1.0f}}) {
    rails.startNewSubPath(centre.translated(dx * innermost * aspect, dy * innermost));
    rails.lineTo(centre.translated(dx * reach * aspect, dy * reach));
  }
  g.setGradientFill(juce::ColourGradient(colour.withMultipliedAlpha(0.05f), centre,
                                         colour.withMultipliedAlpha(0.6f),
                                         centre.translated(reach * aspect, 0.0f), true));
  g.strokePath(rails, juce::PathStrokeType(1.0f));
  for (int k = 0;; ++k) {
    const float half = innermost * std::pow(ratio, (float)k + t);
    if (half > reach * 1.05f)
      break;
    const float out = juce::jlimit(0.0f, 1.0f, (half - innermost) / reach);
    g.setColour(colour.withMultipliedAlpha(0.08f + 0.85f * out));
    g.drawRect(juce::Rectangle<float>(half * 2.0f * aspect, half * 2.0f).withCentre(centre),
               1.0f);
  }
}

juce::Rectangle<float> drawPanel(juce::Graphics &g, juce::Rectangle<int> bounds,
                                 const juce::String &title, float reserveRight) {
  using namespace colours;
  const auto b = bounds.toFloat().reduced(0.5f);
  constexpr float cut = 12.0f, arm = 18.0f;
  const auto shape = bevelBox(b, cut);
  g.setColour(panel);
  g.fillPath(shape);
  strokeNeon(g, shape, cyan.withAlpha(0.38f), 1.0f, 0.3f);

  // The cut corners, lit full, with a little of the edges either side.
  juce::Path corners;
  corners.startNewSubPath(b.getX(), b.getY() + cut + arm);
  corners.lineTo(b.getX(), b.getY() + cut);
  corners.lineTo(b.getX() + cut, b.getY());
  corners.lineTo(b.getX() + cut + arm, b.getY());
  corners.startNewSubPath(b.getRight(), b.getBottom() - cut - arm);
  corners.lineTo(b.getRight(), b.getBottom() - cut);
  corners.lineTo(b.getRight() - cut, b.getBottom());
  corners.lineTo(b.getRight() - cut - arm, b.getBottom());
  strokeNeon(g, corners, cyan, 1.5f, 0.8f);

  // The title: a slanted tab of the accent, the name, then a rule.
  auto row = b.reduced(18.0f, 0.0f).withTop(b.getY() + 8.0f).withHeight(19.0f);
  {
    const auto tab = row.removeFromLeft(9.0f).withSizeKeepingCentre(9.0f, 10.0f);
    juce::Path slant;
    slant.addQuadrilateral(tab.getX(), tab.getBottom(), tab.getX() + 4.0f, tab.getY(),
                           tab.getRight(), tab.getY(), tab.getRight() - 4.0f, tab.getBottom());
    fillNeon(g, slant, accent(), 0.6f);
  }
  row.removeFromLeft(8.0f);
  const auto titleFont = caps(11.5f, 0.2f);
  const auto titleText = title.toUpperCase();
  const float titleWidth = juce::GlyphArrangement::getStringWidth(titleFont, titleText) + 4.0f;
  drawAberrated(g, titleText, titleFont, row.removeFromLeft(titleWidth),
                juce::Justification::centredLeft, text, 1.0f);
  row.removeFromLeft(10.0f);

  const auto rule = row.withTrimmedRight(reserveRight);
  const float y = std::round(row.getCentreY());
  g.setColour(cyan.withAlpha(0.22f));
  g.fillRect(rule.withY(y).withHeight(1.0f));
  if (rule.getWidth() > 24.0f) { // a lit block at its far end
    g.setColour(cyan.withAlpha(0.8f));
    g.fillRect(juce::Rectangle<float>(rule.getRight() - 10.0f, y - 1.0f, 10.0f, 3.0f));
  }
  return row;
}

void drawChassis(juce::Graphics &g, juce::Rectangle<int> bounds) {
  const auto b = bounds.toFloat();
  g.setColour(colours::chassis);
  g.fillRect(b);
  // Graph paper, barely there.
  constexpr float pitch = 24.0f;
  juce::RectangleList<float> grid;
  for (float x = b.getX() + pitch; x < b.getRight(); x += pitch)
    grid.addWithoutMerging({x, b.getY(), 1.0f, b.getHeight()});
  for (float y = b.getY() + pitch; y < b.getBottom(); y += pitch)
    grid.addWithoutMerging({b.getX(), y, b.getWidth(), 1.0f});
  g.setColour(colours::blue.withAlpha(0.08f));
  g.fillRectList(grid);
}

void drawCrtGlass(juce::Graphics &g, juce::Rectangle<float> area) {
  drawScanlines(g, area, 0.2f);

  // A vignette, elliptical to fit the picture, darkest in its corners.
  {
    const juce::Graphics::ScopedSaveState save(g);
    const auto c = area.getCentre();
    const float rx = area.getWidth() * 0.5f, ry = area.getHeight() * 0.5f;
    g.addTransform(juce::AffineTransform::scale(1.0f, ry / rx, c.x, c.y));
    const float reach = rx * 1.42f;
    juce::ColourGradient shade(shadow.withAlpha(0.0f), c, shadow.withAlpha(0.42f),
                               c.translated(reach, 0.0f), true);
    shade.addColour(0.64, shadow.withAlpha(0.0f));
    g.setGradientFill(shade);
    g.fillRect(juce::Rectangle<float>(reach * 2.0f, reach * 2.0f).withCentre(c));
  }
  // The glass curving away at its edges.
  constexpr float edge = 9.0f;
  const auto fade = [&g](juce::Rectangle<float> band, juce::Point<float> from,
                         juce::Point<float> to) {
    g.setGradientFill(juce::ColourGradient(shadow.withAlpha(0.32f), from, shadow.withAlpha(0.0f),
                                           to, false));
    g.fillRect(band);
  };
  fade(area.withHeight(edge), area.getTopLeft(), area.getTopLeft().translated(0.0f, edge));
  fade(area.withTop(area.getBottom() - edge), area.getBottomLeft(),
       area.getBottomLeft().translated(0.0f, -edge));
  fade(area.withWidth(edge), area.getTopLeft(), area.getTopLeft().translated(edge, 0.0f));
  fade(area.withLeft(area.getRight() - edge), area.getTopRight(),
       area.getTopRight().translated(-edge, 0.0f));
}

namespace {
/** The logo's letters for this height, their ink starting at the origin. */
juce::Path logoLetters(float height) {
  juce::GlyphArrangement word;
  word.addLineOfText(logoFont(height), "HEADROOM", 0.0f, 0.0f);
  juce::Path outline;
  word.createPath(outline);

  // Fit the letters' real ink to the top of the height, leaving the rest for
  // the line under them.
  const auto ink = outline.getBounds();
  const float fit = height * 0.62f / ink.getHeight();
  outline.applyTransform(
      juce::AffineTransform::translation(-ink.getX(), -ink.getY()).scaled(fit));
  return outline;
}
} // namespace

float logoWidth(float height) { return logoLetters(height).getBounds().getWidth(); }

void drawLogo(juce::Graphics &g, juce::Point<float> topLeft, float height) {
  using namespace colours;
  auto outline = logoLetters(height);
  outline.applyTransform(juce::AffineTransform::translation(topLeft));
  const auto letters = outline.getBounds();
  const float offset = juce::jmax(1.0f, height * 0.045f);

  juce::DropShadow(magenta.withAlpha(0.6f), juce::roundToInt(height * 0.3f), {})
      .drawForPath(g, outline);
  g.setColour(magenta.withAlpha(0.9f));
  g.fillPath(outline, juce::AffineTransform::translation(-offset, 0.0f));
  g.setColour(cyan.withAlpha(0.9f));
  g.fillPath(outline, juce::AffineTransform::translation(offset, 0.0f));

  // The face: cyan, running down into blue, as an eighties logo's chrome runs
  // into its horizon.
  juce::ColourGradient face(cyan, 0.0f, letters.getY(), cyan.interpolatedWith(blue, 0.7f), 0.0f,
                            letters.getBottom(), false);
  face.addColour(0.45, cyan);
  g.setGradientFill(face);
  g.fillPath(outline);

  // Two slits low across it, the lower one wider.
  g.setColour(black);
  const auto span = letters.expanded(offset + 3.0f, 0.0f);
  for (const auto &[at, thickness] : {std::pair{0.7f, 0.05f}, std::pair{0.84f, 0.075f}})
    g.fillRect(span.withY(letters.getY() + letters.getHeight() * at)
                   .withHeight(letters.getHeight() * thickness));

  // The line under it, tracked out to the width of the word.
  const auto sub = height * 0.2f;
  const juce::String subtitle("AMPLIFIER");
  auto subFont = caps(sub, 0.0f);
  const float bare = juce::GlyphArrangement::getStringWidth(subFont, subtitle);
  subFont = caps(sub, juce::jmax(0.0f, (letters.getWidth() - bare) /
                                           ((float)subtitle.length() * subFont.getHeight())));
  g.setFont(subFont);
  g.setColour(cyan.withAlpha(0.85f));
  g.drawText(subtitle,
             juce::Rectangle<float>(letters.getX(), letters.getBottom() + height * 0.07f,
                                    letters.getWidth() + 8.0f, sub * 1.25f),
             juce::Justification::centredLeft, false);
}

// --- the cube ----------------------------------------------------------------
namespace {
/** A cube in perspective, turned `yaw` about its upright and tipped `pitch`
    towards the viewer; its far edges are dimmer than its near ones. */
void drawCube(juce::Graphics &g, juce::Point<float> centre, float size, float yaw,
              float pitch, juce::Colour colour, float glow) {
  std::array<juce::Point<float>, 8> at;
  std::array<float, 8> depth;
  const float cy = std::cos(yaw), sy = std::sin(yaw);
  const float cp = std::cos(pitch), sp = std::sin(pitch);
  constexpr float camera = 3.8f;
  for (int i = 0; i < 8; ++i) {
    const float x = (i & 1) != 0 ? 1.0f : -1.0f, y = (i & 2) != 0 ? 1.0f : -1.0f,
                z = (i & 4) != 0 ? 1.0f : -1.0f;
    const float x1 = x * cy + z * sy, z1 = z * cy - x * sy;
    const float y2 = y * cp - z1 * sp, z2 = y * sp + z1 * cp;
    const float k = camera / (camera + z2);
    at[(size_t)i] = centre + juce::Point<float>(x1, y2) * (size * k);
    depth[(size_t)i] = z2;
  }
  for (int i = 0; i < 8; ++i)
    for (const int bit : {1, 2, 4}) {
      const int j = i | bit;
      if (j == i)
        continue;
      const float nearness = juce::jlimit(
          0.0f, 1.0f, 0.5f - (depth[(size_t)i] + depth[(size_t)j]) * 0.25f);
      juce::Path edge;
      edge.startNewSubPath(at[(size_t)i]);
      edge.lineTo(at[(size_t)j]);
      strokeNeon(g, edge, colour.withMultipliedAlpha(0.3f + 0.7f * nearness), 1.2f,
                 glow * (0.3f + 0.7f * nearness));
    }
  g.setColour(colours::text.withMultipliedAlpha(colour.getFloatAlpha()));
  for (const auto &point : at)
    g.fillRect(juce::Rectangle<float>(2.4f, 2.4f).withCentre(point));
}
} // namespace

void WireCube::update(bool powered, float signal) {
  const float before = heat;
  // Slow to warm up and cool down, quick to answer the guitar.
  heat += powered ? juce::jmin(0.04f, 1.0f - heat) : -juce::jmin(0.05f, heat);
  const float target = juce::jlimit(0.0f, 1.0f, signal);
  drive += (target > drive ? 0.5f : 0.08f) * (target - drive);

  constexpr float tick = 1.0f / 30.0f, twoPi = juce::MathConstants<float>::twoPi;
  turn += tick * heat * (0.3f + 2.4f * drive);
  turn -= twoPi * std::floor(turn / twoPi);
  travel += tick * heat * (0.3f + 1.5f * drive);
  travel -= std::floor(travel);
  if (heat > 0.0f || before > 0.0f)
    repaint();
}

void WireCube::snapTo(bool powered, float signal) {
  heat = powered ? 1.0f : 0.0f;
  drive = juce::jlimit(0.0f, 1.0f, signal);
  repaint();
}

void WireCube::paint(juce::Graphics &g) {
  using namespace colours;
  const auto b = getLocalBounds().toFloat();
  const float glow = heat * (0.35f + 0.65f * drive);
  const float horizon = std::round(b.getY() + b.getHeight() * 0.64f);

  drawFloorGrid(g, b.withTop(horizon), blue.withAlpha(0.5f + 0.5f * heat), travel);
  juce::Path line;
  line.startNewSubPath(b.getX(), horizon);
  line.lineTo(b.getRight(), horizon);
  strokeNeon(g, line, accent().withAlpha(0.2f + 0.8f * heat), 1.0f, glow);

  const float room = horizon - b.getY();
  const auto centre = juce::Point<float>(b.getCentreX(), b.getY() + room * 0.5f);
  const float size = juce::jmin(b.getWidth() * 0.22f, room * 0.29f);
  const auto edges = cyan.interpolatedWith(blue, 0.75f * (1.0f - heat));
  drawCube(g, centre, size, turn, 0.42f + 0.14f * std::sin(turn),
           edges.withAlpha(0.45f + 0.55f * heat), glow);
  drawCube(g, centre, size * 0.46f, 0.4f - 2.0f * turn, 0.32f,
           accent().withAlpha(0.25f + 0.75f * heat), glow);
}

// --- the speaker -------------------------------------------------------------
void Speaker::setActive(bool shouldBeActive) {
  if (active != shouldBeActive) {
    active = shouldBeActive;
    repaint();
  }
}

void Speaker::paint(juce::Graphics &g) {
  const auto size = juce::jmin(getWidth(), getHeight());
  cone.draw(g, getLocalBounds(), active ? 1 : 0, [size, on = active](juce::Graphics &c) {
    using namespace colours;
    const auto centre = juce::Point<float>((float)size, (float)size) * 0.5f;
    const float r = (float)size * 0.5f - 4.0f;
    const float glow = on ? 0.7f : 0.0f;
    const auto lines = on ? cyan : mutedSoft;
    constexpr float pi = juce::MathConstants<float>::pi;

    // The frame, an octagon, bolted down at its diagonals.
    juce::Path frame;
    frame.addPolygon(centre, 8, r, pi / 8.0f);
    strokeNeon(c, frame, lines.withAlpha(0.85f), 1.2f, glow * 0.6f);
    c.setColour(lines.withAlpha(0.7f));
    for (int i = 0; i < 4; ++i)
      c.drawRect(juce::Rectangle<float>(4.0f, 4.0f).withCentre(
                     centre.getPointOnCircumference(r * 0.86f, pi * (0.25f + 0.5f * (float)i))),
                 1.0f);

    // The surround, two rings.
    for (const float ring : {0.8f, 0.75f}) {
      c.setColour(lines.withAlpha(0.75f));
      c.drawEllipse(circle(centre, r * ring), 1.0f);
    }

    // The cone in perspective: spokes running down it, and rings closing up
    // towards the throat.
    constexpr float rim = 0.71f, throat = 0.27f;
    juce::Path spokes;
    for (int i = 0; i < 16; ++i) {
      const float angle = pi * (float)i / 8.0f;
      spokes.startNewSubPath(centre.getPointOnCircumference(r * throat, angle));
      spokes.lineTo(centre.getPointOnCircumference(r * rim, angle));
    }
    c.setColour((on ? blue : mutedSoft).withAlpha(0.75f));
    c.strokePath(spokes, juce::PathStrokeType(1.0f));
    constexpr int rings = 7;
    for (int i = 0; i < rings; ++i) {
      const float t = (float)i / (float)rings;
      const float radius = r * (rim - (rim - throat) * std::pow(t, 0.75f));
      c.setColour(i % 2 == 0 ? lines.withAlpha(0.75f - 0.06f * (float)i)
                             : (on ? blue : mutedSoft));
      c.drawEllipse(circle(centre, radius), 1.0f);
    }

    // The dust cap, lit in the accent.
    juce::Path cap;
    cap.addEllipse(circle(centre, r * 0.22f));
    c.setColour(glass);
    c.fillPath(cap);
    const auto lit = on ? accent() : mutedSoft;
    strokeNeon(c, cap, lit, 1.3f, glow);
    c.setColour(lit.withAlpha(0.55f));
    c.drawEllipse(circle(centre, r * 0.13f), 1.0f);
    c.fillRect(juce::Rectangle<float>(3.0f, 3.0f).withCentre(centre));
  });
}
} // namespace headroom::gui
