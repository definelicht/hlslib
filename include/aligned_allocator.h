#pragma once

// Adapted from https://stackoverflow.com/a/12942652/2949968
// All credit to the original author.

#include <cstddef>
#include <memory>
#include <type_traits>

namespace detail {
template <size_t alignment>
void* allocate_aligned_memory(size_t size);
void deallocate_aligned_memory(void* ptr) noexcept;
}  // namespace detail

template <typename T, size_t alignment>
class aligned_allocator;

template <size_t alignment>
class aligned_allocator<void, alignment> {
 public:
  typedef void* pointer;
  typedef const void* const_pointer;
  typedef void value_type;

  template <class U>
  struct rebind {
    typedef aligned_allocator<U, alignment> other;
  };
};

template <typename T, size_t alignment>
class aligned_allocator {
 public:
  typedef T value_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  typedef std::true_type propagate_on_container_move_assignment;

  template <class U>
  struct rebind {
    typedef aligned_allocator<U, alignment> other;
  };

 public:
  aligned_allocator() noexcept {}

  template <class U>
  aligned_allocator(const aligned_allocator<U, alignment>&) noexcept {}

  size_type max_size() const noexcept {
    return (size_type(~0) - size_type(alignment)) / sizeof(T);
  }

  pointer address(reference x) const noexcept { return std::addressof(x); }

  const_pointer address(const_reference x) const noexcept {
    return std::addressof(x);
  }

  pointer allocate(
      size_type n,
      typename aligned_allocator<void, alignment>::const_pointer = 0) {
    void* ptr = detail::allocate_aligned_memory<alignment>(n * sizeof(T));
    if (ptr == nullptr) {
      throw std::bad_alloc();
    }

    return reinterpret_cast<pointer>(ptr);
  }

  void deallocate(pointer p, size_type) noexcept {
    return detail::deallocate_aligned_memory(p);
  }

  template <class U, class... Args>
  void construct(U* p, Args&&... args) {
    ::new (reinterpret_cast<void*>(p)) U(std::forward<Args>(args)...);
  }

  void destroy(pointer p) { p->~T(); }
};

template <typename T, size_t alignment>
class aligned_allocator<const T, alignment> {
 public:
  typedef T value_type;
  typedef const T* pointer;
  typedef const T* const_pointer;
  typedef const T& reference;
  typedef const T& const_reference;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  typedef std::true_type propagate_on_container_move_assignment;

  template <class U>
  struct rebind {
    typedef aligned_allocator<U, alignment> other;
  };

 public:
  aligned_allocator() noexcept {}

  template <class U>
  aligned_allocator(const aligned_allocator<U, alignment>&) noexcept {}

  size_type max_size() const noexcept {
    return (size_type(~0) - size_type(alignment)) / sizeof(T);
  }

  const_pointer address(const_reference x) const noexcept {
    return std::addressof(x);
  }

  pointer allocate(
      size_type n,
      typename aligned_allocator<void, alignment>::const_pointer = 0) {
    void* ptr = detail::allocate_aligned_memory<alignment>(n * sizeof(T));
    if (ptr == nullptr) {
      throw std::bad_alloc();
    }

    return reinterpret_cast<pointer>(ptr);
  }

  void deallocate(pointer p, size_type) noexcept {
    return detail::deallocate_aligned_memory(p);
  }

  template <class U, class... Args>
  void construct(U* p, Args&&... args) {
    ::new (reinterpret_cast<void*>(p)) U(std::forward<Args>(args)...);
  }

  void destroy(pointer p) { p->~T(); }
};

template <typename T, size_t TAlignment, typename U, size_t UAlignment>
inline bool operator==(const aligned_allocator<T, TAlignment>&,
                       const aligned_allocator<U, UAlignment>&) noexcept {
  return TAlignment == UAlignment;
}

template <typename T, size_t TAlignment, typename U, size_t UAlignment>
inline bool operator!=(const aligned_allocator<T, TAlignment>&,
                       const aligned_allocator<U, UAlignment>&) noexcept {
  return TAlignment != UAlignment;
}

template <size_t alignment>
void* detail::allocate_aligned_memory(size_t size) {
  if (size == 0) {
    return nullptr;
  }
  void* ptr = nullptr;
  int rc = posix_memalign(&ptr, alignment, size);
  if (rc != 0) {
    return nullptr;
  }
  return ptr;
}

inline void detail::deallocate_aligned_memory(void* ptr) noexcept {
  return free(ptr);
}
