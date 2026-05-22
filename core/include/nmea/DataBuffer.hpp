#pragma once

#include <array>
#include <condition_variable>
#include <mutex>
#include <optional>

namespace nmea {

/**
 * @brief Thread-safe, fixed-capacity ring buffer using RAII and C++17.
 *
 * Designed for producer/consumer patterns in embedded-style pipelines where
 * dynamic allocation must be avoided after initialisation. A typical use case
 * is buffering telemetry frames between the I/O thread and the processing core.
 *
 * @tparam T    Element type. Must be move-constructible.
 * @tparam Cap  Maximum number of elements (compile-time constant).
 */
template <typename T, std::size_t Cap>
class DataBuffer {
    static_assert(Cap > 0, "DataBuffer capacity must be greater than zero");

public:
    DataBuffer() = default;

    // Non-copyable, non-movable — owns the mutex and CV.
    DataBuffer(const DataBuffer&)            = delete;
    DataBuffer& operator=(const DataBuffer&) = delete;
    DataBuffer(DataBuffer&&)                 = delete;
    DataBuffer& operator=(DataBuffer&&)      = delete;

    // ── Producer side ──────────────────────────────────────────────────────

    /**
     * @brief Push a new element. Blocks the caller until space is available.
     * @param item  Value to store (moved in).
     */
    void push(T item)
    {
        std::unique_lock lock{m_mutex};
        m_notFull.wait(lock, [this] { return m_size < Cap; });

        m_buffer[m_head] = std::move(item);
        m_head           = (m_head + 1) % Cap;
        ++m_size;

        m_notEmpty.notify_one();
    }

    /**
     * @brief Try to push without blocking.
     * @return true if the element was inserted, false if the buffer was full.
     */
    bool tryPush(T item)
    {
        std::lock_guard lock{m_mutex};
        if (m_size >= Cap) return false;

        m_buffer[m_head] = std::move(item);
        m_head           = (m_head + 1) % Cap;
        ++m_size;

        m_notEmpty.notify_one();
        return true;
    }

    // ── Consumer side ──────────────────────────────────────────────────────

    /**
     * @brief Pop the oldest element. Blocks until one is available.
     */
    T pop()
    {
        std::unique_lock lock{m_mutex};
        m_notEmpty.wait(lock, [this] { return m_size > 0; });

        T item = std::move(m_buffer[m_tail]);
        m_tail = (m_tail + 1) % Cap;
        --m_size;

        m_notFull.notify_one();
        return item;
    }

    /**
     * @brief Try to pop without blocking.
     * @return The element if available, std::nullopt otherwise.
     */
    std::optional<T> tryPop()
    {
        std::lock_guard lock{m_mutex};
        if (m_size == 0) return std::nullopt;

        T item = std::move(m_buffer[m_tail]);
        m_tail = (m_tail + 1) % Cap;
        --m_size;

        m_notFull.notify_one();
        return item;
    }

    // ── Observers ──────────────────────────────────────────────────────────

    [[nodiscard]] std::size_t size() const
    {
        std::lock_guard lock{m_mutex};
        return m_size;
    }

    [[nodiscard]] bool empty() const
    {
        std::lock_guard lock{m_mutex};
        return m_size == 0;
    }

    [[nodiscard]] bool full() const
    {
        std::lock_guard lock{m_mutex};
        return m_size >= Cap;
    }

    static constexpr std::size_t capacity() noexcept { return Cap; }

private:
    mutable std::mutex      m_mutex;
    std::condition_variable m_notEmpty;
    std::condition_variable m_notFull;

    std::array<T, Cap> m_buffer{};
    std::size_t        m_head{0};
    std::size_t        m_tail{0};
    std::size_t        m_size{0};
};

} // namespace nmea