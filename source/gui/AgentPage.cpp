#include "AgentPage.h"

#include "HeadroomBinaryData.h"
#include "HeadroomLookAndFeel.h"
#include "Widgets.h"

namespace headroom::gui {
namespace {
using Entry = agent::Session::Entry;
using agent::AgentSettings;
using agent::ProviderId;

constexpr std::array<ProviderId, 3> providers{
    ProviderId::anthropic, ProviderId::openai, ProviderId::gemini};

const std::array<const char *, 3> examples{
    "A tone like [song or artist]. I have a [guitar].",
    "Make it creamier, and add a bit of fuzz.",
    "Set up a boost I can stomp on for the solo."};

/** Everything that moves runs off one clock, which wraps after this long: a
    whole number of turns of each, so nothing jumps when it does. */
constexpr float clockWrap = 36.0f;
constexpr float globeTurnSeconds = 18.0f;
constexpr float blinkSeconds = 1.2f;
constexpr float chaseStepsPerSecond = 7.5f;

juce::Image brandImage(const void *data, int size) {
  return juce::ImageCache::getFromMemory(data, size);
}

juce::String rightArrow() { return juce::String::charToString(0x2192); }

bool blinkOn(float clock) { return std::fmod(clock, blinkSeconds) < blinkSeconds * 0.62f; }

/** A misconverged tube's fringes round a shape, for whatever the pointer's over. */
void strokeFringes(juce::Graphics &g, const juce::Path &shape) {
  g.setColour(colours::magenta.withAlpha(0.5f));
  g.strokePath(shape, juce::PathStrokeType(1.0f), juce::AffineTransform::translation(-1.2f, 0.0f));
  g.setColour(colours::cyan.withAlpha(0.5f));
  g.strokePath(shape, juce::PathStrokeType(1.0f), juce::AffineTransform::translation(1.2f, 0.0f));
}

/** The slanted tab a panel's title starts with. */
void drawTab(juce::Graphics &g, juce::Rectangle<float> area, juce::Colour colour) {
  const auto tab = area.withSizeKeepingCentre(8.0f, 9.0f);
  juce::Path slant;
  slant.addQuadrilateral(tab.getX(), tab.getBottom(), tab.getX() + 3.5f, tab.getY(),
                         tab.getRight(), tab.getY(), tab.getRight() - 3.5f, tab.getBottom());
  fillNeon(g, slant, colour, 0.6f);
}

/** A warning sign in wireframe. */
void drawWarning(juce::Graphics &g, juce::Rectangle<float> area, juce::Colour colour) {
  const auto r = area.withSizeKeepingCentre(11.0f, 10.0f);
  juce::Path sign;
  sign.addTriangle(r.getCentreX(), r.getY(), r.getRight(), r.getBottom(), r.getX(), r.getBottom());
  strokeNeon(g, sign, colour, 1.2f, 0.5f);
  g.setColour(colour);
  g.fillRect(juce::Rectangle<float>(1.2f, 3.0f).withCentre(r.getCentre().translated(0.0f, 1.0f)));
  g.fillRect(juce::Rectangle<float>(1.2f, 1.2f).withCentre({r.getCentreX(), r.getBottom() - 1.8f}));
}

/** A source the agent cites, as a chip: its title, then an arrow out. */
class LinkChip final : public juce::HyperlinkButton {
public:
  static constexpr int height = 22;

  LinkChip(const juce::String &title, const juce::URL &link) : juce::HyperlinkButton(title, link) {
    setTooltip(link.toString(true));
  }
  int idealWidth() const {
    return juce::roundToInt(juce::GlyphArrangement::getStringWidth(font(), getButtonText())) + 38;
  }

protected:
  void paintButton(juce::Graphics &g, bool highlighted, bool down) override {
    using namespace colours;
    const bool hot = highlighted || down;
    const auto b = getLocalBounds().toFloat().reduced(0.5f);
    const auto shape = bevelBox(b, 5.0f);
    g.setColour(glass);
    g.fillPath(shape);
    if (hot) {
      g.setColour(accent().withAlpha(down ? 0.22f : 0.1f));
      g.fillPath(shape);
      strokeFringes(g, shape);
    }
    g.setColour(accent().withAlpha(hot ? 0.95f : 0.38f));
    g.strokePath(shape, juce::PathStrokeType(1.0f));

    auto area = b.reduced(10.0f, 0.0f);
    const auto box = area.removeFromRight(7.0f).withSizeKeepingCentre(7.0f, 7.0f);
    juce::Path out; // an arrow up and out to the right
    out.startNewSubPath(box.getBottomLeft());
    out.lineTo(box.getTopRight());
    out.startNewSubPath(box.getX() + 2.0f, box.getY());
    out.lineTo(box.getTopRight());
    out.lineTo(box.getRight(), box.getBottom() - 2.0f);
    g.setColour(hot ? colours::text : accent().withAlpha(0.8f));
    g.strokePath(out, juce::PathStrokeType(1.2f));
    area.removeFromRight(8.0f);
    g.setFont(font());
    g.setColour(hot ? colours::text : accent());
    g.drawText(getButtonText(), area, juce::Justification::centredLeft, true);
  }

private:
  static juce::Font font() { return body(12.0f); }
};

/** One of the prompts to start from: a numbered line on the screen. */
class ExampleButton final : public juce::Button {
public:
  ExampleButton(int number, const juce::String &prompt)
      : juce::Button(prompt), index(juce::String(number).paddedLeft('0', 2)) {
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
  }

