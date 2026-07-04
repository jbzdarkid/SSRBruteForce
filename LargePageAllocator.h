#pragma once

// Large-page ("huge page") allocator for Abseil's flat_hash containers.
//
// The state hashtable is a single multi-gigabyte allocation touched with random
// probes, so it is dominated by TLB misses. Backing it with 2 MB large pages
// instead of 4 KB pages shrinks the number of page-table entries by ~512x and
// removes most of those misses.
//
// Enable by defining USE_LARGE_PAGES (e.g. via the project's ExtraDefines or a
// /D flag). When it is NOT defined, LargePageAllocator is a thin alias for
// std::allocator and EnableLockMemoryPrivilege() is a no-op, so the rest of the
// codebase compiles and behaves exactly as before.
//
// Runtime requirements when enabled:
//   * The account must hold "Lock pages in memory" (SeLockMemoryPrivilege),
//     granted via secpol.msc -> Local Policies -> User Rights Assignment, then
//     re-login.
//   * Call EnableLockMemoryPrivilege() once at startup, before the table is
//     allocated. If the privilege is missing or a large-page allocation fails
//     (typically physical-memory fragmentation), the allocator transparently
//     falls back to an ordinary VirtualAlloc.

#include <memory> // std::allocator

#ifdef USE_LARGE_PAGES

#include <Windows.h>
#include <new> // std::bad_alloc

#pragma comment(lib, "advapi32.lib") // OpenProcessToken / LookupPrivilegeValue / AdjustTokenPrivileges

// Enable SeLockMemoryPrivilege for the current process. Returns true on success.
// Safe to call more than once. The account must also be granted the privilege
// via group policy, otherwise this returns false and large-page allocations
// will quietly fall back to normal pages.
inline bool EnableLockMemoryPrivilege() {
  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
    return false;
  }

  TOKEN_PRIVILEGES tp = {};
  tp.PrivilegeCount = 1;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  bool ok = false;
  if (LookupPrivilegeValue(nullptr, SE_LOCK_MEMORY_NAME, &tp.Privileges[0].Luid)) {
    // AdjustTokenPrivileges can "succeed" yet not assign every privilege, so we
    // must also check GetLastError().
    AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    ok = (GetLastError() == ERROR_SUCCESS);
  }

  CloseHandle(token);
  return ok;
}

// Standard-conforming, stateless allocator that backs its storage with large
// pages when possible and falls back to ordinary pages otherwise. Suitable as
// the Alloc template parameter of absl::flat_hash_set / absl::flat_hash_map.
template <typename T>
class LargePageAllocator {
public:
  using value_type = T;

  LargePageAllocator() noexcept = default;
  template <typename U>
  LargePageAllocator(const LargePageAllocator<U>&) noexcept {}

  [[nodiscard]] T* allocate(size_t n) {
    const size_t bytes = n * sizeof(T);

    const size_t largePage = GetLargePageMinimum(); // 0 => large pages unsupported
    if (largePage != 0) {
      const size_t rounded = (bytes + largePage - 1) & ~(largePage - 1);
      void* p = VirtualAlloc(nullptr, rounded,
                             MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
      if (p != nullptr) return static_cast<T*>(p);
      // Privilege missing or memory too fragmented for contiguous large pages;
      // fall through to a normal allocation.
    }

    void* p = VirtualAlloc(nullptr, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (p == nullptr) throw std::bad_alloc();
    return static_cast<T*>(p);
  }

  void deallocate(T* p, size_t /*n*/) noexcept {
    // MEM_RELEASE ignores the size and releases the whole reservation.
    if (p != nullptr) VirtualFree(p, 0, MEM_RELEASE);
  }

  template <typename U>
  bool operator==(const LargePageAllocator<U>&) const noexcept { return true; }
  template <typename U>
  bool operator!=(const LargePageAllocator<U>&) const noexcept { return false; }
};

#else // !USE_LARGE_PAGES

// No-op stub so callers never need their own #ifdef around startup code.
inline bool EnableLockMemoryPrivilege() { return false; }

// Behaviourally identical to the previous default allocator.
template <typename T>
using LargePageAllocator = std::allocator<T>;

#endif // USE_LARGE_PAGES
