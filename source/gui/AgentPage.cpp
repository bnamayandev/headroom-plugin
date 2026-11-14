#include "AgentPage.h"

#include "EvilBinaryData.h"
#include "EvilLookAndFeel.h"
#include "Widgets.h"

namespace evil::gui {
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

juce::Image brandImage(const void *data, int size) {
  return juce::ImageCache::getFromMemory(data, size);
}
} // namespace

// --- the conversation --------------------------------------------------------
class AgentPage::Transcript final : public juce::Component {
public:
  explicit Transcript(AgentPage &p) : page(p) {
    for (const auto *example : examples) {
      auto *button = exampleButtons.add(new juce::TextButton(example));
      gui::setProse(*button);
      button->onClick = [this, example] { page.setComposer(example); };
      addChildComponent(button);
    }
    addChildComponent(hint);
    styleAsText(hint, sans(15.5f), colours::textSoft);
    hint.setJustification(juce::Justification::centred);
    refreshHint();
  }

  /** The hint says where to add a key while the provider has none. */
  void refreshHint() {
    juce::String text = "Describe a sound: a song, an artist, or a change to what you "
                        "have now. It looks up the gear, finds captures on TONE3000 and "
                        "sets the amp.";
    if (page.settings.getApiKey(page.settings.getProvider()).isEmpty())
      text += "\n\nFirst, add your " +
              AgentSettings::providerName(page.settings.getProvider()) +
              " API key in SETTINGS (top right) " + juce::String::charToString(0x2192) +
              " API keys.";
    if (text == hint.getText())
      return;
    hint.setText(text);
    layoutForWidth(getWidth());
  }

  void update(const std::vector<Entry> &entries) {
    if (entries.size() < rows.size())
      rows.clear();
    for (size_t i = rows.size(); i < entries.size(); ++i)
      rows.push_back(makeRow(entries[i], i));
    for (size_t i = 0; i < entries.size(); ++i)
      if (rows[i]->revert != nullptr && entries[i].reverted) {
        rows[i]->revert->setEnabled(false);
        rows[i]->revert->setButtonText("reverted");
      }
    const bool empty = entries.empty();
    hint.setVisible(empty);
    for (auto *button : exampleButtons)
      button->setVisible(empty);
    layoutForWidth(getWidth());
  }

  /** Sets this component's height to fit its contents at this width. */
  void layoutForWidth(int width) {
    constexpr int inset = 2;
    const int textWidth = width - 2 * inset;
    if (textWidth <= 0)
      return;
    int y = 2;

    if (rows.empty()) {
      y = 70;
      const int hintWidth = juce::jmin(textWidth, 560);
      y += placeText(hint, (width - hintWidth) / 2, y, hintWidth) + 18;
      for (auto *button : exampleButtons) {
        button->setBounds(juce::Rectangle<int>(0, y, width, 30)
                              .withSizeKeepingCentre(juce::jmin(textWidth, 520), 30));
        y += 36;
      }
    }

    for (const auto &rowPointer : rows) {
      auto &row = *rowPointer;
      if (row.kind == Entry::Kind::activity) {
        y += placeText(row.body, inset, y, textWidth) + 2;
      } else {
        y += 10;
        row.captionArea = {inset, y, textWidth, 15};
        y += 18;
        y += placeText(row.body, inset, y, textWidth) + 6;
      }
      y = layoutLinks(row.linkCaptionArea, row.linksCaption, row.links, y, inset, textWidth);
      y = layoutLinks(row.suggestionCaptionArea, row.suggestionsCaption, row.suggestions, y,
                      inset, textWidth);
      if (row.revert != nullptr) {
        row.revert->setBounds(inset, y + 2, 170, 24);
        y += 32;
      }
    }
    setSize(width, y + 8);
    repaint();
  }

  void paint(juce::Graphics &g) override {
    g.setFont(caption());
    for (const auto &row : rows) {
      if (!row->captionArea.isEmpty()) {
        g.setColour(row->kind == Entry::Kind::error ? colours::accent()
                                                    : colours::muted);
        g.drawText(row->kind == Entry::Kind::user        ? "YOU"
                   : row->kind == Entry::Kind::assistant ? "AGENT"
                                                         : "PROBLEM",
                   row->captionArea, juce::Justification::centredLeft, false);
      }
      g.setColour(colours::mutedSoft);
      if (!row->linkCaptionArea.isEmpty())
        g.drawText(row->linksCaption, row->linkCaptionArea,
                   juce::Justification::centredLeft, false);
      if (!row->suggestionCaptionArea.isEmpty())
        g.drawText(row->suggestionsCaption, row->suggestionCaptionArea,
                   juce::Justification::centredLeft, false);
    }
  }

private:
  struct Row {
    Entry::Kind kind = Entry::Kind::activity;
    juce::TextEditor body;
    juce::OwnedArray<juce::HyperlinkButton> links, suggestions;
    juce::String linksCaption, suggestionsCaption;
    std::unique_ptr<juce::TextButton> revert;
    juce::Rectangle<int> captionArea, linkCaptionArea, suggestionCaptionArea;
  };