  void paintButton(juce::Graphics &g, bool highlighted, bool down) override {
    using namespace colours;
    const bool hot = highlighted || down;
    const auto b = getLocalBounds().toFloat().reduced(0.5f);
    const auto shape = bevelBox(b, 6.0f);
    g.setColour(glass);
    g.fillPath(shape);
    if (hot) {
      g.setColour(accent().withAlpha(down ? 0.2f : 0.08f));
      g.fillPath(shape);
      strokeFringes(g, shape);
    }
    strokeNeon(g, shape, (hot ? accent() : cyan).withAlpha(hot ? 0.95f : 0.32f), 1.0f,
               hot ? 0.5f : 0.0f);

    auto area = b;
    const auto cell = area.removeFromLeft(42.0f);
    g.setFont(readout(19.0f));
    g.setColour(hot ? accent() : muted);
    g.drawText(index, cell, juce::Justification::centred, false);
    g.setColour(cyan.withAlpha(0.18f));
    g.fillRect(juce::Rectangle<float>(cell.getRight(), cell.getY() + 8.0f, 1.0f,
                                      cell.getHeight() - 16.0f));
    area.removeFromLeft(14.0f);

    const auto mark = area.removeFromRight(30.0f).getCentre();
    juce::Path chevron;
    chevron.startNewSubPath(mark.translated(-2.0f, -4.5f));
    chevron.lineTo(mark.translated(2.5f, 0.0f));
    chevron.lineTo(mark.translated(-2.0f, 4.5f));
    strokeNeon(g, chevron, hot ? accent() : mutedSoft, 1.5f, hot ? 0.8f : 0.0f);

    g.setFont(body(12.5f));
    g.setColour(hot ? colours::text : textSoft);
    g.drawText(getButtonText(), area, juce::Justification::centredLeft, true);
  }

private:
  juce::String index;
};
} // namespace

// --- the conversation --------------------------------------------------------
class AgentPage::Transcript final : public juce::Component {
public:
  explicit Transcript(AgentPage &p) : page(p) {
    int number = 0;
    for (const auto *example : examples) {
      auto *button = exampleButtons.add(new ExampleButton(++number, example));
      button->onClick = [this, example] { page.setComposer(example); };
      addChildComponent(button);
    }
    styleAsText(hint, body(13.0f), colours::textSoft);
    hint.setText("A song, an artist, or a change to what you have now. It looks up the "
                 "gear, finds captures on TONE3000 and sets the amp.");
    addChildComponent(hint);
    styleAsText(keyHint, body(12.5f), colours::yellow);
    addChildComponent(keyHint);
    gui::setPrimary(keysButton);
    keysButton.onClick = [this] { page.openKeys(); };
    addChildComponent(keysButton);
    refreshHint();
  }

  /** Under the hint, where to add a key while the provider has none, unless
      the notice under the screen is already saying so. */
  void refreshHint() {
    const auto provider = page.settings.getProvider();
    const auto text = page.settings.getApiKey(provider).isEmpty() &&
                              page.missing != Missing::apiKey
                          ? "First, add your " + AgentSettings::providerName(provider) +
                                " API key."
                          : juce::String();
    if (text == keyHint.getText())
      return;
    keyHint.setText(text);
    relayout();
  }

  void update(const std::vector<Entry> &entries, bool isBusy) {
    if (entries.size() < rows.size())
      rows.clear();
    for (size_t i = rows.size(); i < entries.size(); ++i)
      rows.push_back(makeRow(entries[i], i));
    for (size_t i = 0; i < entries.size(); ++i)
      if (rows[i]->revert != nullptr && entries[i].reverted) {
        rows[i]->revert->setEnabled(false);
        rows[i]->revert->setButtonText("reverted");
      }
    busy = isBusy;
    relayout();
  }

  /** Lays it out for this width, and at least this height while it's empty, to
      centre what's on it then. Sets this component's height to fit. */
  void layout(int width, int viewHeight) {
    layoutWidth = width;
    viewportHeight = viewHeight;
    relayout();
  }

  /** Repaints what moves, at the frame rate. */
  void animate() {
    if (rows.empty()) {
      repaint(globeArea);
      repaint(standbyLed.getSmallestIntegerContainer().expanded(12));
    } else if (!liveArea.isEmpty()) {
      repaint(liveArea.expanded(6));
      if (rows.back()->kind == Entry::Kind::activity)
        repaint(rows.back()->area.withWidth(16).expanded(6));
    }
  }

