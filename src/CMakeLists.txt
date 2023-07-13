microcosm_add_library("Json" SHARED SOURCES "Json.cc" EXPORT_MACRO "MI_JSON_API" EXPORT_FILENAME "Microcosm/Json_Export.h")
microcosm_add_library("miniz" SHARED SOURCES "miniz.c" "miniz.cc" EXPORT_MACRO "MI_MINIZ_API" EXPORT_FILENAME "Microcosm/miniz_Export.h")
microcosm_add_library("Quadrature" SHARED SOURCES "Quadrature.cc" EXPORT_MACRO "MI_QUADRATURE_API" EXPORT_FILENAME "Microcosm/Quadrature_Export.h")
microcosm_add_library("Serializer" SHARED SOURCES "Serializer.cc" DEPENDS ${PROJECT_NAME}::miniz EXPORT_MACRO "MI_SERIALIZER_API" EXPORT_FILENAME "Microcosm/Serializer_Export.h")
microcosm_add_library("stbi" SHARED SOURCES "stbi.cc" DEPENDS ${PROJECT_NAME}::miniz EXPORT_MACRO "MI_STBI_API" EXPORT_FILENAME "Microcosm/stbi_Export.h")
find_package(assimp QUIET)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(
    LIBAV IMPORTED_TARGET 
    libavformat libavcodec libavutil
    libswresample libswscale
    )
  if(LIBAV_FOUND)
    message(STATUS "Found FFMPEG (libav) libraries, enabling ${PROJECT_NAME}::Video.")
    microcosm_add_library("Video" SHARED SOURCES "Video.cc" DEPENDS PkgConfig::LIBAV EXPORT_MACRO "MI_VIDEO_API" EXPORT_FILENAME "Microcosm/Video_Export.h")
  endif()
endif()

# Recurse into sub-directories.
add_subdirectory(Application)
add_subdirectory(Geometry)
add_subdirectory(GPU)
add_subdirectory(Render)
add_subdirectory(UI)
add_subdirectory(unit_tests)
