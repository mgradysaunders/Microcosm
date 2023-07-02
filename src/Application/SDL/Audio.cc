#include "Microcosm/Application/SDL/Audio"

namespace mi::SDL {

int AudioStream::put(const void *buf, int len) {
  int result = SDL_AudioStreamPut(stream, buf, len);
  if (result < 0) throwError();
  return result;
}

int AudioStream::get(void *buf, int len) {
  int result = SDL_AudioStreamGet(stream, buf, len);
  if (result < 0) throwError();
  return result;
}

int AudioStream::available() {
  int result = SDL_AudioStreamAvailable(stream);
  if (result < 0) throwError();
  return result;
}

void AudioStream::flush() {
  int result = SDL_AudioStreamFlush(stream);
  if (result < 0) throwError();
}

void AudioStream::clear() noexcept { SDL_AudioStreamClear(stream); }

} // namespace mi::SDL