  void paint(juce::Graphics &g) override {
    if (rows.empty()) {
      paintEmpty(g);
      return;
    }
    for (size_t i = 0; i < rows.size(); ++i) {
      const auto &row = *rows[i];
      if (!g.clipRegionIntersects(row.area.expanded(8, 16)))
        continue;
      switch (row.kind) {
      case Entry::Kind::activity:
        paintActivity(g, i);
        break;
      case Entry::Kind::assistant:
        paintAnswer(g, row);
        break;
      case Entry::Kind::user:
      case Entry::Kind::error:
        paintCard(g, row);
        break;
      }
      g.setFont(caption());
      g.setColour(colours::muted);
      if (!row.linkCaptionArea.isEmpty())
        g.drawText(row.linksCaption, row.linkCaptionArea, juce::Justification::centredLeft,
                   false);
      if (!row.suggestionCaptionArea.isEmpty())
        g.drawText(row.suggestionsCaption, row.suggestionCaptionArea,
                   juce::Justification::centredLeft, false);
    }
    if (!liveArea.isEmpty() && g.clipRegionIntersects(liveArea.expanded(6)))
      paintLive(g);
  }

private:
  static constexpr int inset = 2;
  static constexpr int indent = 20; // from a row's marker to its text

  struct Row {
    Entry::Kind kind = Entry::Kind::activity;
    juce::TextEditor body;
    juce::OwnedArray<LinkChip> links, suggestions;
    juce::String linksCaption, suggestionsCaption;
    std::unique_ptr<juce::TextButton> revert;
    juce::Rectangle<int> area, captionArea, linkCaptionArea, suggestionCaptionArea;
  };

  std::unique_ptr<Row> makeRow(const Entry &entry, size_t index) {
    using namespace colours;
    auto row = std::make_unique<Row>();
    row->kind = entry.kind;
    const bool activity = entry.kind == Entry::Kind::activity;
    if (activity) {
      // As a log has it: what it did picked out ("loaded amp:"), then the
      // details.
      styleAsText(row->body, readout(17.0f), textSoft);
      const int colon = entry.text.indexOf(": ");
      const int split = colon > 0 && colon < 24 ? colon + 1 : 0;
      row->body.insertTextAtCaret(entry.text.substring(0, split));
      row->body.setColour(juce::TextEditor::textColourId, muted);
      row->body.insertTextAtCaret(entry.text.substring(split));
    } else {
      styleAsText(row->body, body(13.5f), text);
      row->body.setLineSpacing(1.12f);
      row->body.setText(entry.text);
    }
    addAndMakeVisible(row->body);

    const auto addLinks = [this](juce::OwnedArray<LinkChip> &chips,
                                 const std::vector<agent::Source> &sources) {
      for (const auto &source : sources) {
        if (!agent::isSafeLink(source.url))
          continue;
        auto title = source.title;
        if (title.length() > 48)
          title = title.substring(0, 47).trimEnd() + juce::String::charToString(0x2026);
        addAndMakeVisible(chips.add(new LinkChip(title, juce::URL(source.url))));
      }
    };
    addLinks(row->links, entry.sources);
    addLinks(row->suggestions, entry.suggestions);
    if (!activity)
      row->linksCaption = "SOURCES";
    row->suggestionsCaption = "GOOGLE SEARCH SUGGESTIONS";

    if (entry.before != nullptr) {
      row->revert = std::make_unique<juce::TextButton>("revert to before this");
      row->revert->setTooltip("Put the amp back how it was before this answer");
      row->revert->onClick = [this, index] { page.session.revert(index); };
      addAndMakeVisible(*row->revert);
    }
    return row;
  }

  void relayout() {
    const bool empty = rows.empty();
    const bool needsKey = empty && keyHint.getText().isNotEmpty();
    hint.setVisible(empty);
    keyHint.setVisible(needsKey);
    keysButton.setVisible(needsKey);
    for (auto *button : exampleButtons)
      button->setVisible(empty);
    if (layoutWidth <= 4 * inset)
      return;
    setSize(layoutWidth, empty ? layoutEmpty() : layoutRows());
    repaint();
  }

  // --- the empty screen ------------------------------------------------------
  /** A title card: the globe on the left, what to do on the right. Returns the
      height it needs. */
  int layoutEmpty() {
    constexpr int globeSide = 210, gap = 40, rowGap = 8, exampleHeight = 34;
    const int columnWidth = juce::jmin(520, layoutWidth - globeSide - gap - 2 * inset);
    const int left = juce::jmax(inset, (layoutWidth - globeSide - gap - columnWidth) / 2);
    const int x = left + globeSide + gap;

    // Placed once to measure it, then again centred.
    const auto place = [&](int y) {
      standbyArea = {x, y, columnWidth, 14};
      standbyLed = juce::Rectangle<float>(6.0f, 6.0f)
                       .withCentre({(float)x + 3.0f, (float)standbyArea.getCentreY()});
      y += 14 + 10;
      headlineArea = {x, y, columnWidth, 26};
      y += 26 + 10;
      y += placeText(hint, x, y, columnWidth) + 14;
      keyArea = {};
      if (keyHint.isVisible()) {
        constexpr int buttonWidth = 132, buttonHeight = 28, sign = 22;
        const int textHeight =
            placeText(keyHint, x + sign, y, columnWidth - sign - buttonWidth - 12);
        const int height = juce::jmax(buttonHeight, textHeight);
        keyHint.setTopLeftPosition(x + sign, y + (height - textHeight) / 2);
        keysButton.setBounds(x + columnWidth - buttonWidth, y + (height - buttonHeight) / 2,
                             buttonWidth, buttonHeight);
        keyArea = {x, y, columnWidth, height};
        y += height + 14;
      }
      tryArea = {x, y, columnWidth, 14};
      y += 14 + 8;
      for (auto *button : exampleButtons) {
        button->setBounds(x, y, columnWidth, exampleHeight);
        y += exampleHeight + rowGap;
      }
      return y - rowGap;
    };
    const int columnHeight = place(0);
    const int height = juce::jmax(viewportHeight, juce::jmax(columnHeight, globeSide) + 8);
    place((height - columnHeight) / 2);
    globeArea = {left, (height - globeSide) / 2, globeSide, globeSide};
    return height;
  }

