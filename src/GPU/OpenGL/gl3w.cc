#include "Microcosm/GPU/OpenGL/gl3w"
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

} // namespace mi::gl
