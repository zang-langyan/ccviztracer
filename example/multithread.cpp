
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <random>
#include <atomic>
#include <functional>
#include <future>


class Worker {
public:
    Worker(int id) : id_(id), workload_(0) {}

    
    void doWork(int iterations) {
        for (int i = 0; i < iterations; ++i) {
            std::this_thread::sleep_for(std::chrono::microseconds(10)); 
            workload_ += i;
            if (i % 100 == 0) {
                
                std::lock_guard<std::mutex> lock(cout_mutex_);
                
            }
        }
    }

    
    static int fibonacci(int n) {
        static std::unordered_map<int, int> memo;
        if (n <= 1) return n;
        if (memo.count(n)) return memo[n];
        int res = fibonacci(n - 1) + fibonacci(n - 2);
        memo[n] = res;
        return res;
    }

    
    static void sortVector(std::vector<int>& v) {
        std::sort(v.begin(), v.end());
    }

    int getWorkload() const { return workload_; }

private:
    int id_;
    int workload_;
    static std::mutex cout_mutex_;
};

std::mutex Worker::cout_mutex_;


void producer(std::vector<int>& buffer, std::mutex& mtx, std::condition_variable& cv, std::atomic<bool>& done, int items) {
    for (int i = 0; i < items; ++i) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            buffer.push_back(i);
        }
        cv.notify_one();
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
    done = true;
    cv.notify_all();
}

void consumer(std::vector<int>& buffer, std::mutex& mtx, std::condition_variable& cv, std::atomic<bool>& done, int& sum) {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return !buffer.empty() || done; });
        if (!buffer.empty()) {
            int val = buffer.back();
            buffer.pop_back();
            lock.unlock();
            sum += val;
        } else if (done) {
            break;
        }
    }
}


int deepRecursion(int depth) {
    if (depth <= 0) return 0;
    if (depth == 1) return 1;
    return deepRecursion(depth - 1) + deepRecursion(depth - 2);
}

int main() {
    std::cout << "=== CCTracer Multi-Thread Test ===" << std::endl;

    
    const int NUM_THREADS = 4;
    std::vector<std::thread> threads;
    std::vector<Worker> workers;
    for (int i = 0; i < NUM_THREADS; ++i) {
        workers.emplace_back(i);
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(&Worker::doWork, &workers[i], 2000);
    }
    for (auto& t : threads) t.join();
    threads.clear();

    
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 0);
    std::shuffle(data.begin(), data.end(), std::mt19937{std::random_device{}()});
    Worker::sortVector(data);

    
    int fib_result = Worker::fibonacci(20);
    std::cout << "Fibonacci(20) = " << fib_result << std::endl;

    
    std::vector<int> buffer;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> done{false};
    int sum = 0;
    const int PRODUCER_ITEMS = 500;
    std::thread prod(producer, std::ref(buffer), std::ref(mtx), std::ref(cv), std::ref(done), PRODUCER_ITEMS);
    std::thread cons(consumer, std::ref(buffer), std::ref(mtx), std::ref(cv), std::ref(done), std::ref(sum));
    prod.join();
    cons.join();
    std::cout << "Producer-Consumer sum = " << sum << std::endl;

    
    int rec_result = deepRecursion(10);
    std::cout << "deepRecursion(10) = " << rec_result << std::endl;

    
    auto lambda = [](int x) {
        int y = x * x;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return y;
    };
    auto future = std::async(std::launch::async, lambda, 42);
    int square = future.get();
    std::cout << "Lambda square(42) = " << square << std::endl;

    
    int counter = 0;
    for (int i = 0; i < 1000; ++i) {
        if (i % 3 == 0) {
            counter += 1;
        } else if (i % 3 == 1) {
            counter += 2;
        } else {
            counter += 3;
        }
    }
    std::cout << "Counter = " << counter << std::endl;

    std::cout << "Test completed." << std::endl;
    return 0;
}