  void paintEmpty(juce::Graphics &g) {
    using namespace colours;
    if (g.clipRegionIntersects(globeArea)) {
      const auto turn = juce::MathConstants<float>::twoPi * page.clock / globeTurnSeconds;
      drawGlobe(g, globeArea.toFloat().getCentre(), (float)globeArea.getWidth() * 0.36f, turn);
    }

    drawLed(g, standbyLed, blinkOn(page.clock), accent());
    g.setFont(caption());
    g.setColour(textSoft);
    g.drawText("STANDING BY", standbyArea.toFloat().withTrimmedLeft(16.0f),
               juce::Justification::centredLeft, false);

    if (g.clipRegionIntersects(headlineArea.expanded(12))) {
      const auto letters = textPath("DESCRIBE A SOUND", headline(19.0f, 0.03f),
                                    headlineArea.toFloat(), juce::Justification::centredLeft);
      juce::DropShadow(text.withAlpha(0.3f), 8, {}).drawForPath(g, letters);
      drawAberrated(g, letters, text, 1.3f);
    }

    if (!keyArea.isEmpty()) {
      drawWarning(g, keyArea.toFloat().withWidth(12.0f), yellow);
    }

    g.setFont(caption());
    g.setColour(muted);
    g.drawText("OR START FROM ONE OF THESE", tryArea, juce::Justification::centredLeft, false);
  }

  // --- the conversation ------------------------------------------------------
  int layoutRows() {
    const int x = inset, width = layoutWidth - 2 * inset;
    int y = 0;
    for (size_t i = 0; i < rows.size(); ++i) {
      // Each row leaves its own gap above it; the first, only a little.
      const int start = i == 0 ? y - 12 : y;
      y = layoutRow(*rows[i], i > 0 && rows[i - 1]->kind == Entry::Kind::activity, x, start,
                    width);
    }
    liveArea = {};
    if (busy) {
      y += 18;
      liveArea = {x, y, width, 16};
      y += 16;
    }
    return y + 14;
  }

  /** Places a row below `y`, and returns where it ends. */
  int layoutRow(Row &row, bool afterActivity, int x, int y, int width) {
    row.captionArea = {};
    switch (row.kind) {
    case Entry::Kind::activity: { // a line in the log, threaded on a rail
      y += afterActivity ? 3 : 16;
      const int top = y;
      y += placeText(row.body, x + indent, y, width - indent);
      y = layoutLinks(row, x + indent, y, width - indent, 5);
      row.area = {x, top, width, y - top};
      return y;
    }
    case Entry::Kind::assistant: { // the answer, behind a lit rail
      y += 22;
      const int top = y;
      row.captionArea = {x + indent, y, width - indent, 14};
      y += 14 + 6;
      y += placeText(row.body, x + indent, y, width - indent);
      y = layoutLinks(row, x + indent, y, width - indent, 12);
      if (row.revert != nullptr) {
        row.revert->setBounds(x + indent, y + 12, 180, 24);
        y += 12 + 24;
      }
      row.area = {x, top, width, y - top};
      return y;
    }
    case Entry::Kind::user:
    case Entry::Kind::error: {
      // A card: what you sent at the right, fitted to it; a problem across the
      // screen, with hazard stripes down its edge.
      const bool yours = row.kind == Entry::Kind::user;
      y += 22;
      constexpr int padX = 14, padTop = 9, padBottom = 12, captionHeight = 14;
      const int textLeft = padX + (yours ? 0 : 12);
      int cardWidth = yours ? juce::roundToInt((float)width * 0.72f) : width;
      if (yours) {
        float widest = 0.0f;
        for (const auto &line : juce::StringArray::fromLines(row.body.getText()))
          widest = juce::jmax(widest, juce::GlyphArrangement::getStringWidth(
                                          row.body.getFont(), line));
        cardWidth = juce::jmin(cardWidth, juce::jmax(150, (int)std::ceil(widest) + 6) +
                                              textLeft + padX);
      }
      const int cardX = yours ? x + width - cardWidth : x;
      const int textWidth = cardWidth - textLeft - padX;
      row.captionArea = {cardX + textLeft, y + padTop, textWidth, captionHeight};
      int bottom = row.captionArea.getBottom() + 5;
      bottom += placeText(row.body, cardX + textLeft, bottom, textWidth);
      bottom = layoutLinks(row, cardX + textLeft, bottom, textWidth, 8);
      row.area = {cardX, y, cardWidth, bottom + padBottom - y};
      return row.area.getBottom();
    }
    }
    return y;
  }

