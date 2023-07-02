#include "Microcosm/Application/SDL/Window"

#include <atomic>

namespace mi::SDL {

Window::Window(const char *title, Vector2i size, uint32_t flags) {
  window = SDL_CreateWindow(
    title,                   //
    SDL_WINDOWPOS_UNDEFINED, //
    SDL_WINDOWPOS_UNDEFINED, //
    size[0], size[1], flags | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_ALLOW_HIGHDPI);
  if (!window) throwError();
  setData("refCount", new std::atomic_int(1));

  // If created for OpenGL, create and store the context.
  if (flags & SDL_WINDOW_OPENGL) {
    auto context = SDL_GL_CreateContext(window);
    if (!context) throwError();
    setData("GLContext", context);
  }
}

std::vector<const char *> Window::vulkanInstanceExtensions() const {
  unsigned int count = 0;
  if (!SDL_Vulkan_GetInstanceExtensions(window, &count, nullptr)) throwError();
  std::vector<const char *> names(count);
  if (!SDL_Vulkan_GetInstanceExtensions(window, &count, names.data())) throwError();
  return names;
}

VkSurfaceKHR Window::vulkanCreateSurface(VkInstance instance) const {
  VkSurfaceKHR surface;
  if (!SDL_Vulkan_CreateSurface(window, instance, &surface)) throwError();
  return surface;
}

void Window::incrementRefCount() {
  if (window)
    if (auto refCount = getData<std::atomic_int>("refCount")) refCount->fetch_add(1);
}

void Window::decrementRefCount() {
  if (window) {
    if (auto refCount = getData<std::atomic_int>("refCount"); refCount && refCount->fetch_sub(1) == 1) {
      if (auto context = getData<void *>("GLContext")) SDL_GL_DeleteContext(context);
      SDL_DestroyWindow(window);
      window = nullptr;
      delete refCount;
    }
  }
}

} // namespace mi::SDL