  std::unique_ptr<Row> makeRow(const Entry &entry, size_t index) {
    auto row = std::make_unique<Row>();
    row->kind = entry.kind;
    const bool activity = entry.kind == Entry::Kind::activity;
    styleAsText(row->body, activity ? readout(12.0f) : sans(15.5f),
                activity ? colours::muted : colours::text);
    row->body.setText((activity ? juce::String::charToString(0x00b7) + " " : juce::String()) +
                      entry.text);
    addAndMakeVisible(row->body);

    const auto addLinks = [this](juce::OwnedArray<juce::HyperlinkButton> &buttons,
                                 const std::vector<agent::Source> &sources) {
      for (const auto &source : sources) {
        if (!agent::isSafeLink(source.url))
          continue;
        auto title = source.title;
        if (title.length() > 48)
          title = title.substring(0, 47).trimEnd() + juce::String::charToString(0x2026);
        auto *link = buttons.add(new juce::HyperlinkButton(title, juce::URL(source.url)));
        styleLink(*link);
        link->setTooltip(source.url);
        addAndMakeVisible(link);
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

  /** Sizes a text box to the height of its wrapped text, and returns that. It's
      measured at a height of one line, as an editor reports at least its own
      height. */
  static int placeText(juce::TextEditor &body, int x, int y, int width) {
    body.setBounds(x, y, width, 1);
    const int height = juce::jmax(1, body.getTextHeight());
    body.setSize(width, height);
    return height;
  }

  /** Lays links out left to right, wrapping, under a caption if they have one. */
  static int layoutLinks(juce::Rectangle<int> &captionArea, const juce::String &captionText,
                         juce::OwnedArray<juce::HyperlinkButton> &links, int y, int x,
                         int width) {
    captionArea = {};
    if (links.isEmpty())
      return y;
    if (captionText.isNotEmpty()) {
      captionArea = {x, y, width, 14};
      y += 15;
    }
    int cursor = x;
    constexpr int linkHeight = 18;
    const auto font = sans(13.0f, true);
    for (auto *link : links) {
      const int linkWidth =
          juce::roundToInt(juce::GlyphArrangement::getStringWidth(font, link->getButtonText())) + 14;
      if (cursor > x && cursor + linkWidth > x + width) {
        cursor = x;
        y += linkHeight;
      }
      link->setBounds(cursor, y, juce::jmin(linkWidth, width), linkHeight);
      cursor += linkWidth + 8;
    }
    return y + linkHeight + 4;
  }

  AgentPage &page;
  std::vector<std::unique_ptr<Row>> rows;
  juce::TextEditor hint;
  juce::OwnedArray<juce::TextButton> exampleButtons;
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
    hideNotice();
    session.browseTone3000();
  };

  tone3000Logo.setImage(brandImage(EvilBinaryData::TONE3000logo_png,
                                   EvilBinaryData::TONE3000logo_pngSize),
                        juce::RectanglePlacement::centred);
  tone3000Logo.setInterceptsMouseClicks(false, false);
  addAndMakeVisible(tone3000Logo);
  t3kMark.setImage(brandImage(EvilBinaryData::T3Kmark_png, EvilBinaryData::T3Kmark_pngSize),
                   juce::RectanglePlacement::centred);
  t3kMark.setInterceptsMouseClicks(false, false);
  addAndMakeVisible(t3kMark);
  poweredBy.setButtonText("powered by");
  poweredBy.setURL(juce::URL("https://www.tone3000.com"));
  styleLink(poweredBy, 12.0f);
  poweredBy.setColour(juce::HyperlinkButton::textColourId, colours::muted);
  poweredBy.setTooltip("https://www.tone3000.com");
  addAndMakeVisible(poweredBy);

  viewport.setViewedComponent(transcript.get(), false);
  viewport.setScrollBarsShown(true, false);
  viewport.setScrollBarThickness(8);
  viewport.getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, colours::line);
  addAndMakeVisible(viewport);

  noticeLabel.setFont(sans(14.0f));
  noticeLabel.setColour(juce::Label::textColourId, colours::accent());
  noticeLabel.setMinimumHorizontalScale(1.0f);
  addChildComponent(noticeLabel);
  gui::setPrimary(noticeButton);
  noticeButton.onClick = [this] {
    if (onOpenKeys)
      onOpenKeys();
  };
  addChildComponent(noticeButton);

  composer.setMultiLine(true, true);
  composer.setReturnKeyStartsNewLine(false);
  composer.setFont(sans(15.5f));
  composer.setBorder(juce::BorderSize<int>(0));
  composer.setIndents(10, 10);
  composer.setScrollbarsShown(false);
  composer.setTextToShowWhenEmpty("Describe the sound you're after", colours::mutedSoft);
  composer.onReturnKey = [this] { sendComposed(); };
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
  transcript->update(session.getEntries());
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

  auto browse = top.removeFromRight(216); // the label centres clear of the mark
  browseButton.setBounds(browse);
  t3kMark.setBounds(browse.withTrimmedLeft(10).withWidth(30).withSizeKeepingCentre(30, 10));
  top.removeFromRight(10);
  tone3000Logo.setBounds(top.removeFromRight(92).withSizeKeepingCentre(92, 14));
  top.removeFromRight(2);
  poweredBy.setBounds(top.removeFromRight(76));
  b.removeFromTop(8);

  auto bottom = b.removeFromBottom(44);
  sendButton.setBounds(bottom.removeFromRight(76));
  bottom.removeFromRight(8);
  composer.setBounds(bottom);
  b.removeFromBottom(8);

  panelArea = b;
  screenArea = panelArea.reduced(14, 0).withTrimmedTop(32).withTrimmedBottom(16);
  auto inner = screenArea.reduced(12, 8);
  if (noticeLabel.isVisible()) {
    auto notice = inner.removeFromBottom(36);
    noticeButton.setBounds(notice.removeFromRight(128).withSizeKeepingCentre(128, 28));
    notice.removeFromRight(10);
    noticeLabel.setBounds(notice);
    inner.removeFromBottom(6);
  }
  viewport.setBounds(inner);
  transcript->layoutForWidth(viewport.getMaximumVisibleWidth() - 4);
  viewport.setViewPosition(0, transcript->getHeight());
}

void AgentPage::paint(juce::Graphics &g) {
  using namespace colours;
  const auto font = readout(12.0f);
  const auto width = statusText.isEmpty()
                         ? 0.0f
                         : juce::GlyphArrangement::getStringWidth(font, statusText) + 14.0f;
  frame.draw(g, getLocalBounds(), (juce::int64)width * 2,
             [this, width](juce::Graphics &c) {
               headerRule = drawPanel(c, panelArea, "tone agent", width);
               drawRecess(c, screenArea.toFloat(), 4.0f);
             });
  if (statusText.isEmpty())
    return;
  g.setFont(font);
  g.setColour(session.isBusy() ? accent() : muted);
  g.drawText(statusText,
             headerRule.withLeft(juce::jmax(headerRule.getX(), headerRule.getRight() - width)),
             juce::Justification::centredRight, true);
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
  }
}

void AgentPage::pageShown() {
  refresh();
  if (isShowing())
    composer.grabKeyboardFocus();
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
                  " API key yet. Add one in SETTINGS (top right) " +
                  juce::String::charToString(0x2192) + " API keys."
            : "Browsing TONE3000 needs your TONE3000 key. Add it in SETTINGS "
              "(top right) " + juce::String::charToString(0x2192) +
                  " API keys, where you can also sign in.",
        juce::dontSendNotification);
  const bool changed = noticeLabel.isVisible() != show;
  noticeLabel.setVisible(show);
  noticeButton.setVisible(show && (bool)onOpenKeys);
  if (changed)
    resized();
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

void AgentPage::changeListenerCallback(juce::ChangeBroadcaster *source) {
  if (source == &settings) {
    updateNotice();
    return;
  }
  const bool atEnd = viewport.getViewPositionY() + viewport.getViewHeight() >=
                     transcript->getHeight() - 24;
  transcript->update(session.getEntries());
  if (atEnd) // don't drag someone away from reading further up
    viewport.setViewPosition(0, transcript->getHeight());
  refresh();
}
} // namespace evil::gui