  /** Lays a row's links out left to right as chips, wrapping, under their
      captions. Returns where they end, or `y` if there are none. */
  static int layoutLinks(Row &row, int x, int y, int width, int gapAbove) {
    row.linkCaptionArea = row.suggestionCaptionArea = {};
    if (row.links.isEmpty() && row.suggestions.isEmpty())
      return y;
    y += gapAbove;
    constexpr int gap = 6;
    const auto flow = [&](juce::OwnedArray<LinkChip> &chips, juce::Rectangle<int> &captionArea,
                          const juce::String &captionText) {
      if (chips.isEmpty())
        return;
      if (captionText.isNotEmpty()) {
        captionArea = {x, y, width, 14};
        y += 14 + 5;
      }
      int cursor = x;
      for (auto *chip : chips) {
        const int chipWidth = juce::jmin(chip->idealWidth(), width);
        if (cursor > x && cursor + chipWidth > x + width) {
          cursor = x;
          y += LinkChip::height + gap;
        }
        chip->setBounds(cursor, y, chipWidth, LinkChip::height);
        cursor += chipWidth + gap;
      }
      y += LinkChip::height + 10;
    };
    flow(row.links, row.linkCaptionArea, row.linksCaption);
    flow(row.suggestions, row.suggestionCaptionArea, row.suggestionsCaption);
    return y - 10;
  }

  /** Sizes a text box to the height of its wrapped text, and returns that. It's
      measured at a height of one line, as an editor reports at least its own
      height. */
  static int placeText(juce::TextEditor &body, int x, int y, int width) {
    body.setBounds(x, y, juce::jmax(1, width), 1);
    const int height = juce::jmax(1, body.getTextHeight());
    body.setSize(juce::jmax(1, width), height);
    return height;
  }

  static float firstLineCentre(const juce::TextEditor &body) {
    return (float)body.getY() + body.getFont().getHeight() * 0.5f;
  }

  /** The caption a row starts with: a marker, then who it's from. */
  static void paintCaption(juce::Graphics &g, juce::Rectangle<int> area, Entry::Kind kind) {
    using namespace colours;
    auto r = area.toFloat();
    const auto marker = r.removeFromLeft(10.0f);
    r.removeFromLeft(7.0f);
    const auto font = caption();
    switch (kind) {
    case Entry::Kind::assistant:
    case Entry::Kind::activity: // the answer still coming
      drawTab(g, marker, accent());
      drawAberrated(g, "AGENT", font, r, juce::Justification::centredLeft, accent(), 0.8f);
      return;
    case Entry::Kind::user:
      g.setColour(textSoft);
      g.fillRect(marker.withSizeKeepingCentre(5.0f, 5.0f));
      g.setFont(font);
      g.drawText("YOU", r, juce::Justification::centredLeft, false);
      return;
    case Entry::Kind::error:
      drawWarning(g, marker.translated(0.0f, -0.5f), yellow);
      g.setColour(yellow);
      g.setFont(font);
      g.drawText("PROBLEM", r, juce::Justification::centredLeft, false);
      return;
    }
  }

  void paintActivity(juce::Graphics &g, size_t index) const {
    using namespace colours;
    const auto &row = *rows[index];
    const bool joinsAbove = index > 0 && rows[index - 1]->kind == Entry::Kind::activity;
    const bool joinsBelow =
        index + 1 < rows.size() && rows[index + 1]->kind == Entry::Kind::activity;
    const bool latest = busy && index + 1 == rows.size();
    const float x = (float)row.area.getX() + 6.0f, y = firstLineCentre(row.body);

    const float top = joinsAbove ? (float)row.area.getY() - 4.0f : y;
    const float bottom = joinsBelow ? (float)row.area.getBottom() : y;
    if (bottom > top) {
      g.setColour(blue.withAlpha(0.9f));
      g.fillRect(juce::Rectangle<float>(x - 0.5f, top, 1.0f, bottom - top));
    }
    const auto node = juce::Rectangle<float>(5.0f, 5.0f).withCentre({x, y});
    if (latest) {
      drawLed(g, node, blinkOn(page.clock), accent());
      return;
    }
    g.setColour(glass);
    g.fillRect(node);
    g.setColour(cyan.withAlpha(0.65f));
    g.drawRect(node, 1.0f);
  }

  /** The rail down an answer's left: lit at its head, then dimmer. */
  static void paintRail(juce::Graphics &g, juce::Rectangle<float> area) {
    using namespace colours;
    const auto rail = area.withX(area.getX() + 5.0f).withWidth(2.0f);
    g.setColour(accent().withAlpha(0.3f));
    g.fillRect(rail);
    juce::Path head;
    head.addRectangle(rail.withHeight(juce::jmin(rail.getHeight(), 16.0f)));
    fillNeon(g, head, accent(), 0.7f);
  }

  void paintAnswer(juce::Graphics &g, const Row &row) const {
    paintRail(g, row.area.toFloat());
    paintCaption(g, row.captionArea, Entry::Kind::assistant);
  }

