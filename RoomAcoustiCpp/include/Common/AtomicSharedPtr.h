// Polyfill for std::atomic<std::shared_ptr<T>> — absent from Apple's libc++ as of 2025
// (the feature macro is present in the version table but commented out in the implementation).
// Guarded by the feature-test macro, so this is a no-op on platforms that implement it.

#pragma once

#if !defined(__cpp_lib_atomic_shared_ptr)

#include <atomic>
#include <memory>
#include <mutex>

namespace std {

template<typename T>
struct atomic<shared_ptr<T>> {
    using value_type = shared_ptr<T>;
    static constexpr bool is_always_lock_free = false;

    constexpr atomic() noexcept = default;
    atomic(shared_ptr<T> p) noexcept : ptr_(move(p)) {}
    atomic(const atomic&) = delete;
    void operator=(const atomic&) = delete;
    ~atomic() = default;

    bool is_lock_free() const noexcept { return false; }

    void store(shared_ptr<T> p, memory_order = memory_order_seq_cst) noexcept {
        lock_guard<mutex> g(mtx_);
        ptr_ = move(p);
    }

    shared_ptr<T> load(memory_order = memory_order_seq_cst) const noexcept {
        lock_guard<mutex> g(mtx_);
        return ptr_;
    }

    operator shared_ptr<T>() const noexcept { return load(); }
    void operator=(shared_ptr<T> p) noexcept { store(move(p)); }

    shared_ptr<T> exchange(shared_ptr<T> p,
                           memory_order = memory_order_seq_cst) noexcept {
        lock_guard<mutex> g(mtx_);
        ptr_.swap(p);
        return p;
    }

    bool compare_exchange_strong(shared_ptr<T>& expected, shared_ptr<T> desired,
                                 memory_order = memory_order_seq_cst,
                                 memory_order = memory_order_seq_cst) noexcept {
        lock_guard<mutex> g(mtx_);
        if (ptr_ == expected) { ptr_ = move(desired); return true; }
        expected = ptr_;
        return false;
    }

    bool compare_exchange_weak(shared_ptr<T>& expected, shared_ptr<T> desired,
                               memory_order s = memory_order_seq_cst,
                               memory_order f = memory_order_seq_cst) noexcept {
        return compare_exchange_strong(expected, move(desired), s, f);
    }

private:
    mutable mutex mtx_;
    shared_ptr<T> ptr_;
};

template<typename T>
inline shared_ptr<T> atomic_load(const atomic<shared_ptr<T>>* a) noexcept { return a->load(); }

template<typename T>
inline shared_ptr<T> atomic_load_explicit(const atomic<shared_ptr<T>>* a, memory_order mo) noexcept { return a->load(mo); }

template<typename T>
inline void atomic_store(atomic<shared_ptr<T>>* a, shared_ptr<T> p) noexcept { a->store(move(p)); }

template<typename T>
inline void atomic_store_explicit(atomic<shared_ptr<T>>* a, shared_ptr<T> p, memory_order mo) noexcept { a->store(move(p), mo); }

template<typename T>
inline shared_ptr<T> atomic_exchange(atomic<shared_ptr<T>>* a, shared_ptr<T> p) noexcept { return a->exchange(move(p)); }

template<typename T>
inline shared_ptr<T> atomic_exchange_explicit(atomic<shared_ptr<T>>* a, shared_ptr<T> p, memory_order mo) noexcept { return a->exchange(move(p), mo); }

} // namespace std

#endif // !defined(__cpp_lib_atomic_shared_ptr)
