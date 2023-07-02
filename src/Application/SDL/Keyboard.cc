#include "Microcosm/Application/SDL/Keyboard"

namespace mi::SDL {

Scancode Key::scancode() const noexcept { return code.index() == 0 ? std::get<0>(code) : SDL_GetScancodeFromKey(std::get<1>(code)); }

Keycode Key::keycode() const noexcept { return code.index() == 1 ? std::get<1>(code) : SDL_GetKeyFromScancode(std::get<0>(code)); }

std::string_view Key::name() const noexcept { return code.index() == 0 ? SDL_GetScancodeName(std::get<0>(code)) : SDL_GetKeyName(std::get<1>(code)); }

Key::operator bool() const noexcept { return code.index() == 0 ? std::get<0>(code) != SDL_SCANCODE_UNKNOWN : std::get<1>(code) != SDLK_UNKNOWN; }

Keyboard::State Keyboard::getState() {
  static_assert(sizeof(Uint8) == 1);
  State state;
  int keyCount = 0;
  const Uint8 *keys = SDL_GetKeyboardState(&keyCount);
  state.keys = {reinterpret_cast<const uint8_t *>(keys), size_t(keyCount)};
  state.mods = SDL_GetModState();
  return state;
}

} // namespace mi::SDL