  static void paintCard(juce::Graphics &g, const Row &row) {
    using namespace colours;
    const bool problem = row.kind == Entry::Kind::error;
    const auto colour = problem ? yellow : cyan;
    const auto area = row.area.toFloat().reduced(0.5f);
    const auto shape = bevelBox(area, 8.0f);
    g.setColour(colour.withAlpha(problem ? 0.06f : 0.05f));
    g.fillPath(shape);
    if (problem) {
      const juce::Graphics::ScopedSaveState save(g);
      g.reduceClipRegion(shape);
      const auto edge = area.withWidth(8.0f);
      drawStripes(g, edge, yellow.withAlpha(0.7f), 8.0f, 3.0f);
      g.setColour(yellow.withAlpha(0.45f));
      g.fillRect(edge.withX(edge.getRight()).withWidth(1.0f));
    }
    strokeNeon(g, shape, colour.withAlpha(problem ? 0.6f : 0.4f), 1.0f, problem ? 0.3f : 0.0f);
    paintCaption(g, row.captionArea, row.kind);
  }

  /** While it works: the start of an answer, with lights chasing along. */
  void paintLive(juce::Graphics &g) const {
    using namespace colours;
    const auto area = liveArea.toFloat();
    paintRail(g, area);
    const auto captionArea = liveArea.withTrimmedLeft(indent);
    paintCaption(g, captionArea, Entry::Kind::activity);

    constexpr int lights = 6;
    const float x0 = (float)captionArea.getX() + 17.0f +
                     juce::GlyphArrangement::getStringWidth(caption(), "AGENT") + 16.0f;
    const float span = (float)(2 * lights - 2);
    const float p = std::fmod(page.clock * chaseStepsPerSecond, span);
    const float lead = p < (float)(lights - 1) ? p : span - p;
    for (int k = 0; k < lights; ++k) {
      const auto light = juce::Rectangle<float>(6.0f, 6.0f)
                             .withCentre({x0 + 10.0f * (float)k, area.getCentreY()});
      const float level = juce::jlimit(0.0f, 1.0f, 1.0f - std::abs((float)k - lead) * 0.45f);
      if (level > 0.75f) {
        drawLed(g, light, true, accent());
      } else {
        g.setColour(accent().withAlpha(0.1f + 0.55f * level));
        g.fillRect(light);
        g.setColour(accent().withAlpha(0.45f));
        g.drawRect(light, 1.0f);
      }
    }
  }

  AgentPage &page;
  std::vector<std::unique_ptr<Row>> rows;
  bool busy = false;
  int layoutWidth = 0, viewportHeight = 0;
  juce::Rectangle<int> liveArea;

