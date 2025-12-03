#pragma once

#include <array>
#include <cstddef>
#include <cstring>

namespace turbolog {

// Simple fixed-size log buffer.
// Template parameter SIZE defines the capacity in bytes.
// Efficient, non-copyable, C++17-compatible.
template <int SIZE>
class LogBuffer {
public:
    LogBuffer() : _cur(0) {}
    ~LogBuffer() = default;

    LogBuffer(const LogBuffer&) = delete;
    LogBuffer& operator=(const LogBuffer&) = delete;

    // Append data into the buffer. Requires len <= avail().
    // Copies exactly len bytes from buf.
    void append(const char* buf, std::size_t len) {
        // For performance, avoid branching; use a debug assert-like check.
        // In production, the caller should ensure enough space.
        // If you prefer a safe variant, change to copy the min.
#ifdef TURBOLOG_SAFE_APPEND
        const std::size_t n = len <= avail() ? len : avail();
        if (n == 0) return;
        std::memcpy(_data.data() + _cur, buf, n);
        _cur += n;
#else
        // Fast path: assume caller respects capacity.
        // Optionally, add runtime checks in debug builds.
        // Here we defensively no-op if overflow would occur.
        if (len > avail()) return;
        std::memcpy(_data.data() + _cur, buf, len);
        _cur += len;
#endif
    }

    // Return available space remaining.
    std::size_t avail() const { return SIZE - _cur; }

    // Return pointer to the contiguous data.
    const char* data() const { return _data.data(); }

    // Return current data length.
    std::size_t length() const { return _cur; }

    // Reset buffer (does not zero memory).
    void reset() { _cur = 0; }

    // Optional alias for reset if user prefers bzero semantics name.
    void bzero() { reset(); }

    // Capacity accessor.
    static constexpr std::size_t capacity() { return SIZE; }

private:
    std::array<char, SIZE> _data;
    std::size_t _cur;
};

} // namespace turbolog
