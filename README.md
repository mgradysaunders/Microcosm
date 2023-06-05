# Microcosm

> mi·cro·cosm (noun)
> 
> a community, place, or situation regarded as encapsulating in miniature the characteristic qualities or features of something much larger.

Hello :wave: and welcome! Microcosm is an extensible collection of modern C++ libraries intended to be, well, a *microcosm* for [topics in computer graphics](#topics). While everything is all in one repo, it is not one library. Rather, it is a bunch of libraries written side by side with some sensible level of interdepence, all sharing a template-heavy header-only core.

Microcosm prioritizes three philosophical design pillars:

1. To be **clear and instructive**.
   Use understandable names. Write meaningful comments. Stop and think about language features and design patterns on occasion.

2. To be **minimal**.
   Eliminate redundancy by identifying and refactoring meaningful operations. Do not sacrifice simplicity on the altar of optimization unless completely necessary.

3. To be **as self contained as possible**.
   Only pull in external dependencies when there is a very good reason to do so. Even then, prefer implementations that can be (legally) embedded into the source to keep the build simple.

### Topics

Which topics? Good question. The answer is not entirely well-defined. On one hand, the project certainly wants to be extensible and to grow over time. On the other, bloat and feature creep ruin everything. Here is the current gamut:

- **Core**.
  The core refers to the all of the header-only includes plus a few libraries associated with specific top-level includes, for which there is a good reason not to be header-only. The header only includes cover the usual suite of graphics math structures, shapes, and allocation strategies. The libraries cover JSON represention, miniz compression, numerical quadrature, and binary serialization.

- **Geometry**.
  The geometry library contains more specific geometrical implementations. This includes spatial tree structures (bound box trees, k-d trees), polygonal meshes, common file formats for polygonal meshes (OBJ, PLY, glTF), delaunay triangulations, and so on.

- **GPU**
  The gpu library provides some basic wrappers and interfaces for OpenGL and Vulkan. The OpenGL implementation embeds [gl3w](https://github.com/skaslev/gl3w).

- **UI**
  The UI library defines a basic API-independent framework for 2D immediate mode rendering. It is inspired by the functionality in [imgui](https://github.com/ocornut/imgui). _Work in-progress_.

- **Render**
  The render library contains utilities for offline rendering via ray-tracing. This includes some popular BSDFs and phase functions, diffusion profiles, a fast-enough implementation of an intersectable triangle mesh, and so on.

