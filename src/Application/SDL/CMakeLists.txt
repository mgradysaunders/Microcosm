find_package(SDL3 QUIET)
if (SDL3_FOUND)
  message(STATUS "Found SDL3, enabling ${PROJECT_NAME}::SDL.")
  microcosm_add_library(
    "SDL"
    SHARED
    SOURCES
      "common.cc"
      "Keyboard.cc"
      "Mouse.cc"
      "Window.cc"
    DEPENDS SDL3::SDL3
    EXPORT_MACRO "MI_SDL_API"
    EXPORT_FILENAME "Microcosm/Application/SDL/Export.h"
    )
endif()
