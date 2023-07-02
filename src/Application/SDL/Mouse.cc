#include "Microcosm/Application/SDL/Mouse"

#include <SDL2/SDL.h>

namespace mi::SDL {

Mouse::State Mouse::getState() noexcept {
  State state;
  state.mode = getDefaultMode();
  state.mask = SDL_GetMouseState(&state.position[0], &state.position[1]);
  return state;
}

Mouse::State Mouse::getState(State::Mode mode) noexcept {
  State state;
  state.mode = mode;
  state.mask = mode == State::Mode::Relative ? SDL_GetRelativeMouseState(&state.position[0], &state.position[1]) : SDL_GetGlobalMouseState(&state.position[0], &state.position[1]);
  return state;
}

Mouse::State::Mode Mouse::getDefaultMode() noexcept { return SDL_GetRelativeMouseMode() ? State::Mode::Relative : State::Mode::Global; }

void Mouse::setDefaultMode(State::Mode mode) {
  using enum State::Mode;
  int code = SDL_SetRelativeMouseMode(mode == Relative ? SDL_TRUE : SDL_FALSE);
  if (code < 0) throwError();
}

void Mouse::capture(bool flag) {
  int code = SDL_CaptureMouse(flag ? SDL_TRUE : SDL_FALSE);
  if (code < 0) throwError();
}

void Mouse::show() {
  if (SDL_ShowCursor(SDL_ENABLE) < 0) throwError();
}

void Mouse::hide() {
  if (SDL_ShowCursor(SDL_DISABLE) < 0) throwError();
}

bool Mouse::shown() {
  int code = SDL_ShowCursor(SDL_QUERY);
  if (code < 0) throwError();
  return code == SDL_ENABLE;
}

void Mouse::warpTo(const Window &window, Vector2i position) { SDL_WarpMouseInWindow(window, position[0], position[1]); }

void Mouse::warpTo(Vector2i position) {
  int code = SDL_WarpMouseGlobal(position[0], position[1]);
  if (code < 0) throwError();
}

} // namespace mi::SDL
