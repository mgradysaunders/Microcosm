#include "Microcosm/Geometry/IntersectMPR"
#include "Microcosm/utility"
#include <iostream>

namespace mi::geometry {

template <size_t N> bool IntersectMPR<N>::run() {
  // Initialize with the center support the user gives in the MinkowskiDifference structure.
  supports[0] = minkowskiDifference.center();

  // In theory this is a loop up to the simplex dimension. Since we only practically need to support
  // up to 4 dimensions, we write everything out manually using templates and compile-time constants
  // to have better code generation.
  if constexpr (N >= 1)
    if (auto containsOrigin = calculateWitnessAndFindSupport<0, 1>(supports[1]); containsOrigin != ContainsOrigin::Maybe)
      return containsOrigin == ContainsOrigin::Yes;
  if constexpr (N >= 2)
    if (auto containsOrigin = calculateWitnessAndFindSupport<0, 2>(supports[2]); containsOrigin != ContainsOrigin::Maybe)
      return containsOrigin == ContainsOrigin::Yes;
  if constexpr (N >= 3)
    if (auto containsOrigin = calculateWitnessAndFindSupport<0, 3>(supports[3]); containsOrigin != ContainsOrigin::Maybe)
      return containsOrigin == ContainsOrigin::Yes;
  if constexpr (N >= 4)
    if (auto containsOrigin = calculateWitnessAndFindSupport<0, 4>(supports[4]); containsOrigin != ContainsOrigin::Maybe)
      return containsOrigin == ContainsOrigin::Yes;

  size_t iteration{0};
  while (true) {
    Witness prevWitness{witness};
    Support support;
    if (auto containsOrigin = calculateWitnessAndFindSupport<1, N + 1>(support); containsOrigin == ContainsOrigin::No)
      [[unlikely]] {
      return false;
    } else {
      if (++iteration > maxIterations) [[unlikely]]
        return containsOrigin == ContainsOrigin::Yes;

      // If recalculating the witness didn't move it beyond the tolerance, we are done. Notice that,
      // at this point it is possible to believe that we *maybe* contain the origin, but aren't sure
      // yet. If we reach tolerance before confirming that we do contain the origin, then we do *NOT*
      // contain the origin.
      if (!(absDot(witness.direction, witness.point - prevWitness.point) > tolerance)) [[unlikely]]
        return containsOrigin == ContainsOrigin::Yes;

      // If recalculating the witness yielded a point that had to be clamped to the portal boundary,
      // then we have gotten as close as we are going to get. The idea here is, for generally smooth
      // surfaces, the nearest point to the origin on the Minkowski difference will not be exactly
      // visible through the portal. The portal converges to the point opposite the center (supports[0])
      // through the origin. However, the absolute nearest point (absolute minimal way to resolve the
      // intersection) should satisfy the constraint that the direction from the origin to the point
      // is the same as the normal direction at the point. We rely on this being approximately true,
      // but it generally won't be exactly true.
      if (anyTrue(witness.barycentric[Slice<1, N + 1>()] == 0)) [[unlikely]]
        return containsOrigin == ContainsOrigin::Yes;

      // Now we have to determine which support should be replaced by the new support without
      // invalidating the portal. Recall that the key idea about the "portal" is that the center
      // support should always be able to see the origin through it. So we have to guarantee that
      // a ray from the origin to the center support intersects the sub-simplex representing the
      // portal. We iterate over each support in the current portal (indexes 1,2,...,N) and
      // replace it by the new support, then test if the origin is still visible. We can exit
      // immediately when any test passes because in theory the test should pass exactly once.
      bool success{false};
      Matrix<float, N, N> matrixS;
      for (size_t j = 0; j < N; j++) matrixS.col(j).assign(supports[j + 1].v);
      for (size_t j = 0; j < N; j++) {
        // Swap the new support for column j, which is support j + 1.
        matrixS.col(j).assign(support.v);
        try {
          // The general linear system for ray-simplex intersection is determined by a few
          // basic considerations:
          // - The parametric equation for a ray is R(t) = O + t * D.
          // - The parametric equation for a simplex is S(b1, ..., bn) = b1 * S1 + ... + bn * Sn.
          // - The idea of an intersection is some vector of parameters (b1, ..., bn, t) that satisfy
          //   S(b1, ..., bn) = R(t), or rewriting as a homogeneous constraint,
          //   S(b1, ..., bn) - R(t) = 0.
          // - This combines with the constraint that barycentric coordinates sum to 1 to
          //   form a linear system with the following structure, which is solvable using the
          //   standard formula for 2x2 block matrices (https://en.wikipedia.org/wiki/Block_matrix):
          //      ( S    -D ) ( b ) = ( O )
          //      ( 1^T   0 ) ( t )   ( 1 )
          // For our purposes here, we know that the ray origin is in fact the true origin of all zeros,
          // and we do not care if the barycentric coordinates are normalized, and we do not care about the
          // ray parameter at all. This simplifies everything down to a single matrix inverse b = -S^-1 D.
          // If all of the barycentric coordinates are the same sign, we know the ray intersected with the
          // portal.
          Vector<float, N> barycentric = DecompLU(matrixS).solve(supports[0].v);
          if (allTrue(barycentric >= 0) || allTrue(barycentric <= 0)) {
            supports[j + 1] = support;
            success = true;
            break;
          }
        } catch (...) {
          // Do nothing. This just means the matrix was not invertible, in which case carry on.
        }
        // Swap support j + 1 back in to column j.
        matrixS.col(j).assign(supports[j + 1].v);
      }
      // It is not completely obvious to me what we should return if no ray intersection test
      // actually passes. This should be extremely rare. I can only imagine it happening in the
      // case where the portal is very tightly converged, so all of the supports are very close
      // to each other and the LU decomposition is ill-posed. With that in mind, I figure we
      // should treat this the same as if we are reaching tolerance.
      if (!success) return containsOrigin == ContainsOrigin::Yes;
    }
  }

  // Unreachable.
  return false;
}

template <size_t N> Vector<float, N> IntersectMPR<N>::penetrationCenter() const noexcept {
  Vector<float, N> center;
  Vector<float, N + 1> barycentric{witness.barycentric};
  // If the witness is full rank, then the barycentric coordinates stored in the witness
  // do not represent the intersection center. They instead represent the nearest point on the
  // portal to the origin. In this case, we need to compute the barycentric coordinates of the
  // origin in the entire simplex. We do this with LU decomposition because that is much faster
  // than SVD, and at this point we know/expect the linear system to be square and full-rank.
  if (witness.isFullRank()) [[likely]] {
    try {
      barycentric = DecompLU{supportMatrix<0, N + 1>()}.solve(Vector<float, N>().append(1.0f));
    } catch (...) {
      barycentric = Vector<float, N + 1>(1.0f / (N + 1));
    }
  }
  for (size_t k = 0; k < witness.rank; k++) center += 0.5f * barycentric[k] * (supports[k].pointA + supports[k].pointB);
  return center;
}

template <size_t N> Vector<float, N> IntersectMPR<N>::penetrationOffsetVector() const noexcept {
  // If the witness is full rank, then the barycentric coordinates represent the nearest point
  // on the portal to the origin, which is the representative point for the intersection offset
  // vector.
  return witness.isFullRank() ? witness.point : supports[witness.rank - 1].v;
}

template struct IntersectMPR<2>;
template struct IntersectMPR<3>;
template struct IntersectMPR<4>;

} // namespace mi::geometry
