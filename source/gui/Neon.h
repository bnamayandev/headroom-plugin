#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace headroom::gui {
/** Keeps a static paint job in an image at the pixel scale it's drawn at, so
    glows, grids and headlines cost one paint rather than one per frame. It
    paints again if the size, the scale, the signal colour or the key changes. */
class PaintCache {
public:
  using Painter = std::function<void(juce::Graphics &)>;

  /** Draws the job with its top left at `at`; it paints from (0, 0). */
  void draw(juce::Graphics &g, juce::Point<float> at, int width, int height,
            juce::int64 key, const Painter &paint);
  void draw(juce::Graphics &g, juce::Rectangle<int> area, juce::int64 key,
            const Painter &paint) {
    draw(g, area.getPosition().toFloat(), area.getWidth(), area.getHeight(), key,
         paint);
  }

private:
  juce::Image image;
  int cachedWidth = 0, cachedHeight = 0;
  float cachedScale = 0.0f;
  juce::uint32 cachedSignal = 0;
  juce::int64 cachedKey = 0;
};

juce::Rectangle<float> circle(juce::Point<float> centre, float radius);

/** A box with its top-left and bottom-right corners cut off at 45 degrees: the
    shape every panel, field and button is cut to. */
juce::Path bevelBox(juce::Rectangle<float> box, float cut);

/** Strokes a path as lit phosphor: a soft bloom round it, then the line itself.
    `glow` scales the bloom; at 0 it's the bare line. */
void strokeNeon(juce::Graphics &, const juce::Path &, juce::Colour, float width = 1.0f,
                float glow = 1.0f);
/** Fills a shape as a lit block of phosphor, blooming past its edge. */
void fillNeon(juce::Graphics &, const juce::Path &, juce::Colour, float glow = 1.0f);

/** The outlines of some text, fitted into an area as Graphics::drawFittedText()
    would place it. */
juce::Path textPath(const juce::String &, const juce::Font &, juce::Rectangle<float> area,
                    juce::Justification, int maxLines = 1, float minimumScale = 0.7f);
/** Fills letters as a misconverged picture tube shows them: a magenta ghost
    `offset` to the left and a cyan one to the right. */
void drawAberrated(juce::Graphics &, const juce::Path &letters, juce::Colour,
                   float offset = 1.0f);
void drawAberrated(juce::Graphics &, const juce::String &, const juce::Font &,
                   juce::Rectangle<float> area, juce::Justification, juce::Colour,
                   float offset = 1.0f);

/** Scanlines: a dark line every third physical pixel, so they stay crisp at any
    scale. */
void drawScanlines(juce::Graphics &, juce::Rectangle<float> area, float strength);

/** A monitor set into a panel: a black field in a thin blue frame, with lit
    brackets at its corners. */
void drawScreen(juce::Graphics &, juce::Rectangle<float> area);

/** A square indicator, blooming when lit. */
void drawLed(juce::Graphics &, juce::Rectangle<float> led, bool lit, juce::Colour);

/** Diagonal stripes across an area, rising to the right. `phase` slides them
    along, in pixels. */
void drawStripes(juce::Graphics &, juce::Rectangle<float> area, juce::Colour, float pitch,
                 float width, float phase = 0.0f);

/** A floor ruled in perspective, running back to a horizon along the area's top
    edge. `travel` (0..1, it wraps) brings the cross lines towards the viewer. */
void drawFloorGrid(juce::Graphics &, juce::Rectangle<float> floor, juce::Colour,
                   float travel = 0.0f);

/** Squares nested round a centre, each further out bigger by the same ratio, as
    if looking down a tunnel. `travel` (0..1, it wraps) moves them outwards. */
void drawTunnel(juce::Graphics &, juce::Rectangle<float> area, juce::Point<float> centre,
                juce::Colour, float travel = 0.0f);

/** A panel: a bevelled frame on the black, its title at the top left followed
    by a rule. The rule stops `reserveRight` short of its row, leaving room for
    an indicator there. Returns the row. */
juce::Rectangle<float> drawPanel(juce::Graphics &, juce::Rectangle<int> bounds,
                                 const juce::String &title, float reserveRight = 0.0f);

/** The black behind the panels, with a faint grid ruled across it. */
void drawChassis(juce::Graphics &, juce::Rectangle<int> bounds);

/** The tube's glass over everything: scanlines, and the picture falling into
    shadow towards the corners, where the glass curves away. */
void drawCrtGlass(juce::Graphics &, juce::Rectangle<float> area);

/** The logo filling this height, with its top left here. */
void drawLogo(juce::Graphics &, juce::Point<float> topLeft, float height);
/** How wide drawLogo() draws at this height. */
float logoWidth(float height);

/** A wireframe cube turning over a perspective grid: dark until a model's
    loaded, then turning, faster and brighter with the signal. */
class WireCube final : public juce::Component {
public:
  WireCube() { setInterceptsMouseClicks(false, false); }

  /** Call at the GUI rate. `signal` is 0..1. */
  void update(bool powered, float signal);
  /** Jumps to this state, skipping the warm-up, e.g. as the editor opens. */
  void snapTo(bool powered, float signal);
  void paint(juce::Graphics &) override;

private:
  float heat = 0.0f, drive = 0.0f;
  float turn = 0.7f, travel = 0.0f; // where the cube and the floor have got to
};

/** The cabinet's speaker in wireframe: rings running down into the cone. Dims
    when the cab is off. */
class Speaker final : public juce::Component {
public:
  Speaker() { setInterceptsMouseClicks(false, false); }
  void setActive(bool);
  void paint(juce::Graphics &) override;

private:
  PaintCache cone;
  bool active = true;
};
} // namespace headroom::gui
