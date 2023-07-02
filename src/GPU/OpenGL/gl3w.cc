#include "Microcosm/GPU/OpenGL/gl3w"
#include "Microcosm/utility"
#include "fmt/color.h"
#include <iostream>

namespace mi::gl {

void initOrExit() {
  if (int err = gl3wInit(); err != GL3W_OK) {
    std::cerr << "Error! gl3wInit() failed.\n";
    std::cerr << "Error code = ";
    if (err == GL3W_ERROR_INIT)
      std::cerr << "GL3W_ERROR_INIT\n";
    else if (err == GL3W_ERROR_LIBRARY_OPEN)
      std::cerr << "GL3W_ERROR_LIBRARY_OPEN\n";
    else if (err == GL3W_ERROR_OPENGL_VERSION)
      std::cerr << "GL3W_ERROR_OPENGL_VERSION\n";
    else
      std::cerr << err << " (Unknown)\n";
    std::exit(err);
  }
  if (!gl3wIsSupported(4, 5)) {
    GLint major = getInteger(GL_MAJOR_VERSION);
    GLint minor = getInteger(GL_MINOR_VERSION);
    std::cerr << "Error! OpenGL version >=4.5 is required. (Loaded version = " << major << "." << minor << ")";
    std::exit(GL3W_ERROR_OPENGL_VERSION);
  }

  // Set up sensible default configuration. Disable depth, stencil, and scissor tests, and
  // enable back-face culling and ordinary alpha blending.
  disable(GL_DEPTH_TEST, GL_STENCIL_TEST, GL_SCISSOR_TEST);
  enable(GL_CULL_FACE, GL_BLEND);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glClearColor(0, 0, 0, 0);
}

static void debugPrinter(GLenum source, GLenum type, GLuint, GLenum severity, GLsizei, const GLchar *message, const void *) {
  const char *sourceStr = "Other?";
  switch (source) {
  default: break;
  case GL_DEBUG_SOURCE_API: sourceStr = "API"; break;
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceStr = "Window System"; break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceStr = "Shader Compiler"; break;
  case GL_DEBUG_SOURCE_THIRD_PARTY: sourceStr = "Third Party"; break;
  case GL_DEBUG_SOURCE_APPLICATION: sourceStr = "Application"; break;
  case GL_DEBUG_SOURCE_OTHER: sourceStr = "Other"; break;
  }
  const char *typeStr = "Other?";
  switch (type) {
  default: break;
  case GL_DEBUG_TYPE_ERROR: typeStr = "Error"; break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "Deprecated"; break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: typeStr = "Undefined"; break;
  case GL_DEBUG_TYPE_PORTABILITY: typeStr = "Portability"; break;
  case GL_DEBUG_TYPE_PERFORMANCE: typeStr = "Performance"; break;
  case GL_DEBUG_TYPE_MARKER: typeStr = "Marker"; break;
  case GL_DEBUG_TYPE_PUSH_GROUP: typeStr = "Push Group"; break;
  case GL_DEBUG_TYPE_POP_GROUP: typeStr = "Pop Group"; break;
  case GL_DEBUG_TYPE_OTHER: typeStr = "Other"; break;
  }
  fmt::color color{};
  switch (severity) {
  case GL_DEBUG_SEVERITY_HIGH: color = fmt::color::red; break;
  case GL_DEBUG_SEVERITY_MEDIUM: color = fmt::color::orange_red; break;
  case GL_DEBUG_SEVERITY_LOW: color = fmt::color::light_yellow; break;
  case GL_DEBUG_SEVERITY_NOTIFICATION: color = fmt::color::lime_green; break;
  }
  std::cerr << fmt::format("{} from {}: {}\n", fmt::format(fmt::fg(color), "[OpenGL] [{}]", typeStr), sourceStr, message);
}

void initDebugPrinting() {
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(debugPrinter, nullptr);
}

} // namespace mi::gl
