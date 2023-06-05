find_package(SDL2 QUIET)
if (SDL2_FOUND)
  message(STATUS "Found SDL2, enabling ${PROJECT_NAME}::SDL.")
  microcosm_add_library(
    "SDL"
    SHARED
    SOURCES
      "Audio.cc"
      "common.cc"
      "Keyboard.cc"
      "Mouse.cc"
      "Window.cc"
    DEPENDS SDL2::SDL2
    EXPORT_MACRO "MI_SDL_API"
    EXPORT_FILENAME "Microcosm/Application/SDL/Export.h"
    )
endif()
