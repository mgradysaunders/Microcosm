find_package(OpenMP REQUIRED)
microcosm_add_library(
  "Render"
  SHARED
  SOURCES
    "common.cc"
    "Manifold.cc"
    "Material.cc"
    "MLT.cc"
    "Path.cc"
    "Scattering.cc"
    "Scene.cc"
    "Shape.cc"
    "Spectrum.cc"
    "SpectrumImage.cc"
    "More/Scattering/Diffuse.cc"
    "More/Scattering/Diffusion.cc"
    "More/Scattering/Fresnel.cc"
    "More/Scattering/Media.cc"
    "More/Scattering/Microflake.cc"
    "More/Scattering/Microsurface.cc"
    "More/Scattering/Phase.cc"
    "More/Shape/Cylinder.cc"
    "More/Shape/Disk.cc"
    "More/Shape/Follicle.cc"
    "More/Shape/Sphere.cc"
    "More/Shape/Triangle.cc"
    "More/Shape/TriangleMesh.cc"
    "More/Spectrum/Prospect.cc"
    "More/Spectrum/RefractiveIndex.cc"
  DEPENDS 
    ${PROJECT_NAME}::Geometry
    ${PROJECT_NAME}::Quadrature
    OpenMP::OpenMP_CXX
  EXPORT_MACRO "MI_RENDER_API"
  EXPORT_FILENAME "Microcosm/Render/Export.h"
  )
#add_subdirectory(unit_tests)
