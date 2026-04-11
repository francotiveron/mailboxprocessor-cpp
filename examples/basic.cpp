#include <iostream>
#include <string>
#include "mailbox_processor.h"

enum class Command { Greet, Count, Quit };

int main() {
    MailboxProcessor<Command> agent;

    agent.on_error([](std::exception_ptr e) {
        try { std::rethrow_exception(e); }
        catch (const std::exception& ex) {
            std::cerr << "Agent error: " << ex.what() << std::endl;
        }
    });

    int count = 0;

    agent.start([&count](MailboxProcessor<Command>& inbox) -> AgentTask {
        while (inbox.is_active()) {
            auto msg = co_await inbox.receive();
            switch (msg) {
                case Command::Greet:
                    std::cout << "Hello!" << std::endl;
                    break;
                case Command::Count:
                    std::cout << "Count: " << ++count << std::endl;
                    break;
                case Command::Quit:
                    std::cout << "Goodbye!" << std::endl;
                    inbox.stop();
                    co_return;
            }
        }
    });

    agent.post(Command::Greet);
    agent.post(Command::Count);
    agent.post(Command::Count);
    agent.post(Command::Count);
    agent.post(Command::Quit);

    ThreadPool::_join();
}
