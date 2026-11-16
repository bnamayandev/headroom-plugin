#pragma once

#include "Neon.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace headroom::gui {
/** The neons whatever's live can glow in, chosen in settings: the palette's
    brightest three. */
enum class Neon { magenta, yellow, cyan };

namespace colours {
// A 1986 network's computer, on a black tube: the palette.
inline const juce::Colour black{0xff05050a};   // the screen everything's on
inline const juce::Colour cyan{0xff00f0ff};    // the line work: frames, labels, readouts
inline const juce::Colour magenta{0xffff2bd6}; // what's live, unless settings say otherwise
inline const juce::Colour yellow{0xfff5ff3b};  // data picked out: presets, measurements
inline const juce::Colour blue{0xff1a3cff};    // grids, scales and fills behind things

// What each part of the editor takes from it.
inline const juce::Colour chassis = black;        // behind the panels
inline const juce::Colour background = black;     // behind any letterboxing
inline const juce::Colour panel{0xff07071a};      // a panel's face
inline const juce::Colour glass{0xff020208};      // screens and fields
inline const juce::Colour text{0xffeafdff};       // phosphor white
inline const juce::Colour textSoft{0xff00d4e2};   // values, secondary text
inline const juce::Colour muted{0xff16929e};      // labels, meta: cyan turned down
inline const juce::Colour mutedSoft{0xff10596a};  // faintest text, idle marks
inline const juce::Colour line = cyan.withAlpha(0.45f);     // frames
inline const juce::Colour lineSoft = cyan.withAlpha(0.14f); // dividers

/** The neon whatever's live glows in: lights, lit scales, values, the primary
    action. */
constexpr Neon defaultNeon = Neon::magenta;
juce::Colour neon(Neon);
/** The chosen neon. */
juce::Colour accent();
Neon getNeon();
/** Message thread only. Open editors repaint through gui::Settings, which is
    what normally calls this. */
void setNeon(Neon);
} // namespace colours

/** The embedded faces, shared by all open editors. */
struct Typefaces {
  Typefaces();
  juce::Typeface::Ptr headline, caps, logo, readout, body;
};

/** Michroma: the names of things, in capitals. Amp, cab, pedals, the note.
    Sizes are in points (CSS px); tracking is JUCE's kerning factor, a fraction
    of the line height. */
juce::Font headline(float size, float tracking = 0.0f);
/** Orbitron Bold: labels, tabs and buttons. Always upper case, small, and
    tracked out. */
juce::Font caps(float size, float tracking = 0.12f);
/** Orbitron Black, for the logo alone. */
juce::Font logoFont(float size);
/** VT323: every number and value, as a terminal shows them. It's small for its
    size: 19 matches most faces at 14. */
juce::Font readout(float size, float tracking = 0.0f);
/** IBM Plex Mono: prose, anything read as a sentence. */
juce::Font body(float size);
/** Small tracked label, e.g. section captions. */
inline juce::Font caption() { return caps(9.5f, 0.2f); }
/** A navigation tab's label. */
inline juce::Font tabFont() { return caps(10.5f, 0.12f); }

/** Draws a TextButton as the primary action, picked out in the accent. */
void setPrimary(juce::Button &);
/** Draws a TextButton as a navigation tab: lit solid while selected (its
    toggle state). */
void setTab(juce::Button &);
/** Draws a TextButton's text as written rather than as a label, for buttons
    holding a sentence. */
void setProse(juce::Button &);
/** Marks a slider as one of the small knobs on a pedal: smaller value text. */
void setCompact(juce::Slider &);
/** Draws a Label's text with the colour fringes of a misconverged tube, and a
    glow: for headlines that change, e.g. the loaded model's name. */
void setHeadline(juce::Label &);

class HeadroomLookAndFeel final : public juce::LookAndFeel_V4 {
public:
  HeadroomLookAndFeel();

  /** Re-applies the colour ids that hold the accent; call after it changes. */
  void refreshAccent();

  void drawRotarySlider(juce::Graphics &, int x, int y, int width, int height,
                        float sliderPos, float startAngle, float endAngle,
                        juce::Slider &) override;
  void drawLinearSlider(juce::Graphics &, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        juce::Slider::SliderStyle, juce::Slider &) override;
  void drawToggleButton(juce::Graphics &, juce::ToggleButton &,
                        bool highlighted, bool down) override;
  void drawButtonBackground(juce::Graphics &, juce::Button &,
                            const juce::Colour &, bool highlighted,
                            bool down) override;
  void drawButtonText(juce::Graphics &, juce::TextButton &, bool highlighted,
                      bool down) override;
  void drawComboBox(juce::Graphics &, int width, int height, bool isButtonDown,
                    int buttonX, int buttonY, int buttonW, int buttonH,
                    juce::ComboBox &) override;
  void drawLabel(juce::Graphics &, juce::Label &) override;
  void drawPopupMenuBackground(juce::Graphics &, int width,
                               int height) override;
  void drawPopupMenuItem(juce::Graphics &, const juce::Rectangle<int> &area,
                         bool isSeparator, bool isActive, bool isHighlighted,
                         bool isTicked, bool hasSubMenu, const juce::String &text,
                         const juce::String &shortcutKeyText, const juce::Drawable *icon,
                         const juce::Colour *textColour) override;
  void drawCallOutBoxBackground(juce::CallOutBox &, juce::Graphics &,
                                const juce::Path &, juce::Image &) override;
  int getCallOutBoxBorderSize(const juce::CallOutBox &) override { return 14; }
  float getCallOutBoxCornerSize(const juce::CallOutBox &) override { return 0.0f; }
  void drawAlertBox(juce::Graphics &, juce::AlertWindow &, const juce::Rectangle<int> &textArea,
                    juce::TextLayout &) override;
  void drawTextEditorOutline(juce::Graphics &, int width, int height,
                             juce::TextEditor &) override;
  void drawScrollbar(juce::Graphics &, juce::ScrollBar &, int x, int y, int width, int height,
                     bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                     bool isMouseOver, bool isMouseDown) override;
  void drawCornerResizer(juce::Graphics &, int w, int h, bool isMouseOver,
                         bool isMouseDragging) override;
  void drawTooltip(juce::Graphics &, const juce::String &text, int width,
                   int height) override;
  juce::Rectangle<int> getTooltipBounds(const juce::String &text,
                                        juce::Point<int> screenPos,
                                        juce::Rectangle<int> parentArea) override;
  juce::Font getTextButtonFont(juce::TextButton &, int buttonHeight) override;
  juce::Font getComboBoxFont(juce::ComboBox &) override;
  juce::Font getPopupMenuFont() override;
  juce::Font getLabelFont(juce::Label &) override;
  juce::Font getAlertWindowTitleFont() override;
  juce::Font getAlertWindowMessageFont() override;
  juce::Font getAlertWindowFont() override;
  juce::Label *createSliderTextBox(juce::Slider &) override;

private:
  juce::SharedResourcePointer<Typefaces> typefaces; // keeps them loaded
};
} // namespace headroom::gui
