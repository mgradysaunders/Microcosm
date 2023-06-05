#include "Microcosm/UI/Text"
#include "Microcosm/UI/Color"

namespace mi::ui {

void Text::clear() noexcept { mLetters.clear(), mLines.clear(); }

void Text::refresh(std::string_view text) {
  // The standard 8 terminal colors, in normal and light variants. The specific colors used
  // vary substantially from terminal to terminal. These colors follow Terminal.app from Mac.
  // (Disclaimer: I use Ubuntu and have never really developed on Mac, but I am not the biggest
  // fan of the default Ubuntu terminal colors, so I looked at the alternatives on Wikipedia.
  // See here, https://en.wikipedia.org/wiki/ANSI_escape_code.)
  static const Vector4b TerminalColors[8][2] = {
    {Vector4b(0x00, 0x00, 0x00, 0xFF), Vector4b(0x81, 0x83, 0x83, 0xFF)},  // Black
    {Vector4b(0xC2, 0x36, 0x21, 0xFF), Vector4b(0xFC, 0x39, 0x1F, 0xFF)},  // Red
    {Vector4b(0x25, 0xBC, 0x24, 0xFF), Vector4b(0x31, 0xE7, 0x22, 0xFF)},  // Green
    {Vector4b(0xAD, 0xAD, 0x27, 0xFF), Vector4b(0xEA, 0xEC, 0x23, 0xFF)},  // Yellow
    {Vector4b(0x49, 0x2E, 0xFF, 0xFF), Vector4b(0x58, 0x33, 0xFF, 0xFF)},  // Blue
    {Vector4b(0xD3, 0x38, 0xD3, 0xFF), Vector4b(0xF9, 0x35, 0xF8, 0xFF)},  // Magenta
    {Vector4b(0x33, 0xBB, 0xC8, 0xFF), Vector4b(0x14, 0xF0, 0xF0, 0xFF)},  // Cyan
    {Vector4b(0xCB, 0xCC, 0xCD, 0xFF), Vector4b(0xE9, 0xEB, 0xEb, 0xFF)}}; // White

  mLetters.clear();
  mLetters.reserve(text.size());
  mLines.clear();
  mLines.reserve(32);
  float cursor{0};
  float upperX{0};
  char32_t glyphY{Font::None};
  char32_t glyphZ{Font::None};
  auto addLine = [&] {
    cursor = 0;
    glyphY = Font::None;
    glyphZ = Font::None;
    mLines.emplace_back(mLines.empty() ? 0 : mLines.back().second, mLetters.size());
  };
  Vector4b foreground{0xFF, 0xFF, 0xFF, 0xFF};
  Vector4b background{0x00, 0x00, 0x00, 0x00};
  Vector4b foregroundDefault = foreground;
  Vector4b backgroundDefault = background;
  Emphasis emphasis{};
  std::vector<int> escValues;
  UTF8DecodeRange textRange{text};
  for (auto itr = textRange.begin(); itr != textRange.end();) {
    char32_t codepoint = *itr;
    // Is this *NOT* the beginning of an ANSI escape sequence? This if-check is inverted because *NOT*
    // parsing ANSI escape sequences is the dominant code-path in this loop, and the escape-sequence parsing
    // is more involved than ordinary letter initialization. See the else clause further below for how the
    // escape sequences are handled.
    if (codepoint != '\033') [[likely]] {
      Letter &letter = mLetters.emplace_back();
      letter.codepoint = codepoint;
      letter.left = cursor;
      letter.right = cursor;
      letter.baseline = mLineAdvance * float(mLines.size() - 0.333f * emphasis.script);
      letter.baselinePlusAscent = letter.baseline + metrics().ascent;
      letter.baselinePlusDescent = letter.baseline + metrics().descent;
      letter.foreground = foreground;
      letter.background = background;
      letter.emphasis = emphasis;
      if (codepoint == '\n') {
        addLine();
      } else if (codepoint == '\t') {
        cursor = mFont->tabWidth * (1 + floor(cursor / mFont->tabWidth));
        letter.left = cursor;
        letter.right = cursor;
        glyphY = glyphZ = Font::None;
      } else {
        if (emphasis.hide) codepoint = '*';
        if (glyphZ = mFont->codepointToGlyph(codepoint); glyphZ != Font::None) {
          letter.left = cursor += mFont->kerning(glyphY, glyphZ);
          letter.right = cursor += mFont->glyphs[glyphZ].advance;
          letter.glyph = &mFont->glyphs[glyphZ];
          glyphY = glyphZ;
          upperX = max(upperX, cursor);
        } else {
          glyphY = glyphZ = Font::None;
        }
      }
      ++itr;
    } else {
      // Try to parse the escape sequence. It should begin with the escape character (we
      // already know it does by the above if-check), then it should have an open square
      // bracket. Then it should feature a semi-colon separated list of decimal integers,
      // and end with the letter 'm'.
      escValues.clear();
      auto escItr = itr;
      auto escItrEnd = textRange.end();
      auto ParseEscSequence = [&] {
        // Must begin with the escape character and open square bracket.
        if (*escItr++ != '\033') [[unlikely]]
          return false;
        if (*escItr++ != '[') [[unlikely]]
          return false;
        // Loop to parse semi-colon separated list of integers.
        while (escItr != escItrEnd) {
          // Parse the next integer.
          int escValue = 0;
          while (escItr != escItrEnd) {
            int dec = *escItr - '0';
            if (dec > 9 || dec < 0) break;
            escValue *= 10;
            escValue += dec;
            ++escItr;
          }
          // Hit the end of the string before the letter 'm'?
          if (escItr == escItrEnd) [[unlikely]] {
            break; // Fail!
          } else {
            escValues.push_back(escValue);
            if (*escItr == ';') { // Semi-colon? Continue to next integer.
              ++escItr;
            } else if (*escItr == 'm') {
              ++escItr;
              return true; // Done.
            } else {
              break; // Fail!
            }
          }
        }
        return false;
      };
      if (ParseEscSequence()) {
        auto escValueItr = escValues.begin();
        auto escValueItrEnd = escValues.end();
        auto ProcessColor = [&](int escValue, Vector4b &color, const Vector4b &defaultColor) {
          if (0 <= escValue && escValue < 8) {
            color = TerminalColors[escValue][0];
          } else if (escValue == 8) {
            ++escValueItr;
            uint8_t r = *escValueItr++;
            uint8_t g = *escValueItr++;
            uint8_t b = *escValueItr++;
            color = Vector4b(r, g, b, 0xFF);
          } else if (escValue == 9) {
            color = defaultColor;
          } else if (60 <= escValue && escValue < 68) {
            color = TerminalColors[escValue - 60][1];
          }
        };
        while (escValueItr < escValueItrEnd) {
          int escValue = *escValueItr++;
          if (escValue == 0) { // Reset everything.
            emphasis = {};
            foreground = foregroundDefault;
            background = backgroundDefault;
          } else if (1 <= escValue && escValue < 10) { // Enable styling.
            switch (escValue) {
            default: break;
            case 1: emphasis.bold = true; break;
            case 2: emphasis.faint = true; break;
            case 3: emphasis.italic = true; break;
            case 4: emphasis.underline = true; break;
            case 5: emphasis.blink = true; break;
            case 8: emphasis.hide = true; break;
            case 9: emphasis.strike = true; break;
            }
          } else if (20 <= escValue && escValue < 30) { // Reset styling.
            switch (escValue) {
            default: break;
            case 22: emphasis.bold = emphasis.faint = false; break;
            case 23: emphasis.italic = false; break;
            case 24: emphasis.underline = false; break;
            case 25: emphasis.blink = false; break;
            case 28: emphasis.hide = false; break;
            case 29: emphasis.strike = false; break;
            }
          } else if ((30 <= escValue && escValue < 40) || (90 <= escValue && escValue < 100)) {
            ProcessColor(escValue - 30, foreground, foregroundDefault);
          } else if ((40 <= escValue && escValue < 50) || (100 <= escValue && escValue < 110)) {
            ProcessColor(escValue - 40, background, backgroundDefault);
          } else if (escValue == 73) { // Superscript.
            emphasis.script = +1;
          } else if (escValue == 74) { // Subscript.
            emphasis.script = -1;
          } else if (escValue == 75) { // No script.
            emphasis.script = 0;
          }
        }
        itr = escItr;
      } else {
        ++itr;
      }
    }
  }
  // Add terminating line if necessary.
  if (mLines.empty() || mLines.back().second != mLetters.size()) addLine();
  // Set the content rectangle.
  mRect = {{0, mFont->metrics.ascent}, {upperX, mLineAdvance * (mLines.size() - 1) + mFont->metrics.descent}};
  // Guarantee that all of the letters agree on their left and right side positions. This can be imperfect at
  // this point because of kerning.
  for (const auto &[lineFrom, lineTo] : mLines) {
    for (size_t i = lineFrom; i + 1 < lineTo; i++) {
      mLetters[i].right = mLetters[i + 1].left;
    }
  }
}

Text::LineView Text::line(int lineNo) const noexcept {
  LineView lineView;
  lineView.lineNo = lineNo;
  lineView.baseline = mLineAdvance * lineNo;
  lineView.baselinePlusAscent = lineView.baseline + mFont->metrics.ascent;
  lineView.baselinePlusDescent = lineView.baseline + mFont->metrics.descent;
  if (lineNo >= 0 && size_t(lineNo) < mLines.size())
    lineView.letters = {
      mLetters.data() + mLines[lineNo].first, //
      mLetters.data() + mLines[lineNo].second};
  return lineView;
}

Text::LineView Text::hoverLine(float cursorY, bool clampLineNo) const noexcept {
  int lineNo = int(ceil(cursorY / mLineAdvance));
  if (clampLineNo) {
    lineNo = max(lineNo, 0);
    lineNo = min(lineNo, int(numLines()));
  }
  return line(lineNo);
}

const Text::Letter *Text::LineView::hoverLetter(float cursorX) const noexcept {
  if (letters.empty()) return nullptr;
  if (!(cursorX > 0)) return letters.begin();
  if (!(cursorX < letters.back().right)) return letters.end() - (letters.back().codepoint == '\n');
  return std::ranges::lower_bound(letters, cursorX, std::ranges::less(), [](auto &letter) { return letter.left; }) - 1;
}

Text::LineView::operator Rect() const noexcept {
  return {{0, baselinePlusDescent}, {letters.empty() ? 0 : letters.back().right, baselinePlusAscent}};
}

Vector2f Text::cursorToInsertBefore(const Letter *letter) const noexcept {
  if (letter < mLetters.data() || !letter) return {};
  if (letter >= mLetters.data() + mLetters.size()) return cursorToInsertAfter(&mLetters.back());
  return {max(0, letter->left), letter->baseline};
}

Vector2f Text::cursorToInsertAfter(const Letter *letter) const noexcept {
  if (letter < mLetters.data() || !letter) return {};
  if (letter >= mLetters.data() + mLetters.size()) letter = &mLetters.back();
  if (letter->codepoint == '\n') return {0, letter->baseline + mLineAdvance};
  return {letter->right, letter->baseline};
}

Vector2f Text::cursorCentered(const Letter *letter) const noexcept {
  Vector2f cursor = cursorToInsertBefore(letter);
  if (letter < mLetters.data() || !letter) return cursor;
  if (letter < mLetters.data() + mLetters.size()) cursor[0] = 0.5f * letter->left + 0.5f * letter->right;
  return cursor;
}

Text::operator std::string() const {
  std::string result;
  result.reserve(mLetters.size());
  for (const Letter &letter : mLetters) result += UTF8Encoding(letter.codepoint);
  return result;
}

} // namespace mi::ui
