#include "Microcosm/Application/SDL/common"

#include <SDL2/SDL.h>
#include <SDL2/SDL_revision.h>
#include <SDL2/SDL_version.h>
#include <filesystem>
#include <iostream>

#include <signal.h>

namespace mi::SDL {

void throwError(const std::source_location &location) { throw Error(std::runtime_error(SDL_GetError()), location); }

Version::operator std::string() const {
  if (revision.empty())
    return "{}.{}.{}"_format(major, minor, patch);
  else
    return "{}.{}.{} ({})"_format(major, minor, patch, revision);
}

Version Version::running() noexcept {
  SDL_version version;
  SDL_GetVersion(&version);
  return {int(version.major), int(version.minor), int(version.patch), SDL_GetRevision()};
}

Version Version::builtAgainst() noexcept { return {SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL, SDL_REVISION}; }

static void LogOutput(void *, int, SDL_LogPriority priority, const char *message) {
  const char *label = "";
  switch (priority) {
  case SDL_LOG_PRIORITY_CRITICAL: label = "\e[91m\e[1m[Critical]\e[0m "; break;
  case SDL_LOG_PRIORITY_ERROR: label = "\e[91m[Error]\e[0m "; break;
  case SDL_LOG_PRIORITY_WARN: label = "\e[33m[Warn]\e[0m "; break;
  case SDL_LOG_PRIORITY_DEBUG: label = "\e[96m[Debug]\e[0m "; break;
  case SDL_LOG_PRIORITY_VERBOSE: // Same as Info
  case SDL_LOG_PRIORITY_INFO: label = "\e[92m[Info]\e[0m "; break;
  default: break;
  }
  while (message && *message != '\0') {
    const char *stop = message;
    while (*stop != '\n' && *stop != '\0') ++stop;
    std::cerr << label << std::string_view(message, stop) << '\n';
    if (*stop == '\0') break;
    message = stop + 1;
  }
}

// For Ctrl+C and OS kill signals, terminate immediately rather than
// generating an SDL_QUIT event.
static void sigKill(int sig) noexcept {
  std::cerr << "\e[91m\e[1m[Critical]\e[0m Killed ";
  std::cerr << (sig == SIGINT ? "(SIGINT)\n" : "(SIGTERM)\n");
  std::exit(0);
}

void init(uint32_t flags) {
  SDL_LogSetOutputFunction(LogOutput, nullptr);
  if (SDL_Init(flags) < 0) throwError();

  struct sigaction sa = {};
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = sigKill;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  // Log SDL version information.
  Log::debug("SDL Version:"
             "\n    Running       = {}"
             "\n    Built against = {}"_format(std::string(Version::running()), std::string(Version::builtAgainst())));

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
}

void quit() { SDL_Quit(); }

bool Clipboard::hasText() noexcept { return SDL_HasClipboardText(); }

void Clipboard::setText(const std::string &text) {
  if (SDL_SetClipboardText(text.c_str()) < 0) throwError();
}

std::string Clipboard::getText() {
  if (!SDL_HasClipboardText()) return {};
  auto text = SDL_GetClipboardText();
  auto copy = std::string(text);
  SDL_free(text);
  if (copy.empty()) throwError();
  return copy;
}

std::string Filesystem::basePath() {
  static const std::string path = [] {
    auto path = SDL_GetBasePath();
    auto copy = std::string(path);
    SDL_free(path);
    if (!std::filesystem::is_directory(copy)) throw std::logic_error("SDL_GetBasePath() returned an invalid path");
    return copy;
  }();
  return path;
}

std::string Filesystem::prefPath(const char *org, const char *app) {
  auto path = SDL_GetPrefPath(org, app);
  auto copy = std::string(path);
  SDL_free(path);
  if (!std::filesystem::is_directory(copy)) throw std::logic_error("SDL_GetPrefPath() returned an invalid path");
  return copy;
}

} // namespace mi::SDL
