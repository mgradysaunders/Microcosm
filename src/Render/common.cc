#include "Microcosm/Render/common"

#include "fmt/chrono.h"
#include "fmt/color.h"
#include <iostream>

namespace mi::render {

void Any::throwLogicErrorBadCast(const std::type_info &type, const std::type_info &internType) {
  throw Error(std::logic_error("Can't access any as<{}>(): has type {}"_format( //
    typenameString(type), typenameString(internType))));
}

void AnyLookup::clear() noexcept { mLookup.clear(); }

bool AnyLookup::has(std::string_view name) const { return mLookup.find(name) != mLookup.end(); }

bool AnyLookup::has(std::string_view name, const std::type_info &type) const {
  auto itr = mLookup.find(name);
  if (itr == mLookup.end()) return false;
  return itr->second.type() == type;
}

void AnyLookup::throwLogicErrorNotFound(std::string_view name, const std::type_info &type) {
  throw Error(std::logic_error("Can't get<{}>({}): variable not found"_format(typenameString(type), show(name))));
}

void AnyLookup::throwLogicErrorBadCast(std::string_view name, const std::type_info &type, const std::type_info &internType) {
  throw Error(std::logic_error("Can't get<{}>({}): variable has type {}"_format( //
    typenameString(type), show(name), typenameString(internType))));
}

void Progress::increment() {
  constexpr uint64_t ProgressBarWidth{50};
  constexpr uint64_t NanosecPerSecond{1000000000ULL};
  constexpr uint64_t NanosecPerMinute{NanosecPerSecond * 60ULL};
  constexpr uint64_t NanosecPerHour{NanosecPerMinute * 60ULL};
  constexpr auto ColorDone{fmt::color::chartreuse};
  constexpr auto ColorTodo{fmt::color::coral};
  if (uint64_t count{++mCount}; count == mTotal || count % mPrintFrequency == 0) {
    double fraction{double(count) / double(mTotal)};
    std::string progressBarDone;
    std::string progressBarTodo;
    progressBarDone.reserve(ProgressBarWidth);
    progressBarTodo.reserve(ProgressBarWidth);
    uint64_t numDone{uint64_t(ProgressBarWidth * fraction)};
    uint64_t numTodo{uint64_t(ProgressBarWidth - numDone)};
    for (uint64_t i = 0; i < numDone; i++) progressBarDone += "━";
    for (uint64_t i = 0; i < numTodo; i++) progressBarTodo += "─";
    auto printTime = [&](uint64_t nanosec) -> std::string {
      if (nanosec < NanosecPerMinute) {
        return "{}s"_format(nanosec / NanosecPerSecond);
      } else if (nanosec < NanosecPerHour) {
        return "{}m:{:0>2d}s"_format(
          (nanosec / NanosecPerMinute), //
          (nanosec % NanosecPerMinute) / NanosecPerSecond);
      } else {
        return "{}h:{:0>2d}m:{:0>2d}s"_format(
          (nanosec / NanosecPerHour),                    //
          (nanosec % NanosecPerHour) / NanosecPerSecond, //
          (nanosec % NanosecPerMinute) / NanosecPerSecond);
      }
    };
    uint64_t nanosecDone = mTimer.nanoseconds();
    uint64_t nanosecTodo = nanosecDone / fraction - nanosecDone;
    std::cerr << "\33[2K\r{}: {: >5.1f}% | {}{} | 🕑 {}↑ {}↓ "_format(
      mHeading, 100 * fraction,                                //
      fmt::styled(progressBarDone, fmt::fg(ColorDone)),        //
      fmt::styled(progressBarTodo, fmt::fg(ColorTodo)),        //
      fmt::styled(printTime(nanosecDone), fmt::fg(ColorDone)), //
      fmt::styled(printTime(nanosecTodo), fmt::fg(ColorTodo)));
    if (count == mTotal) {
      std::cerr << "\nDone!\n";
    }
  }
}

} // namespace mi::render
