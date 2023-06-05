microcosm_add_library(
  "Geometry"
  SHARED
  SOURCES
    "Delaunator.cc"
    "DynamicBVH.cc"
    "DynamicKDTree.cc"
    "FCurve.cc"
    "FileMTL.cc"
    "FileOBJ.cc"
    "FilePLY.cc"
    "glTF.cc"
    "HalfEdgeMesh.cc"
    "ImmutableBVH.cc"
    "ImmutableKDTree.cc"
    "IntersectMPR.cc"
    "Mesh.cc"
    "PiecewiseLinear.cc"
    "SparseMatrix.cc"
  DEPENDS 
    ${PROJECT_NAME}::Json
  EXPORT_MACRO "MI_GEOMETRY_API"
  EXPORT_FILENAME "Microcosm/Geometry/Export.h"
  )
target_include_directories(Geometry PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
add_subdirectory(unit_tests)
