microcosm_add_library(
  "GPU_OpenGL"
  SHARED
  SOURCES
    "gl3w.c"
    "gl3w.cc"
    "ScreenQuad.cc"
  EXPORT_MACRO "MI_GL_API"
  EXPORT_FILENAME "Microcosm/GPU/OpenGL/Export.h"
  )
