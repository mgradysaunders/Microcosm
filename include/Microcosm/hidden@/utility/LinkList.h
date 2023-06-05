/*-*- C++ -*-*/
#pragma once

#include <concepts>
#include <cstddef>
#include <iterator>

namespace mi {

namespace concepts {

template <typename T>
concept singly_linked = requires(T *link) {
  { link->next } -> std::convertible_to<T *>;
};

template <typename T>
concept doubly_linked = requires(T *link) {
  { link->prev } -> std::convertible_to<T *>;
  { link->next } -> std::convertible_to<T *>;
};

} // namespace concepts

struct ForwardLinkSentinel {};

template <typename Link> struct ForwardLinkIterator {
  using difference_type = ptrdiff_t;

  using value_type = Link *;

  using reference = Link *;

  using iterator_category = std::forward_iterator_tag;

  constexpr ForwardLinkIterator() noexcept = default;

  constexpr ForwardLinkIterator(Link *link) noexcept : link(link), next(link ? link->next : nullptr) {}

  constexpr Link *operator*() noexcept { return link; }

  constexpr ForwardLinkIterator &operator++() noexcept {
    link = next;
    next = link ? link->next : nullptr;
    return *this;
  }

  constexpr ForwardLinkIterator operator++(int) noexcept {
    ForwardLinkIterator copy = *this;
    operator++();
    return copy;
  }

  constexpr bool operator==(ForwardLinkSentinel) const noexcept { return link == nullptr; }

  constexpr bool operator!=(ForwardLinkSentinel) const noexcept { return link != nullptr; }

  constexpr operator Link *() noexcept { return link; }

  constexpr operator const Link *() const noexcept { return link; }

  Link *link{nullptr};
  Link *next{nullptr};
};

template <typename Subclass> struct SinglyLinked {
  Subclass *next{nullptr};
};

template <typename Subclass> struct DoublyLinked {
  Subclass *next{nullptr};
  Subclass *prev{nullptr};
};

template <typename Link> struct SinglyLinkedList {
public:
  using size_type = size_t;

  using iterator = ForwardLinkIterator<Link>;

  using sentinel = ForwardLinkSentinel;

  using const_iterator = ForwardLinkIterator<const Link>;

  using const_sentinel = ForwardLinkSentinel;

  constexpr SinglyLinkedList() noexcept = default;

  constexpr SinglyLinkedList(Link *link) noexcept {
    if (link) {
      head = link;
      for (link = head; link; link = link->next) count++;
    }
  }

  [[nodiscard]] constexpr size_t size() const noexcept { return count; }

  [[nodiscard]] constexpr bool empty() const noexcept { return count == 0; }

  [[nodiscard]] constexpr auto begin() noexcept { return ForwardLinkIterator(head); }

  [[nodiscard]] constexpr auto begin() const noexcept { return ForwardLinkIterator(head); }

  [[nodiscard]] constexpr auto end() noexcept { return ForwardLinkSentinel(); }

  [[nodiscard]] constexpr auto end() const noexcept { return ForwardLinkSentinel(); }

  constexpr Link *prepend(Link *link) noexcept {
    if (link) {
      ++count;
      link->next = head;
      head = link;
    }
    return link;
  }

  constexpr Link *append(Link *link) noexcept {
    if (link) {
      ++count;
      if (!head)
        head = link;
      else {
        Link *tail = head;
        while (tail->next) tail = tail->next;
        tail->next = link;
      }
      link->next = nullptr;
    }
    return link;
  }

  constexpr Link *extract(Link *link) noexcept {
    if (link) {
      if (head == link) {
        head = head->next;
      } else {
        Link *prev = head;
        while (prev && prev->next != link) prev = prev->next;
        if (prev)
          prev->next = link->next;
        else
          return nullptr; // Error
      }
      link->next = nullptr;
      --count;
    }
    return link;
  }

  constexpr Link *extract_head() noexcept { return extract(head); }

  constexpr operator Link *() const noexcept { return head; }

  Link *head{nullptr};
  size_t count{0};
};

template <typename Link> struct DoublyLinkedList {
public:
  using size_type = size_t;

  using iterator = ForwardLinkIterator<Link>;

  using sentinel = ForwardLinkSentinel;

  using const_iterator = ForwardLinkIterator<const Link>;

  using const_sentinel = ForwardLinkSentinel;

  constexpr DoublyLinkedList() noexcept = default;

  constexpr DoublyLinkedList(Link *link) noexcept {
    if (link) {
      head = link;
      tail = link;
      while (head->prev) head = head->prev;
      while (tail->next) tail = tail->next;
      for (link = head; link; link = link->next) count++;
    }
  }

  [[nodiscard]] constexpr size_t size() const noexcept { return count; }

  [[nodiscard]] constexpr bool empty() const noexcept { return count == 0; }

  [[nodiscard]] constexpr auto begin() noexcept { return ForwardLinkIterator(head); }

  [[nodiscard]] constexpr auto begin() const noexcept { return ForwardLinkIterator(head); }

  [[nodiscard]] constexpr auto end() noexcept { return ForwardLinkSentinel(); }

  [[nodiscard]] constexpr auto end() const noexcept { return ForwardLinkSentinel(); }

  constexpr Link *prepend(Link *link) noexcept {
    if (link) {
      ++count;
      if (!tail) tail = link;
      if (head) head->prev = link;
      link->next = head;
      head = link;
    }
    return link;
  }

  constexpr Link *append(Link *link) noexcept {
    if (link) {
      ++count;
      if (!head) head = link;
      if (tail) tail->next = link;
      link->prev = tail;
      tail = link;
    }
    return link;
  }

  constexpr Link *extract(Link *link) noexcept {
    if (link) {
      Link *prev = link->prev;
      Link *next = link->next;
      if (prev) prev->next = next;
      if (next) next->prev = prev;
      if (head == link) head = next;
      if (tail == link) tail = prev;
      link->prev = nullptr;
      link->next = nullptr;
      --count;
    }
    return link;
  }

  constexpr Link *extract_head() noexcept { return extract(head); }

  constexpr Link *extract_tail() noexcept { return extract(tail); }

  constexpr operator Link *() const noexcept { return head; }

  Link *head{nullptr};
  Link *tail{nullptr};
  size_t count{0};
};

} // namespace mi
