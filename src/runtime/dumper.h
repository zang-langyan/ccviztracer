#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <utility>
#include <thread>

namespace cctracer {

template <typename T>
class MPSCQueue {
public:
    MPSCQueue() {
        Node* dummy = new Node();
        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
        size_.store(0, std::memory_order_relaxed);
    }

    ~MPSCQueue() {
        T dummy;
        while (try_dequeue(dummy)) {}
        Node* node = head_.load(std::memory_order_relaxed);
        while (node) {
            Node* next = node->next.load(std::memory_order_relaxed);
            delete node;
            node = next;
        }
    }

    MPSCQueue(const MPSCQueue&) = delete;
    MPSCQueue& operator=(const MPSCQueue&) = delete;
    MPSCQueue(MPSCQueue&&) = delete;
    MPSCQueue& operator=(MPSCQueue&&) = delete;

    /**
     * thread-safe producer
     */
    void enqueue(T&& item) {
        Node* node = new Node(std::move(item));
        Node* prev_tail = tail_.exchange(node, std::memory_order_acq_rel);
        prev_tail->next.store(node, std::memory_order_release);
        size_.fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * consumer: try dequeue called by one consumer
     * @param item output
     * @return true: successful dequeue, false: queue is empty
     */
    bool try_dequeue(T& item) {
        Node* head = head_.load(std::memory_order_relaxed);
        Node* next = head->next.load(std::memory_order_acquire);
        
        if (next == nullptr) {
            return false;
        }

        item = std::move(next->data);
        head_.store(next, std::memory_order_release);
        delete head;
        return true;
    }

    /**
     * consumer: block and wait until has element to dequeue or notified to stop
     * @param stop_flag when true and queue is empty, return false
     * @param item output
     * @return true: successful dequeue, false: stop and quit
     */
    bool dequeue(std::atomic<bool>& stop_flag, T& item) {
        while (true) {
            if (try_dequeue(item)) {
                return true;
            }
            if (stop_flag.load(std::memory_order_acquire)) {
                return false;
            }
            std::unique_lock<std::mutex> lock(cv_mutex_);
            if (head_.load(std::memory_order_relaxed)->next.load(std::memory_order_acquire) != nullptr) {
                continue;
            }
            if (stop_flag.load(std::memory_order_acquire)) {
                return false;
            }
            cv_.wait(lock);
        }
    }

    /**
     * block dequeue with timeout
     * @return true: successful dequeue；false: timeout or stop
     */
    template <typename Rep, typename Period>
    bool dequeue_for(std::atomic<bool>& stop_flag,
                     T& item,
                     const std::chrono::duration<Rep, Period>& timeout) {
        auto deadline = std::chrono::steady_clock::now() + timeout;
        while (true) {
            if (try_dequeue(item)) {
                return true;
            }
            if (stop_flag.load(std::memory_order_acquire)) {
                return false;
            }
            std::unique_lock<std::mutex> lock(cv_mutex_);
            if (head_.load(std::memory_order_relaxed)->next.load(std::memory_order_acquire) != nullptr) {
                continue;
            }
            if (stop_flag.load(std::memory_order_acquire)) {
                return false;
            }
            if (cv_.wait_until(lock, deadline) == std::cv_status::timeout) {
                return false;
            }
        }
    }

    /**
     * outside waken consumer
     */
    void notify_consumer() {
        cv_.notify_one();
    }

    /**
     * approximate size in queue
     */
    size_t size_approx() const {
        return size_.load(std::memory_order_relaxed);
    }

private:
    struct Node {
        T data;
        std::atomic<Node*> next;

        Node() : data(), next(nullptr) {}
        explicit Node(T&& item) : data(std::move(item)), next(nullptr) {}
    };

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;

    std::mutex cv_mutex_;
    std::condition_variable cv_;

    std::atomic<size_t> size_;
};

/**
 * "{\"ph\":\"X\",\"pid\":%d,\"tid\":%u,\"ts\":%llu,\"dur\":%llu,\"name\":\"%s\",\"args\":{\"file\":\"%s\",\"line\":%d,\"column\":%d}},",
 */
struct TraceEvent {
    enum eventType {
        X,
        B,
        E,
    } ph;
    int pid; // process id
    uint32_t tid; // thread id
    uint64_t ts;  // begin time
    uint64_t dur; // duration
    int line; // line
    int column; // column
    const char* func_name;
    const char* file_name;
};

class Dumper {
private:
    MPSCQueue<std::vector<TraceEvent>> _q;
    std::atomic<bool> _stop;
    bool is_init;
    std::thread _dumping_thread;
    FILE* _f = nullptr;
public:
    Dumper() : _stop(false), is_init(false)
    {}

    int preinit(FILE* dumpfile) {
        _f = dumpfile;
        if (!_f) {
            std::cerr << "Fail to open dump file.\n";
            return -1;
        }
        return 0;
    }
    
    int initialize() {
        if (!_f) return -1;
        _dumping_thread = std::thread(&Dumper::dumping_worker, this);
        is_init = true;
        fprintf(_f, "{\"traceEvents\":[");
        return 0;
    }
    
    void stop() {
        if (is_init) {
            _stop = true;
            _q.notify_consumer();
            if (_dumping_thread.joinable()) {
                _dumping_thread.join();
            }
        }
        fprintf(_f, "]}");
        fclose(_f);
    }

    void dumpEvents(std::vector<TraceEvent>&& e) {
        _q.enqueue(std::move(e));
        _q.notify_consumer();
    }

private:
    void dumping_worker() {
        std::vector<TraceEvent> events;
        while (_q.dequeue(_stop, events)) {
            if (!_f) {
                std::cerr << "File handler is null, quit dumping..\n";
            }
#define DUMP_FORMAT \
    "{\"ph\":\"X\",\"pid\":%d,\"tid\":%u,\"ts\":%llu,\"dur\":%llu,\"name\":\"%s\",\"args\":{\"file\":\"%s\",\"line\":%d,\"column\":%d}},"
            for (auto&& e : events) {
                fprintf(
                    _f,
                    DUMP_FORMAT,
                    e.pid, e.tid, e.ts, e.dur, e.func_name, e.file_name, e.line, e.column
                );
            }
        }
    }
};

} // namespace cctracer
