#ifndef ATG_ENGINE_SIM_RING_BUFFER_H
#define ATG_ENGINE_SIM_RING_BUFFER_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>

template <typename T_Data>
class RingBuffer {
public:
    RingBuffer() = default;
    ~RingBuffer() { destroy(); }

    void initialize(size_t capacity) {
        destroy();
        if (capacity == 0) return;
        m_buffer = new T_Data[capacity];
        m_capacity = capacity;
        m_writeIndex = 0;
        m_start = 0;
        m_count = 0;
    }

    void destroy() {
        delete[] m_buffer;
        m_buffer = nullptr;
        m_capacity = 0;
        m_writeIndex = 0;
        m_start = 0;
        m_count = 0;
    }

    bool write(T_Data data) {
        if (m_count >= m_capacity || m_capacity == 0) return false;
        m_buffer[m_writeIndex] = data;
        m_writeIndex = (m_writeIndex + 1) % m_capacity;
        ++m_count;
        return true;
    }

    void overwrite(T_Data data, size_t offset) {
        assert(offset < m_count);
        m_buffer[(m_start + offset) % m_capacity] = data;
    }

    size_t index(size_t base, int offset) const {
        if (m_capacity == 0) return 0;
        const long long raw = static_cast<long long>(base) + offset;
        const long long cap = static_cast<long long>(m_capacity);
        return static_cast<size_t>((raw % cap + cap) % cap);
    }

    T_Data read(size_t offset) const {
        assert(offset < m_count);
        return m_buffer[(m_start + offset) % m_capacity];
    }

    void read(size_t n, T_Data *target) const {
        assert(n <= m_count);
        copyOut(n, target);
    }

    void readAndRemove(size_t n, T_Data *target) {
        assert(n <= m_count);
        copyOut(n, target);
        removeBeginning(n);
    }

    void removeBeginning(size_t n) {
        assert(n <= m_count);
        if (m_capacity != 0) m_start = (m_start + n) % m_capacity;
        m_count -= n;
        if (m_count == 0) m_start = m_writeIndex;
    }

    size_t size() const { return m_count; }
    size_t capacity() const { return m_capacity; }
    size_t freeSpace() const { return m_capacity - m_count; }
    bool empty() const { return m_count == 0; }
    bool full() const { return m_count == m_capacity && m_capacity != 0; }
    size_t writeIndex() const { return m_writeIndex; }
    size_t start() const { return m_start; }

private:
    void copyOut(size_t n, T_Data *target) const {
        if (n == 0) return;
        const size_t first = std::min(n, m_capacity - m_start);
        std::memcpy(target, m_buffer + m_start, first * sizeof(T_Data));
        if (n > first) std::memcpy(target + first, m_buffer, (n - first) * sizeof(T_Data));
    }

    T_Data *m_buffer = nullptr;
    size_t m_capacity = 0;
    size_t m_writeIndex = 0;
    size_t m_start = 0;
    size_t m_count = 0;
};

#endif
