#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

namespace kaiserqzyue {
struct control_block {
    std::atomic_size_t ref_count_{};
    std::atomic_size_t weak_count_{};

public:
    void increment() { ref_count_.fetch_add(1, std::memory_order_relaxed); }
    bool decrement() {
        auto old = ref_count_.fetch_sub(1, std::memory_order_release);
        if (old == 1) { std::atomic_thread_fence(std::memory_order_acquire); }
        return old == 1;
    }
    void weak_increment() { weak_count_.fetch_add(1, std::memory_order_relaxed); }
    bool weak_decrement() {
        auto old = weak_count_.fetch_sub(1, std::memory_order_release);
        if (old == 1) { std::atomic_thread_fence(std::memory_order_acquire); }
        return old == 1;
    }
    std::size_t count() const { return ref_count_.load(std::memory_order_relaxed); }
    std::size_t weak_count() const { return weak_count_.load(std::memory_order_relaxed); }
};

template <typename T>
class weak_ptr;
template <typename T>
class shared_ptr {
    friend class weak_ptr<T>;

public:
    shared_ptr() = default;
    explicit shared_ptr(T *p) : ptr(p) {
        if (!ptr) { return; }
        ctrl_blk = new control_block();
        ctrl_blk->increment();
        ctrl_blk->weak_increment();
    }
    ~shared_ptr() { check_and_delete(); }

    shared_ptr(const shared_ptr &other) : ctrl_blk(other.ctrl_blk), ptr(other.ptr) {
        if (ctrl_blk) { ctrl_blk->increment(); }
    }
    shared_ptr(shared_ptr &&other) noexcept : ctrl_blk(other.ctrl_blk), ptr(other.ptr) {
        other.ctrl_blk = nullptr;
        other.ptr      = nullptr;
    }

    shared_ptr &operator=(const shared_ptr &other) {
        if (this == &other) { return *this; }
        check_and_delete();
        ctrl_blk = other.ctrl_blk;
        ptr      = other.ptr;
        if (ctrl_blk) { ctrl_blk->increment(); }
        return *this;
    }
    shared_ptr &operator=(shared_ptr &&other) noexcept {
        if (this == &other) { return *this; }
        check_and_delete();
        ctrl_blk       = other.ctrl_blk;
        ptr            = other.ptr;
        other.ctrl_blk = nullptr;
        other.ptr      = nullptr;
        return *this;
    }

    T *get() const { return ptr; }
    T &operator*() const { return *ptr; }
    T *operator->() const { return ptr; }
    shared_ptr<T> &reset(T *p = nullptr) {
        check_and_delete();
        if (!p) {
            ctrl_blk = nullptr;
            ptr      = nullptr;
            return *this;
        }
        ptr      = p;
        ctrl_blk = new control_block();
        ctrl_blk->increment();
        ctrl_blk->weak_increment();
        return *this;
    }
    std::size_t use_count() const { return ctrl_blk ? ctrl_blk->count() : 0; }
    std::size_t weak_count() const { return ctrl_blk ? ctrl_blk->weak_count() : 0; }
    explicit operator bool() const { return ptr != nullptr; }

private:
    shared_ptr(const weak_ptr<T> &wp);
    void check_and_delete() {
        if (!ctrl_blk) { return; }
        if (ctrl_blk->decrement()) {
            delete ptr;
            if (ctrl_blk->weak_decrement()) { delete ctrl_blk; }
            return;
        }
    }
    control_block *ctrl_blk{};
    T *ptr{};
};

template <typename T>
class weak_ptr {
    friend class shared_ptr<T>;

public:
    weak_ptr() = default;
    weak_ptr(const shared_ptr<T> &sp) : ctrl_blk(sp.ctrl_blk), ptr(sp.ptr) {
        if (ctrl_blk) { ctrl_blk->weak_increment(); }
    }
    weak_ptr(const weak_ptr &other) : ctrl_blk(other.ctrl_blk), ptr(other.ptr) {
        if (ctrl_blk) { ctrl_blk->weak_increment(); }
    }
    weak_ptr(weak_ptr &&other) noexcept : ctrl_blk(other.ctrl_blk), ptr(other.ptr) {
        other.ctrl_blk = nullptr;
        other.ptr      = nullptr;
    }
    ~weak_ptr() { check_and_delete(); }

    weak_ptr &operator=(const weak_ptr &other) {
        if (this == &other) { return *this; }
        check_and_delete();
        ctrl_blk = other.ctrl_blk;
        ptr      = other.ptr;
        if (ctrl_blk) { ctrl_blk->weak_increment(); }
        return *this;
    }
    weak_ptr &operator=(weak_ptr &&other) noexcept {
        if (this == &other) { return *this; }
        check_and_delete();
        ctrl_blk       = other.ctrl_blk;
        ptr            = other.ptr;
        other.ctrl_blk = nullptr;
        other.ptr      = nullptr;
        return *this;
    }

