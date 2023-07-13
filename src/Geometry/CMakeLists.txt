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
    "glTF.cc"
    "HalfEdgeMesh.cc"
    "ImmutableBVH.cc"
    "ImmutableKDTree.cc"
    "IntersectMPR.cc"
    "Mesh.cc"
    "PiecewiseLinearCurve.cc"
    "SparseMatrix.cc"
  DEPENDS 
    ${PROJECT_NAME}::Json
  EXPORT_MACRO "MI_GEOMETRY_API"
  EXPORT_FILENAME "Microcosm/Geometry/Export.h"
  )
target_include_directories(Geometry PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
if(assimp_FOUND)
  target_link_libraries(Geometry PUBLIC assimp::assimp)
  target_compile_definitions(Geometry PUBLIC -DMI_BUILT_WITH_ASSIMP=1)
endif()
add_subdirectory(unit_tests)

