#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <vector>
#include <atomic>
#include "threading.h"

class ThreadPool {
    static ThreadPool _pool;
    std::vector<std::thread> _workers;
    std::queue<std::function<void()>> _tasks;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::atomic<bool> _running{true};

    void run_worker() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _cv.wait(lock, [this] { return !_running || !_tasks.empty(); });
                if (!_running && _tasks.empty()) return;
                task = std::move(_tasks.front());
                _tasks.pop();
            }
            task();
        }
    }

public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency()) {
        for (size_t i = 0; i < num_threads; ++i) {
            threading::ThreadConfig config;
            snprintf(config.name, sizeof(config.name), "tp%d", static_cast<int>(i));
            _workers.push_back(threading::spawn(config, [this] { run_worker(); }));
        }
    }

    ~ThreadPool() {
        if (_running) shutdown();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void enqueue(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _tasks.push(std::move(task));
        }
        _cv.notify_one();
    }

    void join() { run_worker(); }

    void shutdown() {
        _running = false;
        _cv.notify_all();
        for (auto& w : _workers)
            if (w.joinable()) w.join();
    }

    static void _enqueue(std::function<void()> task) { _pool.enqueue(std::move(task)); }
    static void _join()                               { _pool.join(); }
    static void _shutdown()                           { _pool.shutdown(); }
};
inline ThreadPool ThreadPool::_pool{};
