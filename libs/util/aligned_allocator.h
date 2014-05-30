#ifndef MVE_ALIGNED_ALLOCATOR_HEADER
#define MVE_ALIGNED_ALLOCATOR_HEADER

#include <memory>
#include <vector>
#include <limits>
#include <cstdlib>

#include "util/defines.h"

UTIL_NAMESPACE_BEGIN

template <typename T_raw, size_t alignment>
struct AlignedAllocator {
#ifdef _MSC_VER
    typedef T_raw T __declspec(align(alignment));
#else
    typedef T_raw T __attribute__((aligned(alignment)));
#endif

    typedef T value_type;

    typedef T *pointer;
    typedef T const *const_pointer;

    typedef T &reference;
    typedef T const &const_reference;

    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    template<class U>
    struct rebind {
        typedef AlignedAllocator<T, alignment> other;
    };

    AlignedAllocator() { }

    template <class T_other, size_t alignment_other>
    AlignedAllocator(AlignedAllocator<T_other, alignment_other> const&) { }

    pointer allocate(size_type n) {
      if (n > std::numeric_limits<size_t>::max() / sizeof(T))
          throw std::bad_alloc();
      size_t size = n * sizeof(T);
      pointer p = NULL;
#ifdef _MSC_VER
      p = reinterpret_cast<pointer>(::_aligned_malloc(size, alignment));
      if (p == NULL)
          throw std::bad_alloc();
#else
      if (::posix_memalign(&reinterpret_cast<void*&>(p), alignment, size))
        throw std::bad_alloc();
#endif
      return p;
    }

    void deallocate(pointer p, size_type /* n */)
    {
#ifdef _MSC_VER
        ::_aligned_free(p);
#else
        ::free(p);
#endif
    }

    size_type max_size() const
    {
      return std::numeric_limits<size_t>::max() / sizeof(T);
    }

    void construct(pointer p, const_reference other) { ::new((void*)p) T(other); }
    void destroy(pointer p) { p->~T(); }

    template <class T_other, size_t alignment_other>
    bool operator==(AlignedAllocator<T_other, alignment_other> const&) { return true; }

    template <class T_other, size_t alignment_other>
    bool operator!=(AlignedAllocator<T_other, alignment_other> const&) { return false; }
};

UTIL_NAMESPACE_END

#endif // MVE_ALIGNED_ALLOCATOR_HEADER
