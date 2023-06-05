#include "Microcosm/UI/DrawCmds"
#include <cstring>

namespace mi::ui {

void DrawCmds::commit(const Context &ctx) {
  uint32_t count = mIdxBuffer.size() - mFirstIdx;
  if (count == 0) [[unlikely]]
    return;
  Cmd cmd = {
    .scissor = Rect(),
    .layer = ctx->layer,
    .texture = ctx->texture,
    .tintGain = ctx->tintGain,
    .tintBias = ctx->tintBias,
    .modelView = ctx.modelToCanvas(),
    .firstVtx = mFirstVtx,
    .firstIdx = mFirstIdx,
    .count = count};
  mFirstVtx = mVtxBuffer.size();
  mFirstIdx = mIdxBuffer.size();
  cmd.scissor = {Vector2f(0, 0), Vector2f(ctx.screen.screenSize)};
  if (ctx->scissor) {
    Transform transform = ctx.screen.canvasToScreen();
    cmd.scissor[0] = transform.applyAffine((*ctx->scissor)[0]);
    cmd.scissor[1] = transform.applyAffine((*ctx->scissor)[1]);
  }

  if (
    mCmdItr != mCmdBuffer.end() &&                    //
    mCmdItr->layer == cmd.layer &&                    //
    mCmdItr->texture == cmd.texture &&                //
    allTrue(mCmdItr->scissor[0] == cmd.scissor[0]) && //
    allTrue(mCmdItr->scissor[1] == cmd.scissor[1]) && //
    allTrue(mCmdItr->tintGain == cmd.tintGain) &&     //
    allTrue(mCmdItr->tintBias == cmd.tintBias)) {
    // We want to support context-based transforms without generating a ton of GPU calls, so
    // if the model-view transform is the only thing that changed since the previous command,
    // we can extend the previous command by transforming the vertex positions accordingly.
    if (std::memcmp(&mCmdItr->modelView, &cmd.modelView, sizeof(Transform)) != 0) {
      Transform modelViewOffset = mCmdItr->modelView.inverse() * cmd.modelView;
      for (auto &each : IteratorRange(&mVtxBuffer[0] + cmd.firstVtx, &mVtxBuffer[0] + mFirstVtx)) {
        each.position = modelViewOffset.applyAffine(each.position);
      }
    }
    mCmdItr->count += count;
  } else {
    if (mCmdBuffer.empty()) {
      mCmdItr = mCmdBuffer.insert(mCmdBuffer.end(), cmd);
    } else if (mCmdItr->layer == cmd.layer) {
      mCmdItr = mCmdBuffer.insert(mCmdItr + 1, cmd);
    } else {
      mCmdItr = std::upper_bound(
        mCmdItr->layer < cmd.layer ? mCmdItr : mCmdBuffer.begin(), //
        mCmdItr->layer > cmd.layer ? mCmdItr : mCmdBuffer.end(), cmd.layer,
        [](auto layer, const Cmd &cmd) { return layer < cmd.layer; });
      mCmdItr = mCmdBuffer.insert(mCmdItr, cmd);
    }
  }
}

void DrawCmds::emitText(const Context &ctx, const Text &text) {
  for (const Text::Letter &letter : text) {
    Vector4b foreground = letter.foreground;
    Vector4b background = letter.background;
    if (letter.emphasis.faint) foreground[3] /= 2, background[3] /= 2;
    if (letter.emphasis.blink) foreground[3] *= ctx.clock.blink, background[3] *= ctx.clock.blink;
    if (background[3] != 0x00 && letter.left != letter.right) {
      Rect position = Rect(letter);
      emitTriFan(vtxBuffer().size(), 4);
      emit(
        Vtx(position.northEast()).withColor(background), //
        Vtx(position.northWest()).withColor(background), //
        Vtx(position.southWest()).withColor(background), //
        Vtx(position.southEast()).withColor(background));
    }
    if (letter.emphasis.underline && letter.left != letter.right) {
      StrokeState state;
      state = emitStroke(ctx, Vtx(Vector2f(letter.left, letter.underline())).withColor(foreground), 0);
      state = emitStroke(ctx, Vtx(Vector2f(letter.right, letter.underline())).withColor(foreground), 0, state);
    }
    if (letter) {
      Rect position = letter.glyph->rect + Vector2f(letter.left, letter.baseline);
      Rect texcoord = letter.glyph->atlasRect;
      Vector2f position00 = position(0, 0);
      Vector2f position01 = position(0, 1);
      Vector2f position11 = position(1, 1);
      Vector2f position10 = position(1, 0);
      if (letter.emphasis.italic) { // Fake italics by slanting the rectangle.
        constexpr float slope = 0.25f;
        position00[0] += slope * letter.glyph->rect[0][1];
        position01[0] -= slope * letter.glyph->rect[1][1];
        position11[0] -= slope * letter.glyph->rect[1][1];
        position10[0] += slope * letter.glyph->rect[0][1];
      }
      float fontFactor = letter.emphasis.bold ? 0.7f : 0.5f; // Fake bold by increasing the text weight.
      emitTriFan(vtxBuffer().size(), 4);
      emit(
        Vtx(position00, texcoord(0, 0), foreground).withFontFactor(fontFactor),
        Vtx(position01, texcoord(0, 1), foreground).withFontFactor(fontFactor),
        Vtx(position11, texcoord(1, 1), foreground).withFontFactor(fontFactor),
        Vtx(position10, texcoord(1, 0), foreground).withFontFactor(fontFactor));
    }
    if (letter.emphasis.strike && letter.left != letter.right) {
      StrokeState state;
      state = emitStroke(ctx, Vtx(Vector2f(letter.left, letter.strike())).withColor(foreground), 0);
      state = emitStroke(ctx, Vtx(Vector2f(letter.right, letter.strike())).withColor(foreground), 0, state);
    }
  }
}

void DrawCmds::emitTextCursor(const Context &ctx, const Text &text, const Text::Letter *letter) {
  Vector2f cursor = text.cursorToInsertBefore(letter);
  auto cursorLine = text.hoverLine(cursor[1], /*clampLineNo=*/false);
  Vector2f cursorA = {cursor[0], cursorLine.baselinePlusAscent};
  Vector2f cursorB = {cursor[0], cursorLine.baselinePlusDescent};
  Vector4b color = {0xFF, 0xFF, 0xFF, 0xFF};
  color[3] *= ctx.clock.blink;
  StrokeState state;
  state = emitStroke(ctx, Vtx(cursorA).withColor(color), 0);
  state = emitStroke(ctx, Vtx(cursorB).withColor(color), 0, state);
}

#if 0
void DrawCmds::emitParametric(Vector2i numSubdivs, const std::function<Vtx(Vector2f)> &parametric) {
  if (anyTrue(numSubdivs < 0)) return;
  Idx idx0 = next();
  Idx nU = numSubdivs[0];
  Idx nV = numSubdivs[1];
  for (Idx kU = 0; kU < nU; kU++) {
    for (Idx kV = 0; kV < nV; kV++) {
      Idx kU0 = (kU + 0) * (nV + 1), kV0 = (kV + 0);
      Idx kU1 = (kU + 1) * (nV + 1), kV1 = (kV + 1);
      emitTriFan(idx0 + kU0 + kV0, {idx0 + kU0 + kV1, idx0 + kU1 + kV1, idx0 + kU1 + kV0});
    }
  }
  for (float coordU : linspace(nU + 1, 0.0f, 1.0f)) {
    for (float coordV : linspace(nV + 1, 0.0f, 1.0f)) {
      emit(parametric(Vector2f(coordU, coordV)));
    }
  }
}
#endif

DrawCmds::FringeState
DrawCmds::emitFringe(const Context &ctx, Idx idxA, Idx idxB, std::optional<FringeState> prevState, float winding) {
  assert(idxA < mVtxBuffer.size() && idxB < mVtxBuffer.size());
  if (idxA == idxB || winding == 0) return {};
  auto fringeOffset = [&](Vector2f localEdge) {
    const auto modelView = ctx.modelToCanvas();
    const Vector2f worldEdge = modelView.applyLinear(localEdge);
    const Vector2f worldEdgeNormal = fastNormalize(hodge(worldEdge));
    const Vector2f worldFringeOffset = -ctx.screen.fringeScale * worldEdgeNormal;
    const Vector2f localFringeOffset = modelView.inverse().applyLinear(worldFringeOffset);
    return localFringeOffset;
  };
  FringeState thisState;
  const auto &vtxA = mVtxBuffer[idxA];
  const auto &vtxB = mVtxBuffer[idxB];
  const Vector2f direction = fastNormalize(vtxB.position - vtxA.position);
  const Vector2f offsetVec = fringeOffset(direction) * winding;
  if (allTrue(direction == 0) || !allTrue(isfinite(direction))) [[unlikely]] { // Attempt at damage control
    thisState.first.idxA = thisState.idxA = idxA;
    thisState.first.idxA = thisState.idxB = idxB;
    if (prevState) thisState.first = prevState->first;
    return thisState;
  }
  const Vector2f positionA = vtxA.position + offsetVec;
  const Vector2f positionB = vtxB.position + offsetVec;
  thisState.direction = direction;
  if (!prevState) {
    thisState.first.idxA = thisState.idxA = emit(Vtx(vtxA).withPosition(positionA).withColorA(0));
    thisState.first.idxB = thisState.idxB = emit(Vtx(vtxB).withPosition(positionB).withColorA(0));
    thisState.first.direction = direction;
  } else {
    /*
    auto &prevPositionA = mVtxBuffer[prevState->idxA].position; */
    auto &prevPositionB = mVtxBuffer[prevState->idxB].position;
    prevPositionB = miter(prevPositionB, prevState->direction, positionA, direction);
    thisState.idxA = prevState->idxB;
    thisState.idxB = emit(Vtx(vtxB).withPosition(positionB).withColorA(0));
    thisState.first = prevState->first;
  }
  if (winding > 0) {
    emitTriFan(idxA, {thisState.idxA, thisState.idxB, idxB});
  } else {
    // If the winding multiplier is negative, then everything is reflected,
    // so the indices have to be emitted in reverse order for the triangle
    // to be CCW.
    emitTriFan(idxA, {idxB, thisState.idxB, thisState.idxA});
  }
  return thisState;
}

void DrawCmds::finishFringeCloseLoop(const Context &, const FringeState &lastState) {
  auto &prevPositionB = mVtxBuffer[lastState.idxB].position;
  auto &nextPositionA = mVtxBuffer[lastState.first.idxA].position;
  prevPositionB = nextPositionA = miter(prevPositionB, lastState.direction, nextPositionA, lastState.first.direction);
}

DrawCmds::StrokeState
DrawCmds::emitStroke(const Context &ctx, const Vtx &vtx, float width, const std::optional<StrokeState> &prevState) {
  StrokeState thisState;
  thisState.position = vtx.position;
  thisState.texcoord = vtx.texcoord;
  thisState.color = vtx.color;
  thisState.width = width;
  if (!prevState) {
    thisState.isFirst = true;
    thisState.first.idxA = thisState.idxA = emit(vtx);
    thisState.first.idxB = thisState.idxB = width > 0 ? emit(vtx) : thisState.idxA;
    return thisState; // We're done.
  }
  thisState.isFirst = false;

  auto &prev = *prevState;
  float distanceToThis = fastLength(thisState.position - prev.position);
  if (distanceToThis < 1e-5f) return prev;
  Vector2f direction = (thisState.position - prev.position) * (1.0f / distanceToThis);
  Vector2f perpDirection = hodge(direction) * 0.5f;
  Vector2f positionA = vtx.position;
  Vector2f positionB = vtx.position;
  thisState.direction = direction;
  if (width > 0) {
    positionA -= width * perpDirection;
    positionB += width * perpDirection;
    thisState.idxA = emit(Vtx(vtx).withPosition(positionA));
    thisState.idxB = emit(Vtx(vtx).withPosition(positionB));
    emit(prev.idxA, thisState.idxA, thisState.idxB);
  } else {
    thisState.idxA = thisState.idxB = emit(vtx);
  }
  if (prev.width > 0) {
    auto &prevVtxA = mVtxBuffer[prev.idxA];
    auto &prevVtxB = mVtxBuffer[prev.idxB];
    Vector2f snapPrevPositionA = prev.position - prev.width * perpDirection;
    Vector2f snapPrevPositionB = prev.position + prev.width * perpDirection;
    if (prev.isFirst) {
      prevVtxA.position = snapPrevPositionA;
      prevVtxB.position = snapPrevPositionB;
    } else {
      Vector2f snapDirectionA = fastNormalize(positionA - snapPrevPositionA);
      Vector2f snapDirectionB = fastNormalize(positionB - snapPrevPositionB);
      prevVtxA.position = miter(prevVtxA.position, prev.direction, snapPrevPositionA, snapDirectionA);
      prevVtxB.position = miter(prevVtxB.position, prev.direction, snapPrevPositionB, snapDirectionB);
    }
    emit(prev.idxA, thisState.idxB, prev.idxB);
  }
  thisState.widthSlope = (thisState.width - prev.width) / distanceToThis;
  thisState.first = prev.first;
  if (prev.isFirst) {
    thisState.first.direction = direction;
    thisState.first.widthSlope = thisState.widthSlope;
    thisState.first.fringeA = thisState.fringeA = emitFringe(ctx, prev.idxA, thisState.idxA, {}, +1);
    thisState.first.fringeB = thisState.fringeB = emitFringe(ctx, prev.idxB, thisState.idxB, {}, -1);
  } else {
    thisState.fringeA = emitFringe(ctx, prev.idxA, thisState.idxA, prev.fringeA, +1);
    thisState.fringeB = emitFringe(ctx, prev.idxB, thisState.idxB, prev.fringeB, -1);
  }
  return thisState;
}

void DrawCmds::finishStroke(const Context &ctx, const StrokeState &lastState, bool roundCapFirst, bool roundCapLast) {
  if (lastState.isFirst) [[unlikely]]
    return;
  auto emitRoundCap = [&](Idx idxA, Idx idxB, Vector2f direction, float widthSlope) {
    if (idxA == idxB) return;
    const Vtx &vtxA = mVtxBuffer[idxA];
    const Vtx &vtxB = mVtxBuffer[idxB];
    const Vector2f directionX = direction;
    const Vector2f directionY = hodge(directionX);
    const Vector2f normalA = -fastNormalize(hodge(directionX - 0.5f * widthSlope * directionY));
    const Vector2f normalB = +fastNormalize(hodge(directionX + 0.5f * widthSlope * directionY));
    const float thetaA = signedAngleBetween(directionX, normalA);
    const float thetaB = signedAngleBetween(directionX, normalB);
    const Vector2f center = miter(vtxA.position, normalA, vtxB.position, normalB);
    StaticStack<Idx, 32> cap;
    cap.push(idxA);
    for (Vector2f cosSinTheta : unitCircleLinspace(12, Exclusive(thetaA), Exclusive(thetaB), vtxA.position - center))
      cap.push(emit(Vtx(vtxA).withPosition(center + cosSinTheta)));
    cap.push(idxB);
    std::optional<FringeState> fringeState;
    emitTriFan(idxA, cap);
    for (auto [idx0, idx1] : ranges::adjacent<2>(cap)) fringeState = emitFringe(ctx, idx0, idx1, fringeState, +1);
  };
  if (roundCapFirst) {
    emitRoundCap(                 //
      lastState.first.idxB,       //
      lastState.first.idxA,       //
      -lastState.first.direction, //
      -lastState.first.widthSlope);
  } else {
    emitFringe(ctx, lastState.first.idxA, lastState.first.idxB);
  }
  if (roundCapLast) {
    emitRoundCap(          //
      lastState.idxA,      //
      lastState.idxB,      //
      lastState.direction, //
      lastState.widthSlope);
  } else {
    emitFringe(ctx, lastState.idxB, lastState.idxA);
  }
}

void DrawCmds::finishStrokeCloseLoop(const Context &ctx, const StrokeState &lastState) {
  if (lastState.isFirst) [[unlikely]]
    return;
  auto prevIdxA = lastState.idxA, nextIdxA = lastState.first.idxA;
  auto prevIdxB = lastState.idxB, nextIdxB = lastState.first.idxB;
  auto &prevVtxA = mVtxBuffer[prevIdxA], &nextVtxA = mVtxBuffer[nextIdxA];
  auto &prevVtxB = mVtxBuffer[prevIdxB], &nextVtxB = mVtxBuffer[nextIdxB];
  if (prevIdxA != prevIdxB) emit(prevIdxA, nextIdxB, prevIdxB);
  if (nextIdxA != nextIdxB) emit(prevIdxA, nextIdxA, nextIdxB);
  const Vector2f prevCenter = 0.5f * (prevVtxA.position + prevVtxB.position);
  const Vector2f nextCenter = 0.5f * (nextVtxA.position + nextVtxB.position);
  const Vector2f direction = fastNormalize(nextCenter - prevCenter);
  const Vector2f perpDirection = hodge(direction) * 0.5f;
  const float prevWidth = fastLength(prevVtxA.position - prevVtxB.position);
  const float nextWidth = fastLength(nextVtxA.position - nextVtxB.position);
  const Vector2f snapPrevPositionA = prevCenter - prevWidth * perpDirection;
  const Vector2f snapPrevPositionB = prevCenter + prevWidth * perpDirection;
  const Vector2f snapNextPositionA = nextCenter - nextWidth * perpDirection;
  const Vector2f snapNextPositionB = nextCenter + nextWidth * perpDirection;
  const Vector2f snapDirectionA = snapPrevPositionA - snapNextPositionA;
  const Vector2f snapDirectionB = snapPrevPositionB - snapNextPositionB;
  if (prevIdxA != prevIdxB) {
    Vector2f prevDirection = hodge((prevVtxB.position - prevVtxA.position) * (1.0f / prevWidth));
    prevVtxA.position = miter(prevVtxA.position, prevDirection, snapPrevPositionA, snapDirectionA);
    prevVtxB.position = miter(prevVtxB.position, prevDirection, snapPrevPositionB, snapDirectionB);
  }
  if (nextIdxA != nextIdxB) {
    Vector2f nextDirection = hodge((nextVtxB.position - nextVtxA.position) * (1.0f / nextWidth));
    nextVtxA.position = miter(nextVtxA.position, nextDirection, snapNextPositionA, snapDirectionA);
    nextVtxB.position = miter(nextVtxB.position, nextDirection, snapNextPositionB, snapDirectionB);
  }
  finishFringeCloseLoop(ctx, emitFringe(ctx, prevIdxA, nextIdxA, lastState.fringeA, +1));
  finishFringeCloseLoop(ctx, emitFringe(ctx, prevIdxB, nextIdxB, lastState.fringeB, -1));
}

void DrawCmds::emitCircleWithFringe(const Context &ctx, const Vtx &vtx, float radius, int numSubdivs) {
  emitTriFan(next(), numSubdivs + 2);
  emit(vtx);
  Idx idx0 = next();
  for (Vector2f cosSinTheta : unitCircleLinspace(numSubdivs + 1, 0.0f, 360.0_degreesf))
    emit(Vtx(vtx).withPosition(vtx.position + radius * cosSinTheta));
  Idx idx1 = next();
  std::optional<FringeState> fringeState;
  for (Idx idx = idx0; idx + 1 < idx1; idx++) fringeState = emitFringe(ctx, idx, idx + 1, fringeState, +1);
}

} // namespace mi::ui