    std::size_t use_count() const { return ctrl_blk ? ctrl_blk->count() : 0; }
    std::size_t weak_count() const { return ctrl_blk ? ctrl_blk->weak_count() : 0; }
    bool expired() const { return use_count() == 0; }
    shared_ptr<T> lock() const {
        auto old = use_count();
        do {
            if (old == 0) { return shared_ptr<T>(); }
        } while (ctrl_blk && !ctrl_blk->ref_count_.compare_exchange_strong(
                                 old, old + 1, std::memory_order_relaxed));
        std::atomic_thread_fence(std::memory_order_acquire);
        return shared_ptr<T>(*this);
    }

private:
    void check_and_delete() {
        if (!ctrl_blk) { return; }
        if (ctrl_blk->weak_count() == 1) {
            delete ctrl_blk;
        } else if (ctrl_blk->weak_decrement()) {
            delete ctrl_blk;
        }
    }
    control_block *ctrl_blk{};
    T *ptr{};
};

template <typename T>
shared_ptr<T>::shared_ptr(const weak_ptr<T> &wp) : ctrl_blk(wp.ctrl_blk), ptr(wp.ptr) {}
}  // namespace kaiserqzyue

struct TestObject {
    int value = 0;
    std::atomic_bool ready{};
    ~TestObject() { std::cout << "TestObject destroyed" << std::endl; }
};

void print_thread_info(const std::string &msg) {
    std::cout << "[" << std::this_thread::get_id() << "] " << msg << std::endl;
}

using kaiserqzyue::shared_ptr, kaiserqzyue::weak_ptr;

void test_empty_shared_ptr_use_count() {
    print_thread_info("Testing empty shared_ptr use_count...");
    shared_ptr<TestObject> sp(nullptr);
    assert(sp.use_count() == 0);
    print_thread_info("Empty shared_ptr use_count test passed");
}

void test_weak_ptr_lock_multithread() {
    print_thread_info("Testing weak_ptr::lock() multithread...");

    constexpr int thread_count = 5;

    std::vector<std::thread> threads;
    shared_ptr<TestObject> sp(new TestObject);
    weak_ptr<TestObject> wp(sp);

    for (int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10 * rand() % 100));
            shared_ptr<TestObject> locked = wp.lock();
            if (locked) {
                print_thread_info("Lock succeeded, object still alive");
                assert(locked->value == 0);
            } else {
                print_thread_info("Lock failed, object expired");
            }
        });
    }

    print_thread_info("Main thread releasing strong reference...");
    sp.reset();

    for (auto &t : threads) { t.join(); }

    assert(wp.expired() == true);
    print_thread_info("weak_ptr::lock() multithread test passed");
}

void test_control_block_lifetime() {
    print_thread_info("Testing control block lifetime...");

    std::vector<std::thread> threads;
    weak_ptr<TestObject> wp;

    threads.emplace_back([&]() {
        shared_ptr<TestObject> sp(new TestObject);
        wp = sp;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        print_thread_info("Thread 1 releasing shared_ptr");
    });

    threads.emplace_back([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        shared_ptr<TestObject> sp2 = wp.lock();
        if (sp2) {
            print_thread_info("Thread 2 acquired shared_ptr from weak_ptr");
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            print_thread_info("Thread 2 releasing shared_ptr2");
        } else {
            print_thread_info("Thread 2 failed to acquire shared_ptr");
        }
    });

    for (auto &t : threads) { t.join(); }

    assert(wp.expired() == true);
    print_thread_info("Control block lifetime test passed");
}

void test_memory_visibility() {
    print_thread_info("Testing memory visibility...");

    const int iterations = 1000;
    shared_ptr<TestObject> sp(new TestObject);
    weak_ptr<TestObject> wp(sp);

    std::thread writer([&]() {
        for (int i = 0; i < iterations; ++i) {
            sp->value = i;
            std::this_thread::yield();
        }
        sp->ready.store(true, std::memory_order_release);
    });

    std::thread reader([&]() {
        while (!sp->ready.load(std::memory_order_acquire)) { std::this_thread::yield(); }
        shared_ptr<TestObject> locked = wp.lock();
        if (locked) {
            assert(locked->value == iterations - 1);
        } else {
            assert(wp.expired());
        }
    });

    writer.join();
    reader.join();

    print_thread_info("Memory visibility test passed");
}

int main() {
    test_empty_shared_ptr_use_count();
    test_weak_ptr_lock_multithread();
    test_control_block_lifetime();
    test_memory_visibility();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
