#include "Microcosm/Render/Scene"

namespace mi::render {

Path Scene::walk(const Spectrum &waveLens, Random &random, Path::Vertex firstVertex, int maxDepth) const {
  if (maxDepth == 0) return {};
  Ray3d ray{firstVertex.position, firstVertex.runtime.omegaI, mShadowEpsilon, Inf};
  Medium medium{firstVertex.material.medium(ray.direction)};
  Spectrum ratio{firstVertex.runtime.ratio};
  Path path;
  path.push(std::move(firstVertex));
  for (int depth = 1; depth < maxDepth || maxDepth < 0; depth++) {
    {
      bool intersected{false};       // Intersected anything?
      bool intersectedVolume{false}; // Intersected volume specifically?
      const auto &lastVertex{path.back()};

      // First use the surface intersection routine. If we intersect something, truncate the ray
      // parameter (which establishes the maximum distance for medium transmission) and remember
      // that we hit something by setting intersected=true.
      Path::Vertex vertex;
      if (auto param = mIntersect(ray, vertex)) {
        ray.maxParam = *param, intersected = true;
        vertex.assertValidInitialSurfaceVertex(); // Sanity checks.
      }

      // Account for volume scattering by sampling an intercept in the current medium, between the
      // current vertex position and the intersected surface, or simply at any point if we did not
      // intersect a surface.
      if (auto volumeScattering = medium.transmissionSample(random, ray, ratio)) {
        vertex.position = volumeScattering->position, intersected = intersectedVolume = true;
        vertex.manifold = {};         // Nullify
        vertex.materialProvider = {}; // Nullify
        vertex.material.medium = MediumAccessor(medium);
        vertex.material.scattering = std::move(volumeScattering->scattering);
        vertex.runtime.flags.isKnownOpaque = false;
        vertex.assertValidInitialVolumeVertex(); // Sanity checks.
      }

      // Intersected surface specifically?
      if (intersected && !intersectedVolume) {
        vertex.invokeMaterialProvider(waveLens);
        // Hit medium boundary? If so, update the ray and medium and then skip to the next iteration.
        if (!vertex.material.hasScattering()) {
          ray.origin = vertex.position;
          ray.minParam = mShadowEpsilon;
          ray.maxParam = Inf;
          medium = vertex.material.medium(ray.direction);
          depth--; // Also do not count this iteration as a bounce!
          continue;
        }
      }

      // Remember what kind of path we're tracing.
      vertex.runtime.kind = lastVertex.runtime.kind;

      // Initialize ratio and directions. We set omegaI opposite omegaO initially because that is
      // the desirable default behavior for non-scattering interfaces that separate media.
      vertex.runtime.ratio = ratio;
      vertex.runtime.omegaO = -ray.direction;
      vertex.runtime.omegaI = +ray.direction;

      // Intersected nothing?
      if (!intersected) {
        vertex.position = lastVertex.position + ray.direction;
        vertex.runtime.flags.isInfinite = true;
      }

      vertex.recalculateForwardPathPDF(lastVertex);
      path.push(std::move(vertex));

      if (!intersected) break; // If we intersected nothing, we're done.
    }

    // If the ratio exploded or diminished to less than the minimum ratio threshold, then stop.
    Path::Vertex &vertex{path.back()};
    if (!isPositiveAndFinite(vertex.runtime.ratio, /*epsilon=*/mMinRatio)) [[unlikely]] {
      vertex.runtime.ratio = 0;
      break;
    }

    // If the vertex material has scattering, which is usually the case, then importance sample the incident
    // direction according to the scattering function. If this explodes or returns zero probability density
    // to indicate rejection, then stop.
    if (vertex.material.hasScattering()) [[likely]] {
      bool isDelta{false};
      vertex.runtime.scatteringPDF = vertex.material.scatterSample(random, vertex.runtime.omegaO, vertex.runtime.omegaI, ratio, isDelta);
      if (isDelta) {
        vertex.runtime.flags.isDeltaScattering = isDelta;
        vertex.runtime.scatteringPDF.forward = 1;
        vertex.runtime.scatteringPDF.reverse = 1;
      }
      if (
        !isPositiveAndFinite(vertex.runtime.scatteringPDF.forward) || //
        !isPositiveAndFinite(ratio, /*epsilon=*/mMinRatio)) [[unlikely]] {
        break;
      }
    }

    // Re-initialize ray and medium for next iteration.
    ray = Ray3d{vertex.position, vertex.runtime.omegaI, mShadowEpsilon, Inf}, medium = vertex.material.medium(ray.direction);
  }
  for (size_t i = 0; i + 1 < path.size(); i++) path[i].recalculateReversePathPDF(path[i + 1]);
  return path;
}

bool Scene::visibility(const Spectrum &waveLens, Random &random, const Path::Vertex &firstVertex, Vector3d omegaI, double maxDistance, Spectrum &tr) const {
  maxDistance *= 1 - mShadowEpsilon;
  if (!(maxDistance > mShadowEpsilon)) return true;
  Ray3d ray{firstVertex.position, normalize(omegaI), mShadowEpsilon, maxDistance};
  Path::Vertex lastVertex{firstVertex};
  Path::Vertex vertex;
  while (true) {
    // First use the surface intersection routine to find the nearest surface vertex. If the surface has
    // no scattering functions and only serves to separate participating media, then we must iterate past
    // it and account for the intermediate transmission term. If the surface is otherwise opaque, then we
    // return false to indicate no visibility.
    bool intersected{false};
    if (auto param = mIntersect(ray, vertex)) {
      ray.maxParam = *param, intersected = true;
      vertex.assertValidInitialSurfaceVertex();
      // Check the opaque flag first. If true, we already know that this vertex blocks visibility and
      // we do not have to construct the material. Otherwise, initialize the material from the provider and test
      // if it is opaque.
      if (vertex.runtime.flags.isKnownOpaque) return false;
      if (vertex.invokeMaterialProvider(waveLens); vertex.material.isOnOpaqueSurface()) return false;
    }

    // Account for medium transmission.
    lastVertex.material.medium(ray.direction).transmission(random, ray, tr);
    // If the transmission collapses to zero (or potentially explodes, but hopefully not), then
    // return false to indicate no visibility.
    if (!isPositiveAndFinite(tr)) [[unlikely]]
      return false;

    // If we did not intersect something, then we are done. We have performed intersection tests until the failure,
    // so we know the ray parameter range is exhausted, and we know that we have accounted for all transmission.
    if (!intersected) break;

    // Scoot the ray parameters. In the extremely rare case that the shadow epsilon pushes the minimum past the
    // maximum, then we will assume that no surface intersection would be detected within that epsilon distance and
    // thus return true.
    ray.minParam = ray.maxParam + mShadowEpsilon;
    ray.maxParam = maxDistance;
    if (ray.minParam >= ray.maxParam) [[unlikely]]
      break;

    // Prepare for the next iteration of the loop.
    lastVertex = std::move(vertex);
    vertex.clear();
  }
  return true;
}

} // namespace mi::render
