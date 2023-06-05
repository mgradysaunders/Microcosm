#include "Microcosm/UI/Artist/Fill"

namespace mi::ui {

FillArtist &FillArtist::nextCorner(Vector2f position) {
  Corner corner;
  corner.position = position;
  corner.innerIdx = mDrawCmds.emit(Vtx(position).withColor(mFillColor));
  if (!(mCornerRadius > mStrokeWidth)) {
    corner.cornerRadius = 0;
    corner.outerIdxA = corner.innerIdx;
    corner.outerIdxB = corner.innerIdx;
  } else {
    uint32_t outerCount = mCornerResolution + 2;
    corner.cornerRadius = mCornerRadius;
    corner.outerIdxA = corner.innerIdx + 1;
    corner.outerIdxB = corner.outerIdxA + outerCount - 1;
    mDrawCmds.emitTriFan(corner.innerIdx, outerCount + 1);
    for (uint32_t i = 0; i < outerCount; i++) mDrawCmds.emit(Vtx(position).withColor(mFillColor));
  }
  corner.strokeWidth = mStrokeWidth;
  corner.strokeColor = mStrokeColor;
  mCorners.push(corner);
  if (mCorners.size() > 1) {
    auto &cornerY = mCorners[mCorners.size() - 2];
    auto &cornerZ = mCorners.back();
    finalizeCorner(cornerY, cornerZ);
  }
  if (mCorners.size() > 2) {
    auto &cornerX = mCorners[mCorners.size() - 3];
    auto &cornerY = mCorners[mCorners.size() - 2];
    auto &cornerZ = mCorners.back();
    finalizeCorner(cornerX, cornerY, cornerZ);
  }
  return *this;
}

FillArtist &FillArtist::finish() {
  if (mCorners.size() >= 3) [[likely]] {
    finalizeCorner(mCorners[-1], mCorners[+0]);
    finalizeCorner(mCorners[-2], mCorners[-1], mCorners[+0]);
    finalizeCorner(mCorners[-1], mCorners[+0], mCorners[+1]);

    bool isStrokeCompletelyOpaque = true;
    bool isStrokeCompletelyTransparent = true;
    for (const auto &corner : mCorners) {
      isStrokeCompletelyOpaque &= corner.strokeColor[3] == 0xFF;
      isStrokeCompletelyTransparent &= corner.strokeColor[3] == 0x00;
    }

    // Emit fringe around the edges of the fill region to anti-alias. However, if we
    // have an opaque outline, then we can skip this because the line will hide whatever
    // aliasing would happen anyway.
    if (!isStrokeCompletelyOpaque) {
      std::optional<DrawCmds::FringeState> fringeState;
      for (const auto &[corner, nextCorner] : ranges::adjacent<2>(mCorners, true))
        for (uint32_t idx = corner.outerIdxA; idx <= corner.outerIdxB; idx++)
          fringeState = mDrawCmds.emitFringe(mCtx, idx, idx < corner.outerIdxB ? idx + 1 : nextCorner.outerIdxA, fringeState);
      mDrawCmds.finishFringeCloseLoop(mCtx, *fringeState);
    }

    // Emit outline on top of the fill region. Skip if the outline is completely
    // transparent, in which case it would be super wasteful to generate a bunch
    // of geometry that will never appear on screen.
    if (!isStrokeCompletelyTransparent) {
      std::optional<DrawCmds::StrokeState> strokeState;
      for (const auto &corner : mCorners)
        for (uint32_t idx = corner.outerIdxA; idx <= corner.outerIdxB; idx++)
          strokeState = mDrawCmds.emitStroke( //
            mCtx, Vtx(mDrawCmds.vtxBuffer()[idx]).withColor(corner.strokeColor), corner.strokeWidth, strokeState);
      mDrawCmds.finishStrokeCloseLoop(mCtx, *strokeState);
    }
  }
  mCorners.clear();
  return *this;
}

void FillArtist::finalizeCorner(const Corner &cornerY, const Corner &cornerZ) {
  if (cornerY.cornerRadius > 0) mDrawCmds.emit(cornerY.outerIdxB, cornerZ.innerIdx, cornerY.innerIdx);
  if (cornerZ.cornerRadius > 0) mDrawCmds.emit(cornerY.outerIdxB, cornerZ.outerIdxA, cornerZ.innerIdx);
  if (mCorners.size() > 2) mDrawCmds.emit(mCorners[0].innerIdx, cornerY.innerIdx, cornerZ.innerIdx);
}

void FillArtist::finalizeCorner(const Corner &cornerX, const Corner &cornerY, const Corner &cornerZ) {
  if (cornerY.cornerRadius > 0) {
    float distanceA = fastLength(cornerX.position - cornerY.position);
    float distanceB = fastLength(cornerZ.position - cornerY.position);
    if (!(distanceA > 0 && distanceB > 0)) return;
    const Vector2f edgeA = (cornerX.position - cornerY.position) * (1.0f / distanceA);
    const Vector2f edgeB = (cornerY.position - cornerZ.position) * (1.0f / distanceB);
    const Vector2f normalA = hodge(edgeA);
    const Vector2f normalB = hodge(edgeB);
    const Vector2f normalC = normalA + normalB;
    const float radius = finiteOrZero(min(cornerY.cornerRadius, 0.475f * min(distanceA, distanceB)));
    const float offset = finiteOrZero(abs(radius * cross(normalA, normalC) / dot(normalA, normalC)));
    const Vector2f center = cornerY.position - hypot(radius, offset) * fastNormalize(normalC);
    const Vector2f pointA = cornerY.position + offset * edgeA;
    const Vector2f pointB = cornerY.position - offset * edgeB;
    const float thetaA = atan2(normalA[1], normalA[0]);
    const float thetaB = thetaA + angleBetween(normalA, normalB);
    if (!isfinite(thetaA)) return;
    Vtx *outerVtx = &mDrawCmds.vtxBuffer()[cornerY.outerIdxA + 1];
    for (Vector2f cosSinTheta :
         unitCircleLinspace(cornerY.outerIdxB - cornerY.outerIdxA + 1, Exclusive(thetaA), Exclusive(thetaB), pointA - center)) {
      outerVtx->position = center + cosSinTheta;
      outerVtx++;
    }
    mDrawCmds.vtxBuffer()[cornerY.outerIdxA].position = pointA;
    mDrawCmds.vtxBuffer()[cornerY.outerIdxB].position = pointB;
    mDrawCmds.vtxBuffer()[cornerY.innerIdx].position = center;
  }
}

} // namespace mi::ui
