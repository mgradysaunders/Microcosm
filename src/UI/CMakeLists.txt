microcosm_add_library(
  "UI"
  SHARED
  SOURCES
    "Color.cc"
    "Context.cc"
    "DrawCmds.cc"
    "Font.cc"
    "Keyboard.cc"
    "Mouse.cc"
    "Panel.cc"
    "Rect.cc"
    "Text.cc"
    "Theme.cc"
    "Transform.cc"
  EXPORT_MACRO "MI_UI_API"
  EXPORT_FILENAME "Microcosm/UI/Export.h"
  )
add_subdirectory(Artist)
add_subdirectory(Behavior)
add_subdirectory(unit_tests)
