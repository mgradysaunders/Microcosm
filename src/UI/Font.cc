#include "Microcosm/UI/Font"

#define STBRP_STATIC 1
#define STB_RECT_PACK_IMPLEMENTATION 1
#include "stb_rect_pack.h"

#define STBTT_STATIC 1
#define STB_TRUETYPE_IMPLEMENTATION 1
#include "stb_truetype.h"

namespace mi::ui {

char32_t Font::glyphToCodepoint(char32_t glyph) const noexcept {
  return glyph < glyphs.size() ? glyphs[glyph].codepoint : None;
}

char32_t Font::codepointToGlyph(char32_t codepoint) const noexcept {
  auto itr = std::lower_bound(
    codepointSpans.begin(), codepointSpans.end(), codepoint,
    [](const CodepointSpan &codepointSpan, char32_t codepoint) { return codepointSpan.to <= codepoint; });
  if (itr != codepointSpans.end() && itr->contains(codepoint)) return codepoint - itr->from + itr->fromGlyph;
  return None;
}

float Font::kerning(char32_t glyph0, char32_t glyph1) const noexcept {
  auto itr = kerningTable.find(KerningKey(glyph0, glyph1));
  if (itr == kerningTable.end()) return 0;
  return itr->second;
}

float Font::netAdvanceWidth(char32_t glyph0, char32_t glyph1) const noexcept {
  return kerning(glyph0, glyph1) + (glyph0 < glyphs.size() ? glyphs[glyph0].advance : 0.0f);
}

void Font::load(const std::string &filename, const CodepointSpanRequests &spanRequests) {
  codepointSpans.clear();
  codepointSpans.reserve(16);
  for (auto &[from, to] : spanRequests) codepointSpans.push_back({None, from, to});
  std::sort(codepointSpans.begin(), codepointSpans.end(), [](const CodepointSpan &span0, const CodepointSpan &span1) {
    return span0.from < span1.from;
  });
  glyphs.clear();
  glyphs.reserve(256);
  for (CodepointSpan &span : codepointSpans) {
    span.fromGlyph = glyphs.size();
    for (char32_t codepoint = span.from; codepoint < span.to; codepoint++) glyphs.emplace_back().codepoint = codepoint;
  }

  auto fontdata = loadFileToString(filename);
  stbtt_fontinfo fontinfo;
  if (!stbtt_InitFont(&fontinfo, reinterpret_cast<unsigned char *>(fontdata.data()), 0))
    throw Error(std::runtime_error("Can't initialize font info for " + show(filename)));
  const float resolution = 64;
  const float scale = stbtt_ScaleForMappingEmToPixels(&fontinfo, resolution);
  const float normalization = scale / resolution;

  // Metrics.
  int ascent = 0;
  int descent = 0;
  int lineGap = 0;
  stbtt_GetFontVMetrics(&fontinfo, &ascent, &descent, &lineGap);
  metrics.ascent = normalization * ascent;
  metrics.descent = normalization * descent;
  metrics.lineGap = normalization * lineGap;
  std::map<int, int> glyphRemap;
  for (Glyph &glyph : glyphs) {
    int glyphIndex = stbtt_FindGlyphIndex(&fontinfo, glyph.codepoint);
    if (glyphIndex == 0) {
      glyph.invisible = true;
      continue;
    }
    int advance = 0;
    int leftSideBearing = 0;
    stbtt_GetGlyphHMetrics(&fontinfo, glyphIndex, &advance, &leftSideBearing);
    glyph.advance = normalization * advance;
    glyph.leftSideBearing = normalization * leftSideBearing;
    if (stbtt_IsGlyphEmpty(&fontinfo, glyphIndex)) glyph.invisible = true;
    glyphRemap[glyphIndex] = &glyph - &glyphs[0];
  }

  // Kerning.
  kerningTable.clear();
  std::vector<stbtt_kerningentry> table(stbtt_GetKerningTableLength(&fontinfo));
  if (stbtt_GetKerningTable(&fontinfo, table.data(), table.size())) {
    for (auto &entry : table) {
      auto itr1 = glyphRemap.find(entry.glyph1);
      auto itr2 = glyphRemap.find(entry.glyph2);
      if (itr1 != glyphRemap.end() && itr2 != glyphRemap.end())
        kerningTable.emplace(KerningKey(itr1->second, itr2->second), normalization * entry.advance);
    }
  } else {
    for (Glyph &glyph1 : glyphs) {
      int glyphIndex1 = stbtt_FindGlyphIndex(&fontinfo, glyph1.codepoint);
      for (Glyph &glyph2 : glyphs) {
        int glyphIndex2 = stbtt_FindGlyphIndex(&fontinfo, glyph2.codepoint);
        int index1 = &glyph1 - &glyphs[0];
        int index2 = &glyph2 - &glyphs[0];
        int kern = stbtt_GetGlyphKernAdvance(&fontinfo, glyphIndex1, glyphIndex2);
        if (kern != 0) kerningTable[KerningKey(index1, index2)] = normalization * kern;
      }
    }
  }

  // Atlas.
  std::vector<stbrp_rect> glyphRects;
  std::vector<unsigned char *> glyphDatas;
  glyphRects.reserve(glyphs.size());
  glyphDatas.reserve(glyphs.size());
  int maxSizeX = 0;
  int maxSizeY = 0;
  for (auto &glyph : glyphs) {
    int sizeX = 0;
    int sizeY = 0;
    int offsetX = 0;
    int offsetY = 0;
    if (glyph.invisible)
      glyphDatas.emplace_back(nullptr);
    else
      glyphDatas.emplace_back() =
        stbtt_GetCodepointSDF(&fontinfo, scale, glyph.codepoint, 12, 128, 30, &sizeX, &sizeY, &offsetX, &offsetY);
    glyph.rect[0] = {float(offsetX), float(offsetY)};
    glyph.rect[1] = {float(offsetX + sizeX), float(offsetY + sizeY)};
    glyph.rect *= Vector2f(1, -1) / resolution;
    glyphRects.emplace_back() = {0, sizeX, sizeY, 0, 0, 0};
    if (sizeX > maxSizeX) maxSizeX = sizeX;
    if (sizeY > maxSizeY) maxSizeY = sizeY;
  }
  int dim = sqrt(glyphs.size()) * max(maxSizeX, maxSizeY);
  std::vector<stbrp_node> nodes(dim);
  stbrp_context ctx;
  stbrp_init_target(&ctx, dim, dim, nodes.data(), nodes.size());
  if (!stbrp_pack_rects(&ctx, glyphRects.data(), glyphRects.size())) std::abort();
  int atlasSizeX = 0;
  int atlasSizeY = 0;
  for (const auto &rect : glyphRects) {
    atlasSizeX = max(atlasSizeX, rect.x + rect.w);
    atlasSizeY = max(atlasSizeY, rect.y + rect.h);
  }
  atlasSizeX += atlasSizeX & 1;
  atlasSizeX += atlasSizeX & 2;
  atlasSizeY += atlasSizeY & 1;
  atlasSizeY += atlasSizeY & 2;
  atlas.resize(atlasSizeX, atlasSizeY);
  for (size_t i = 0; i < glyphs.size(); i++) {
    auto &glyph = glyphs[i];
    auto &glyphRect = glyphRects[i];
    auto *glyphData = glyphDatas[i];
    if (glyph.invisible) continue;
    for (int y = glyphRect.y; y < glyphRect.y + glyphRect.h; y++)
      for (int x = glyphRect.x; x < glyphRect.x + glyphRect.w; x++) atlas(y, x) = *glyphData++;
    stbtt_FreeSDF(glyphDatas[i], nullptr);
    glyph.atlasRect[0] = {glyphRect.x / float(atlasSizeX), glyphRect.y / float(atlasSizeY)};
    glyph.atlasRect[1] = {(glyphRect.x + glyphRect.w) / float(atlasSizeX), (glyphRect.y + glyphRect.h) / float(atlasSizeY)};
  }

  // Tab is twice the em-width.
  if (char32_t glyphEm = codepointToGlyph('m'); glyphEm != None) {
    tabWidth = 2 * glyphs[glyphEm].width();
  } else {
    tabWidth = 1;
  }
}

} // namespace mi::ui
