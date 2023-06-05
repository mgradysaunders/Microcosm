#include "Microcosm/GPU/OpenGL/ScreenQuad"

namespace mi::gl {

static const char *vertShaderSrc = R"(
#version 450
out vec2 texcoord;
void main() {
  const vec2 verts[3] = vec2[3](
        vec2(-1, -1),
        vec2(+3, -1),
        vec2(-1, +3));
  gl_Position = vec4(verts[gl_VertexID], 0, 1);
  texcoord = 0.5 * gl_Position.xy + 0.5;
})";

static const char *fragShaderSrc = R"(
#version 450
layout (binding = 0) uniform sampler2D textureSampler;
layout (location = 0) out vec4 fragColor;
in vec2 texcoord;
void main() {
  fragColor = texture(textureSampler, texcoord);
})";

ScreenQuad::ScreenQuad()
  : mProgram(
      Shader(GL_VERTEX_SHADER, vertShaderSrc), //
      Shader(GL_FRAGMENT_SHADER, fragShaderSrc)) {}

ScreenQuad::ScreenQuad(GLuint fragShader) : mProgram(Shader(GL_VERTEX_SHADER, vertShaderSrc), fragShader) {}

} // namespace mi::gl
