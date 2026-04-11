#pragma once

#include <queue>
#include <mutex>
#include <optional>
#include <coroutine>
#include <functional>
#include <utility>
#include <exception>
#include "thread_pool.h"

struct AgentTask {
    struct promise_type {
        std::function<void(std::exception_ptr)> on_error;

        AgentTask get_return_object() { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { if (on_error) on_error(std::current_exception()); }
    };

    std::coroutine_handle<promise_type> handle;
};

template<typename T>
class MailboxProcessor {
    struct Receiver {
        std::coroutine_handle<> handle;
        T* out;
    };

    std::queue<T> _messages;
    std::optional<Receiver> _receiver;
    std::mutex _mutex;
    ThreadPool* _pool = nullptr;
    bool _active = false;
    std::exception_ptr _error;
    std::function<void(std::exception_ptr)> _on_error;

    void do_enqueue(std::function<void()> task) {
        if (_pool) _pool->enqueue(std::move(task));
        else ThreadPool::_enqueue(std::move(task));
    }

public:
    MailboxProcessor() = default;
    explicit MailboxProcessor(ThreadPool& pool) : _pool(&pool) {}

    MailboxProcessor(const MailboxProcessor&) = delete;
    MailboxProcessor& operator=(const MailboxProcessor&) = delete;
    MailboxProcessor(MailboxProcessor&&) = delete;
    MailboxProcessor& operator=(MailboxProcessor&&) = delete;

    void start(std::function<AgentTask(MailboxProcessor<T>&)> body) {
        _active = true;
        do_enqueue([this, body = std::move(body)]() {
            auto task = body(*this);
            task.handle.promise().on_error = [this](std::exception_ptr e) {
                _active = false;
                _error = e;
                if (_on_error) _on_error(e);
            };
            task.handle.resume();
        });
    }

    void on_error(std::function<void(std::exception_ptr)> handler) { _on_error = std::move(handler); }
    std::exception_ptr error() const { return _error; }
    bool is_active() const { return _active; }
    void stop() { _active = false; }

    struct Awaitable {
        MailboxProcessor& _mb;
        T _result;

        bool await_ready() { return false; }

        bool await_suspend(std::coroutine_handle<> h) {
            std::lock_guard lock(_mb._mutex);
            if (!_mb._messages.empty()) {
                _result = std::move(_mb._messages.front());
                _mb._messages.pop();
                return false;
            }
            _mb._receiver = Receiver{h, &_result};
            return true;
        }

        T await_resume() { return std::move(_result); }
    };

    Awaitable receive() { return Awaitable{*this}; }

    void post(T msg) {
        std::optional<Receiver> receiver;
        {
            std::lock_guard lock(_mutex);
            if (_receiver) {
                *_receiver->out = std::move(msg);
                receiver = std::exchange(_receiver, std::nullopt);
            } else {
                _messages.push(std::move(msg));
            }
        }
        if (receiver) {
            do_enqueue([h = receiver->handle]() { h.resume(); });
        }
    }
};
