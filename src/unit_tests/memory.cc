#include "Microcosm/memory"
#include "testing.h"

TEST_CASE("memory") {
  SUBCASE("DeepPtr") {
    mi::DeepPtr ptr(new int(3));
    mi::DeepPtr ptrCopy = ptr;
    SUBCASE("Copy") {
      CHECK(ptr != ptrCopy);
      CHECK(*ptr == *ptrCopy);
    }
    mi::DeepPtr ptrMove = std::move(ptr);
    SUBCASE("Move") {
      CHECK(ptr == nullptr);
      CHECK(ptrMove != ptrCopy);
      CHECK(*ptrMove == *ptrCopy);
    }
  }
  SUBCASE("RefPtr") {
    // Call counters
    auto prng = PRNG();
    static int ctorCalls = 0;
    static int dtorCalls = 0;
    struct Foo {
      Foo(int v = 0) : v(v) { ++ctorCalls; }
      ~Foo() { ++dtorCalls; }
      int v = 0;
    };
    {
      // Allocate and move around
      std::vector<mi::RefPtr<Foo>> foos;
      for (int repeat = 0; repeat < 8; repeat++) {
        for (int iter = 0; iter < 64; iter++) {
          foos.emplace_back(mi::make_ref<Foo>(prng()));
        }
        for (int iter = 0; iter < 8; iter++) {
          auto pos = prng(foos.size());
          auto foo = foos[pos];
          CHECK(foo.use_count() == 2);
          foos.erase(foos.begin() + pos);
          CHECK(foo.use_count() == 1);
          foos.push_back(foo);
        }
        std::sort(foos.begin(), foos.end(), [](auto lhs, auto rhs) { return lhs->v < rhs->v; });
      }
      // Let foos go out of scope to call destructors
    }
    CHECK(ctorCalls == dtorCalls);
  }
  SUBCASE("StaticQueue") {
    mi::StaticQueue<int, 8> queue;
    for (int k = 0; k < 8; k++) {
      // Should not throw yet.
      CHECK_NOTHROW(queue.push(k));
      // Back == most recently pushed.
      CHECK(queue.back() == k);
    }
    // Should now be full.
    CHECK(queue.full());
    // If full, push should throw.
    CHECK_THROWS(queue.push(8));
    for (int k = 0; k < 8; k++) {
      // Element at k should be k.
      CHECK(queue[k] == k);
      // Reverse indexing should also work.
      CHECK(queue[-(k + 1)] == 7 - k);
    }
    for (int k = 0; k < 8; k++) {
      // Front == least recently pushed.
      CHECK(queue.front() == k);
      // Pop should be least recently pushed.
      CHECK(queue.pop() == k);
    }
    // Should now be empty.
    CHECK(queue.empty());
    // If empty, front should still not throw.
    CHECK_NOTHROW(void(queue.front()));
    // If empty, back should still not throw.
    CHECK_NOTHROW(void(queue.back()));
    // If empty, pop should throw.
    CHECK_THROWS(queue.pop());
  }
  SUBCASE("StaticStack") {
    mi::StaticStack<int, 8> stack;
    for (int k = 0; k < 8; k++) {
      // Should not throw yet.
      CHECK_NOTHROW(stack.push(k));
      // Top == most recently pushed.
      CHECK(stack.back() == k);
    }
    // Should now be full.
    CHECK(stack.full());
    // If full, push should throw.
    CHECK_THROWS(stack.push(8));
    for (int k = 0; k < 8; k++) {
      // Element at k should be k.
      CHECK(stack[k] == k);
      // Reverse indexing should also work.
      CHECK(stack[-(k + 1)] == 7 - k);
    }
    for (int k = 7; k >= 0; k--) {
      // Top == most recently pushed.
      CHECK(stack.back() == k);
      // Pop should be most recently pushed.
      CHECK(stack.pop() == k);
    }
    // Should now be empty.
    CHECK(stack.empty());
    // If empty, front should still not throw.
    CHECK_NOTHROW(void(stack.front()));
    // If empty, back should still not throw.
    CHECK_NOTHROW(void(stack.back()));
    // If empty, pop should throw.
    CHECK_THROWS(stack.pop());
  }
}
