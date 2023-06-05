#include "Microcosm/Render/Path"

namespace mi::render {

void Path::Vertex::assertValidInitialSurfaceVertex(const std::source_location &location) {
  const char *reason{
    !allTrue(isfinite(position))     ? "Non-finite position"
    : !manifold                      ? "Null manifold"
    : !materialProvider              ? "Null material provider"
    : runtime.flags.isInfinite       ? "Flagged as infinite"
    : runtime.flags.isIntangible     ? "Flagged as intangible"
    : runtime.flags.isDeltaPosition  ? "Flagged as delta position"
    : runtime.flags.isDeltaDirection ? "Flagged as delta direction"
                                     : nullptr};
  if (reason) [[unlikely]]
    throw Error(std::logic_error("Call to Path::Vertex::assertValidInitialSurfaceVertex() from {}:{} failed! Reason: {}"_format(
      location.file_name(), location.line(), reason)));
}

void Path::Vertex::assertValidInitialVolumeVertex(const std::source_location &location) {
  const char *reason{
    !allTrue(isfinite(position))     ? "Non-finite position"
    : !!manifold                     ? "Non-null manifold"
    : !!materialProvider             ? "Non-null material provider"
    : runtime.flags.isInfinite       ? "Flagged as infinite"
    : runtime.flags.isIntangible     ? "Flagged as intangible"
    : runtime.flags.isDeltaPosition  ? "Flagged as delta position"
    : runtime.flags.isDeltaDirection ? "Flagged as delta direction"
    : runtime.flags.isKnownOpaque    ? "Flagged as known opaque"
                                     : nullptr};
  if (reason) [[unlikely]]
    throw Error(std::logic_error("Call to Path::Vertex::assertValidInitialVolumeVertex() from {}:{} failed! Reason: {}"_format(
      location.file_name(), location.line(), reason)));
}

void Path::Vertex::invokeMaterialProvider(const Spectrum &waveLens) {
  if (!materialProvider)
    throw Error(
      std::logic_error("Tried to call Path::Vertex::invokeMaterialProvider(), but the vertex has no material provider!"));
  material = materialProvider(waveLens);
}

void PathConnector::connect(Random &random, PathView pathA, PathView pathB, const Receiver &receiver) const {
  for (ptrdiff_t i = 0; i <= pathA.size(); i++) {
    for (ptrdiff_t j = 0; j <= pathB.size(); j++) {
      connectTerm(random, IteratorRange(&pathA[0], i), IteratorRange(&pathB[0], j), receiver);
    }
  }
}

void PathConnector::connectTerm(Random &random, PathView pathA, PathView pathB, const Receiver &receiver) const {
  auto doTruncation = [&](Path::Vertex &vertexP) -> void {
    Path::Kind kind{vertexP.runtime.kind};
    if (mTruncation(vertexP)) {
      if (vertexP.runtime.kind != kind) [[unlikely]]
        throw Error(std::logic_error(
          "Call to PathConnector::connectTerm() failed! Reason: Truncation operator must return same kind of vertex!"));
      receiver(pathA, pathB, multipleImportanceWeight(pathA, pathB), vertexP.runtime.ratio);
    }
  };
  auto doCompletion = [&](Path::Vertex &vertexP, Path::Vertex &vertexQ) -> std::optional<Spectrum> {
    if (mCompletion(vertexP, vertexQ)) {
      if (vertexP.runtime.kind == vertexQ.runtime.kind) [[unlikely]]
        throw Error(std::logic_error(
          "Call to PathConnector::connectTerm() failed! Reason: Completion operator must return opposite kind of vertex!"));
      Vector3d omegaI{vertexP.omega(vertexQ)};
      Spectrum fP{spectrumZerosLike(vertexP.runtime.ratio)};
      vertexP.runtime.scatteringPDF = vertexP.material.scatter(random, vertexP.runtime.omegaO, omegaI, fP);
      if (Spectrum L{
            vertexP.runtime.ratio * fP * //
            vertexQ.runtime.ratio};
          isPositiveAndFinite(L) && mVisibility(vertexP, vertexQ, L)) {
        return L;
      }
    }
    return {};
  };
  const PathBackup backups[2]{PathBackup(pathA), PathBackup(pathB)};
  if (pathA.empty() && pathB.empty()) [[unlikely]] {
    return;
  } else if (pathA.empty()) {
    doTruncation(pathB.back()); // Apply truncation to path B when path A is empty.
  } else if (pathB.empty()) {
    doTruncation(pathA.back()); // Apply truncation to path A when path B is empty.
  } else if (pathA.size() == 1 && pathB.size() == 1) [[unlikely]] {
    return;
  } else if (pathA.size() == 1) { // Implied: && pathB.size() > 1
    Path::Vertex vertexA;
    if (auto L = doCompletion(pathB.back(), vertexA))
      receiver(PathView(&vertexA, 1), pathB, multipleImportanceWeight(PathView(&vertexA, 1), pathB), *L);
  } else if (pathB.size() == 1) { // Implied: && pathA.size() > 1
    Path::Vertex vertexB;
    if (auto L = doCompletion(pathA.back(), vertexB))
      receiver(pathA, PathView(&vertexB, 1), multipleImportanceWeight(pathA, PathView(&vertexB, 1)), *L);
  } else {
    // Connect paths.
    auto &vertexA{pathA.back()};
    auto &vertexB{pathB.back()};
    auto isConnectible = [&]() {
      return !vertexA.runtime.flags.isIncomplete && vertexA.material.hasScattering() && //
             !vertexB.runtime.flags.isIncomplete && vertexB.material.hasScattering();
    };
    if (!isConnectible()) return;
    Vector3d omegaI{vertexA.omega(vertexB)};
    Spectrum fA{spectrumZerosLike(vertexA.runtime.ratio)};
    Spectrum fB{spectrumZerosLike(vertexB.runtime.ratio)};
    vertexA.runtime.scatteringPDF = vertexA.material.scatter(random, vertexA.runtime.omegaO, +omegaI, fA);
    vertexB.runtime.scatteringPDF = vertexB.material.scatter(random, vertexB.runtime.omegaO, -omegaI, fB);
    if (Spectrum L{
          vertexA.runtime.ratio * fA * //
          vertexB.runtime.ratio * fB * //
          (1.0 / distanceSquare(vertexA.position, vertexB.position))};
        isPositiveAndFinite(L) && mVisibility(vertexA, vertexB, L)) {
      receiver(pathA, pathB, multipleImportanceWeight(pathA, pathB), L);
    }
  }
}

double PathConnector::multipleImportanceWeight(PathView pathA, PathView pathB) const {
  // Recalculate the relevant conjugate path-space PDFs.
  ptrdiff_t nA{pathA.size()};
  ptrdiff_t nB{pathB.size()};
  if (nA > 0 && nB > 0) {
    pathA[nA - 1].recalculateReversePathPDF(pathB[nB - 1]);
    pathB[nB - 1].recalculateReversePathPDF(pathA[nA - 1]);
  }
  if (nA > 1) pathA[nA - 2].recalculateReversePathPDF(pathA[nA - 1]);
  if (nB > 1) pathB[nB - 2].recalculateReversePathPDF(pathB[nB - 1]);

  // Determine the multiple importance weight as per the balance heuristic. Ordinarily, the balance heuristic
  // calculation looks like a basic sum-normalization. Suppose you sampled something according to density P, but
  // you could have sampled it from some other densities Q and R as well. Then the weight for the way you actually
  // sampled it with P is P/(P+Q+R). If you work out the probabilities for bidirectional connections in this way,
  // you can eventually arrive at the calculation here. The way we actually sampled this path is the product of all
  // of the forward path-space PDFs. We want to weight that against every other way we could have legitimately
  // sampled it. Consider this configuration:
  //
  //    A0 ----> A1 ~ ~ ~ B1 <---- B0
  //
  // We have two paths A and B with two vertices each. And the way we connected the entire path this time is by
  // sampling A1 from A0, sampling B1 from B0, and then connecting A1 to B1. If the entire path has N vertices in
  // general, then there are N + 1 ways of sampling it at most. It is N + 1 because we can form the path completely
  // "forward" and completely "reverse", and also via connection at each of its N - 1 edges. The other four strategies
  // in this case would be:
  //
  //    A0  ----> A1  ----> B1* ~ ~ ~ B0    (Connect B0 and B1)
  //    A0  ----> A1  ----> B1* ----> B0*   (Forward)
  //    A0  ~ ~ ~ A1* <---- B1  <---- B0    (Connect A0 and A1)
  //    A0* <---- A1* <---- B1  <---- B0    (Reverse)
  //
  // where the star notation indicates where we need the conjugate path-space probability density. As this suggests,
  // we need to conjugate each subpath cumulatively and in reverse order. Now notice that whenever applying the
  // balance heuristic, as in the initial example with P/(P+Q+R), we can divide through by the numerator to obtain
  // an equivalent expression 1/(1+D) where D=Q/P+R/P. If we do that, we obtain signficant term cancellation that
  // results in a nested arithmetic expression (1+Ri)Rj where Ri is the ratio of the reverse to forward PDFs.
  double denomA{1};
  double denomB{1};
  for (auto &vertex : pathA) {
    if (!vertex.runtime.flags.isDeltaScattering) {
      denomA *= vertex.runtime.pathPDF.reverse / vertex.runtime.pathPDF.forward;
      denomA += vertex.runtime.flags.isIncomplete ? 0 : 1;
    }
  }
  for (auto &vertex : pathB) {
    if (!vertex.runtime.flags.isDeltaScattering) {
      denomB *= vertex.runtime.pathPDF.reverse / vertex.runtime.pathPDF.forward;
      denomB += vertex.runtime.flags.isIncomplete ? 0 : 1;
    }
  }
  return finiteOrZero(1 / (denomA + denomB - 1));
}

} // namespace mi::render
