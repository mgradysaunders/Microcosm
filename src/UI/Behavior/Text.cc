#include "Microcosm/UI/Behavior/Text"
#include "Microcosm/UI/Text"

namespace mi::ui {

std::string TextBehavior::selection() const {
  std::string result;
  for (size_t i = min(mCursorA, mCursorB); i < max(mCursorA, mCursorB); i++) result += UTF8Encoding{mBuffer[i]};
  return result;
}

void TextBehavior::selectWord() {
  auto isWord = [](char32_t codepoint) {
    return ('a' <= codepoint && codepoint <= 'z') || ('A' <= codepoint && codepoint <= 'Z') ||
           ('0' <= codepoint && codepoint <= '9') || codepoint == '_';
  };
  mCursorA = mCursorB;
  while (mCursorA > 0 && isWord(mBuffer[mCursorA - 1])) --mCursorA;
  while (mCursorB < mBuffer.size() && isWord(mBuffer[mCursorB])) ++mCursorB;
}

void TextBehavior::selectLine() { pressHome(/*selecting=*/false), pressEnd(/*selecting=*/true); }

void TextBehavior::eraseSelection() {
  if (mCursorA > mCursorB) std::swap(mCursorA, mCursorB);
  if (mCursorA < mCursorB) {
    mBuffer.erase(mBuffer.begin() + mCursorA, mBuffer.begin() + mCursorB);
    mCursorB = mCursorA;
  }
}

void TextBehavior::insert(char32_t codepoint) {
  eraseSelection();
  mBuffer.insert(mBuffer.begin() + mCursorB++, codepoint);
  mCursorA = mCursorB;
}

void TextBehavior::insert(const std::string &text) {
  for (char32_t codepoint : UTF8DecodeRange(text)) insert(codepoint);
}

void TextBehavior::pressLeft(bool selecting) {
  mCursorB -= mCursorB > 0 ? 1 : 0;
  if (!selecting) mCursorA = mCursorB;
}

void TextBehavior::pressRight(bool selecting) {
  mCursorB += mCursorB < mBuffer.size() ? 1 : 0;
  if (!selecting) mCursorA = mCursorB;
}

void TextBehavior::pressUp(bool selecting, const Text &text) {
  assert(mBuffer.size() == text.size());
  if (!mBuffer.empty() && mCursorB > 0) {
    Vector2f cursor = text.cursorCentered(&text[mCursorB]) - Vector2f(0, text.lineAdvance());
    const Text::Letter *letter = text.hoverLine(cursor[1], /*clampLineNo=*/false).hoverLetter(cursor[0]);
    mCursorB = letter ? letter - &text[0] : 0;
  }
  if (!selecting) mCursorA = mCursorB;
}

void TextBehavior::pressDown(bool selecting, const Text &text) {
  assert(mBuffer.size() == text.size());
  if (!mBuffer.empty() && mCursorB < mBuffer.size()) {
    Vector2f cursor = text.cursorCentered(&text[mCursorB]) + Vector2f(0, text.lineAdvance());
    const Text::Letter *letter = text.hoverLine(cursor[1], /*clampLineNo=*/false).hoverLetter(cursor[0]);
    mCursorB = letter ? letter - &text[0] : mBuffer.size();
  }
  if (!selecting) mCursorA = mCursorB;
}

void TextBehavior::pressHome(bool selecting) {
  while (mCursorB > 0 && mBuffer[mCursorB - 1] != '\n') --mCursorB;
  if (!selecting) mCursorA = mCursorB;
}

void TextBehavior::pressEnd(bool selecting) {
  while (mCursorB < mBuffer.size() && mBuffer[mCursorB] != '\n') ++mCursorB;
  if (!selecting) mCursorA = mCursorB;
}

void TextBehavior::pressBackspace() {
  if (hasSelection()) {
    eraseSelection();
  } else if (!mBuffer.empty() && mCursorB > 0) {
    mBuffer.erase(mBuffer.begin() + --mCursorB);
    mCursorA = mCursorB;
  }
}

void TextBehavior::pressDelete() {
  if (hasSelection()) { // Has selection?
    eraseSelection();
  } else if (!mBuffer.empty() && mCursorB < mBuffer.size()) {
    mBuffer.erase(mBuffer.begin() + mCursorB);
  }
}

void TextBehavior::think(Context &ctx, Text &text, bool ignoreMouse, bool ignoreKeyboard) {
  auto &mouse = ctx.mouse;
  auto &keyboard = ctx.keyboard;
  mBuffer.clear();
  mBuffer.reserve(text.size());
  for (const Text::Letter &letter : text) mBuffer.emplace_back(letter.codepoint);

  // Handle mouse.
  if (!ignoreMouse) {
    Vector2f cursor = ctx.mousePosition();
    const Text::Letter *letter = text.hoverLine(cursor[1], /*clampLineNo=*/mMouseActive).hoverLetter(cursor[0]);
    if (letter) mouse.setCursorIcon(Mouse::CursorIcon::Text);
    const auto &mouseButton = mouse[Mouse::Button::L];
    if (!mMouseActive) {
      if (mouseButton.isJustDown()) {
        if (letter) {
          mCursorB = letter - text.begin();
          mCursorA = mCursorB;
          if (mouseButton.clickOrder == 0) // Single click?
            mMouseActive = true;
          else if (mouseButton.clickOrder == 1) // Double click?
            selectWord();
          else if (mouseButton.clickOrder == 2) // Triple click?
            selectLine();
          else if (mouseButton.clickOrder == 3) { // Quadruple click?
            mCursorA = 0;
            mCursorB = mBuffer.size();
          }
        } else {
          mCursorA = mCursorB;
        }
      }
    } else {
      if (letter) mCursorB = letter - text.begin();
      if (mouseButton.isJustUp()) mMouseActive = false;
    }
  } else {
    mMouseActive = false;
  }

  // Handle keyboard.
  if (!ignoreKeyboard && !mMouseActive) {
    if (mEditable) {
      // Navigation
      const bool shift = keyboard.isDown(Keymod::Shift);
      if (keyboard.isJustDownWithRepeats(Key::Right)) {
        pressRight(shift);
      } else if (keyboard.isJustDownWithRepeats(Key::Left)) {
        pressLeft(shift);
      } else if (keyboard.isJustDownWithRepeats(Key::Up)) {
        pressUp(shift, text);
      } else if (keyboard.isJustDownWithRepeats(Key::Down)) {
        pressDown(shift, text);
      } else if (keyboard.isJustDown(Key::Home)) {
        pressHome(shift);
      } else if (keyboard.isJustDown(Key::End)) {
        pressEnd(shift);
      } else if (keyboard.isJustDown(Key::Esc)) {
        deselect();
      } else if (keyboard.isJustDownWithRepeats(Key::Backspace)) {
        pressBackspace();
        text.refresh(convertBackToString());
      } else if (keyboard.isJustDownWithRepeats(Key::Delete)) {
        pressDelete();
        text.refresh(convertBackToString());
      } else if (keyboard.isJustDown(Key::Return)) {
        insert('\n');
        text.refresh(convertBackToString());
      } else if (keyboard.isJustDown(Key::Tab)) {
        insert('\t');
        text.refresh(convertBackToString());
      } else if (keyboard.isHotKeyActivated(Keymod::Ctrl, Key::X)) {
        pressCtrlX(ctx);
        text.refresh(convertBackToString());
      } else if (keyboard.isHotKeyActivated(Keymod::Ctrl, Key::C)) {
        pressCtrlC(ctx);
      } else if (keyboard.isHotKeyActivated(Keymod::Ctrl, Key::V)) {
        pressCtrlV(ctx);
        text.refresh(convertBackToString());
      } else if (!keyboard.inputText.empty()) {
        insert(keyboard.inputText);
        text.refresh(convertBackToString());
      }
    } else {
      // Even if not editable, still need to listen for Ctrl+C to copy.
      if (keyboard.isHotKeyActivated(Keymod::Ctrl, Key::C)) {
        pressCtrlC(ctx);
      }
    }
  }
}

void TextBehavior::convertBackToString(std::string &result) const {
  result.clear();
  result.reserve(mBuffer.size());
  for (char32_t codepoint : mBuffer) result += UTF8Encoding{codepoint};
}

} // namespace mi::ui
