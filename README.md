# mailboxprocessor-cpp

A C++20 coroutine-based implementation of F#'s [MailboxProcessor](https://fsharp.github.io/fsharp-core-docs/reference/fsharp-control-fsharpasync.html), providing a single-threaded agent model for concurrent message processing.

## What is a MailboxProcessor?

In F#, `MailboxProcessor` is a lightweight agent that processes messages sequentially from an internal queue. External code can post messages concurrently, but the agent handles them one at a time — no locks needed inside the handler. This library brings the same pattern to C++20 using coroutines.

```cpp
MailboxProcessor<std::string> agent;

agent.start([](MailboxProcessor<std::string>& inbox) -> AgentTask {
    while (inbox.is_active()) {
        auto msg = co_await inbox.receive();
        std::cout << "Received: " << msg << std::endl;
    }
});

agent.post("hello");
agent.post("world");
```

## Key properties

- **Single-threaded processing** — messages are handled one at a time, in order. No need to synchronise state inside the handler.
- **Thread-safe posting** — `post()` can be called from any thread.
- **Coroutine-based** — the agent suspends when no messages are available and resumes when one arrives. No busy-waiting.
- **Error handling** — register an error callback with `on_error()`, similar to F#'s `Error` event.

## Requirements

- C++20 compiler with coroutine support (GCC 11+, Clang 14+, MSVC 19.28+)
- Linux (uses pthreads for thread naming and CPU affinity)
- CMake 3.20+

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

This builds the example in `examples/`.

## Usage

The library is header-only. Copy the `include/` directory into your project or add it to your include path.

With CMake:

```cmake
add_subdirectory(mailboxprocessor-cpp)
target_link_libraries(your_target PRIVATE mailbox_processor)
```

## API

### MailboxProcessor<T>

| Method | Description |
|--------|-------------|
| `start(body)` | Start the agent with a coroutine body |
| `post(msg)` | Send a message to the agent (thread-safe) |
| `receive()` | Await the next message (inside the agent coroutine) |
| `stop()` | Deactivate the agent |
| `is_active()` | Check if the agent is running |
| `on_error(handler)` | Register an exception handler |
| `error()` | Get the last stored exception |

### ThreadPool

The agent runs on a thread pool. By default it uses a global static pool. You can provide your own:

```cpp
ThreadPool pool(4); // 4 worker threads
MailboxProcessor<int> agent(pool);
```

### Thread configuration

Worker threads can be configured with CPU affinity, scheduling priority, and naming via `threading::ThreadConfig`.

## License

MIT