  // The empty screen.
  juce::TextEditor hint, keyHint;
  juce::TextButton keysButton{"open api keys"};
  juce::OwnedArray<ExampleButton> exampleButtons;
  juce::Rectangle<int> globeArea, standbyArea, headlineArea, keyArea, tryArea;
  juce::Rectangle<float> standbyLed;
};

// --- the page ----------------------------------------------------------------
AgentPage::AgentPage(agent::Session &s, agent::AgentSettings &a)
    : session(s), settings(a), transcript(std::make_unique<Transcript>(*this)) {
  for (const auto id : providers)
    providerBox.addItem(AgentSettings::providerName(id), (int)id + 1);
  providerBox.setTooltip("Whose model answers, on your own API key");
  providerBox.onChange = [this] {
    const auto id = (ProviderId)(providerBox.getSelectedId() - 1);
    if (id != settings.getProvider())
      settings.setProvider(id);
    refreshProviderControls();
  };
  addAndMakeVisible(providerBox);

  modelBox.setEditableText(true);
  modelBox.setTooltip("The model to use. Pick one, or type the name of any other");
  modelBox.onChange = [this] {
    const auto id = settings.getProvider();
    if (modelBox.getText().trim() != settings.getModel(id))
      settings.setModel(id, modelBox.getText());
  };
  addAndMakeVisible(modelBox);

  addAndMakeVisible(newChatButton);
  newChatButton.setTooltip("Start over. The amp stays as it is");
  newChatButton.onClick = [this] { session.newChat(); };

  addAndMakeVisible(browseButton);
  browseButton.setTooltip("Pick a tone yourself from the full TONE3000 catalog, "
                          "and the agent sets the amp up around it. This is the only "
                          "thing here that opens your browser.");
  browseButton.onClick = [this] {
    if (settings.get(AgentSettings::tone3000Key).isEmpty()) {
      showMissingKey(true);
      return;
    }
    if (settings.getApiKey(settings.getProvider()).isEmpty()) {
      showMissingKey(false); // the agent couldn't set the amp up around the pick
      return;
    }
    hideNotice();
    session.browseTone3000();
  };

  tone3000Logo.setImage(brandImage(HeadroomBinaryData::TONE3000logo_png,
                                   HeadroomBinaryData::TONE3000logo_pngSize),
                        juce::RectanglePlacement::centred);
  tone3000Logo.setInterceptsMouseClicks(false, false);
  addAndMakeVisible(tone3000Logo);
  t3kMark.setImage(brandImage(HeadroomBinaryData::T3Kmark_png, HeadroomBinaryData::T3Kmark_pngSize),
                   juce::RectanglePlacement::centred);
  t3kMark.setInterceptsMouseClicks(false, false);
  addAndMakeVisible(t3kMark);
  poweredBy.setButtonText("POWERED BY");
  poweredBy.setURL(juce::URL("https://www.tone3000.com"));
  poweredBy.setFont(caption(), false, juce::Justification::centredRight);
  poweredBy.setColour(juce::HyperlinkButton::textColourId, colours::muted);
  poweredBy.setTooltip("https://www.tone3000.com");
  addAndMakeVisible(poweredBy);

  viewport.setViewedComponent(transcript.get(), false);
  viewport.setScrollBarsShown(true, false);
  viewport.setScrollBarThickness(8);
  addAndMakeVisible(viewport);

  noticeLabel.setFont(body(12.5f));
  noticeLabel.setColour(juce::Label::textColourId, colours::yellow);
  noticeLabel.setMinimumHorizontalScale(1.0f);
  addChildComponent(noticeLabel);
  gui::setPrimary(noticeButton);
  noticeButton.onClick = [this] { openKeys(); };
  addChildComponent(noticeButton);

  composer.setMultiLine(true, true);
  composer.setReturnKeyStartsNewLine(false);
  composer.setFont(body(14.0f));
  composer.setBorder(juce::BorderSize<int>(0));
  composer.setScrollbarsShown(false);
  composer.setTextToShowWhenEmpty("Describe the sound you're after", colours::mutedSoft);
  composer.onReturnKey = [this] { sendComposed(); };
  gui::setConsole(composer);
  addAndMakeVisible(composer);

  gui::setPrimary(sendButton);
  sendButton.onClick = [this] {
    if (session.isBusy() &&
        !(session.isOnlyWaitingOnPlayer() && composer.getText().trim().isNotEmpty()))
      session.stop();
    else
      sendComposed();
  };
  addAndMakeVisible(sendButton);

  session.addChangeListener(this);
  settings.addChangeListener(this);
  refreshProviderControls();
  updateTranscript();
  refresh();
}

AgentPage::~AgentPage() {
  session.removeChangeListener(this);
  settings.removeChangeListener(this);
}

void AgentPage::resized() {
  auto b = getLocalBounds();

  auto top = b.removeFromTop(30);
  providerBox.setBounds(top.removeFromLeft(110));
  top.removeFromLeft(6);
  modelBox.setBounds(top.removeFromLeft(190));
  top.removeFromLeft(6);
  newChatButton.setBounds(top.removeFromLeft(84));

  auto browse = top.removeFromRight(244); // the label centres clear of the mark
  browseButton.setBounds(browse);
  t3kMark.setBounds(browse.withTrimmedLeft(10).withWidth(30).withSizeKeepingCentre(30, 10));
  top.removeFromRight(10);
  tone3000Logo.setBounds(top.removeFromRight(92).withSizeKeepingCentre(92, 14));
  top.removeFromRight(10);
  poweredBy.setBounds(top.removeFromRight(
      juce::roundToInt(juce::GlyphArrangement::getStringWidth(caption(), "POWERED BY")) + 8));
  b.removeFromTop(8);

  // One console: the screen, and the line you type into under it.
  panelArea = b;
  auto inside = panelArea.reduced(14, 0).withTrimmedTop(36).withTrimmedBottom(14);
  auto input = inside.removeFromBottom(44);
  sendButton.setBounds(input.removeFromRight(84));
  input.removeFromRight(8);
  composer.setBounds(input);
  composer.setIndents(30, juce::jmax(0, (input.getHeight() - juce::roundToInt(
                                                                composer.getFont().getHeight())) /
                                             2));
  inside.removeFromBottom(10);
  screenArea = inside;

  auto inner = screenArea.reduced(12, 8);
  noticeArea = {};
  if (noticeLabel.isVisible()) {
    noticeArea = inner.removeFromBottom(38);
    auto notice = noticeArea.withTrimmedLeft(8 + 12 + 18).withTrimmedRight(5);
    noticeButton.setBounds(notice.removeFromRight(128).withSizeKeepingCentre(128, 28));
    notice.removeFromRight(10);
    noticeLabel.setBounds(notice);
    inner.removeFromBottom(8);
  }
  viewport.setBounds(inner);
  // Room for the scroll bar whether it shows or not, so nothing moves when it
  // appears.
  transcript->layout(inner.getWidth() - viewport.getScrollBarThickness() - 4, inner.getHeight());
  viewport.setViewPosition(0, transcript->getHeight());
}

float AgentPage::statusWidth() const {
  if (statusText.isEmpty())
    return 0.0f;
  return juce::GlyphArrangement::getStringWidth(readout(16.0f), statusText) +
         (session.isBusy() ? 30.0f : 14.0f);
}

juce::Rectangle<float> AgentPage::statusArea() const {
  const float width = statusWidth();
  if (width <= 0.0f || headerRule.isEmpty())
    return {};
  return headerRule.withLeft(juce::jmax(headerRule.getX(), headerRule.getRight() - width));
}

void AgentPage::paint(juce::Graphics &g) {
  using namespace colours;
  // The frame's rule stops short of the status, so it's redrawn when the
  // status's room changes; the status itself is drawn fresh on top.
  const auto reserve = statusWidth();
  frame.draw(g, getLocalBounds(), (juce::int64)std::ceil(reserve),
             [this, reserve](juce::Graphics &c) {
               headerRule = drawPanel(c, panelArea, "tone agent", reserve);
               drawScreen(c, screenArea.toFloat());
             });

  if (auto status = statusArea(); !status.isEmpty()) {
    const bool busy = session.isBusy();
    if (busy)
      drawLed(g, juce::Rectangle<float>(6.0f, 6.0f).withCentre(
                     {status.getX() + 12.0f, status.getCentreY()}),
              blinkOn(clock), accent());
    g.setFont(readout(16.0f));
    g.setColour(busy ? accent() : muted);
    g.drawText(statusText, status, juce::Justification::centredRight, true);
  }

  if (!noticeArea.isEmpty()) {
    // A warning strip: hazard stripes at its edge, and a sign.
    const auto area = noticeArea.toFloat().reduced(0.5f);
    const auto shape = bevelBox(area, 6.0f);
    g.setColour(yellow.withAlpha(0.06f));
    g.fillPath(shape);
    {
      const juce::Graphics::ScopedSaveState save(g);
      g.reduceClipRegion(shape);
      drawStripes(g, area.withWidth(8.0f), yellow.withAlpha(0.7f), 8.0f, 3.0f);
    }
    strokeNeon(g, shape, yellow.withAlpha(0.6f), 1.0f, 0.3f);
    drawWarning(g, area.withTrimmedLeft(8.0f + 12.0f).withWidth(12.0f), yellow);
  }
}

void AgentPage::refresh() {
  const bool busy = session.isBusy();
  juce::String text;
  if (busy) {
    const auto detail = session.getStatus();
    text = (detail.isNotEmpty() ? detail : juce::String("working")) + " " +
           juce::String(juce::roundToInt(session.getBusySeconds())) + " s";
  } else {
    text = session.getLastUsage();
  }
  if (text != statusText) {
    statusText = text;
    repaint(panelArea.withHeight(44));
  }
  if (busy != wasBusy) {
    wasBusy = busy;
    sendButton.setButtonText(busy ? "stop" : "send");
    browseButton.setEnabled(!busy);
    newChatButton.setEnabled(!busy);
    updateTranscript();
    repaint(panelArea.withHeight(44));
  }
}

void AgentPage::animate(float secondsSinceLast) {
  clock += secondsSinceLast;
  if (clock >= clockWrap)
    clock -= clockWrap;
  transcript->animate();
  if (session.isBusy())
    if (const auto status = statusArea(); !status.isEmpty())
      repaint(status.withWidth(24.0f).expanded(4.0f).getSmallestIntegerContainer());
}

void AgentPage::pageShown() {
  refresh();
  if (isShowing())
    composer.grabKeyboardFocus();
}

void AgentPage::openKeys() {
  if (onOpenKeys)
    onOpenKeys();
}

void AgentPage::showMissingKey(bool tone3000) {
  missing = tone3000 ? Missing::tone3000 : Missing::apiKey;
  updateNotice();
}

void AgentPage::hideNotice() {
  missing = Missing::nothing;
  updateNotice();
}

void AgentPage::updateNotice() {
  if (missing == Missing::apiKey && settings.getApiKey(settings.getProvider()).isNotEmpty())
    missing = Missing::nothing;
  if (missing == Missing::tone3000 && settings.get(AgentSettings::tone3000Key).isNotEmpty())
    missing = Missing::nothing;

  const bool show = missing != Missing::nothing;
  if (show)
    noticeLabel.setText(
        missing == Missing::apiKey
            ? "No " + AgentSettings::providerName(settings.getProvider()) +
                  " API key yet. Add one in SETTINGS (top right) " + rightArrow() +
                  " API keys."
            : "Browsing TONE3000 needs your TONE3000 key. Add it in SETTINGS "
              "(top right) " + rightArrow() + " API keys, where you can also sign in.",
        juce::dontSendNotification);
  const bool changed = noticeLabel.isVisible() != show;
  noticeLabel.setVisible(show);
  noticeButton.setVisible(show && (bool)onOpenKeys);
  if (changed) {
    resized();
    repaint();
  }
  transcript->refreshHint();
}

void AgentPage::refreshProviderControls() {
  const auto id = settings.getProvider();
  providerBox.setSelectedId((int)id + 1, juce::dontSendNotification);
  modelBox.clear(juce::dontSendNotification);
  modelBox.addItemList(AgentSettings::suggestedModels(id), 1);
  modelBox.setText(settings.getModel(id), juce::dontSendNotification);
}

void AgentPage::setComposer(const juce::String &text) {
  composer.setText(text, false);
  composer.moveCaretToEnd();
  if (isShowing())
    composer.grabKeyboardFocus();
}

void AgentPage::sendComposed() {
  const auto text = composer.getText().trim();
  if (text.isEmpty())
    return;
  if (settings.getApiKey(settings.getProvider()).isEmpty()) {
    showMissingKey(false); // the text stays where it is
    return;
  }
  if (session.send(text)) {
    composer.clear();
    hideNotice();
  }
}

void AgentPage::updateTranscript() {
  // Follows the conversation down, unless someone's reading further up.
  const bool atEnd = viewport.getViewPositionY() + viewport.getViewHeight() >=
                     transcript->getHeight() - 24;
  transcript->update(session.getEntries(), session.isBusy());
  if (atEnd)
    viewport.setViewPosition(0, transcript->getHeight());
}

void AgentPage::changeListenerCallback(juce::ChangeBroadcaster *source) {
  if (source == &settings) {
    updateNotice();
    return;
  }
  updateTranscript();
  refresh();
}
} // namespace headroom::gui
