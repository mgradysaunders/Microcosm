#include "Microcosm/UI/Keyboard"
#include "Microcosm/UI/Clock"

namespace mi::ui {

void Keyboard::clear() noexcept {
  for (auto &key : keys) key = {};
}

void Keyboard::start() noexcept {
  for (auto &key : keys) key.down = (key.down << 1) | (key.down & 1);
  inputText.clear();
}

void Keyboard::afterInput(const Clock &clock) noexcept {
  for (auto &key : keys) {
    if (key.isDown()) {
      if (key.isJustDown()) {
        key.repeat = 0, key.repeatTimer = 500'000;
      } else {
        key.repeat = 0, key.repeatTimer -= clock.deltaTicks;
        if (key.repeatTimer < 0) key.repeat = 1, key.repeatTimer = 50'000;
      }
    } else {
      key.repeat = 0, key.repeatTimer = 0;
    }
  }
}

} // namespace mi::ui
