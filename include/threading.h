#pragma once

#include <thread>
#include <cstring>
#include <system_error>
#include <pthread.h>
#include <sched.h>

namespace threading {
    struct ThreadConfig {
        char name[16] = {};
        int cpu_core = -1;
        int priority = 0;
        int policy = SCHED_OTHER;
    };

    template<class Function, class... Args>
    std::thread spawn(const ThreadConfig& config, Function&& func, Args&&... args) {
        auto t = std::thread(std::forward<Function>(func), std::forward<Args>(args)...);

        pthread_t handle = t.native_handle();

        if (config.cpu_core >= 0) {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(config.cpu_core, &cpuset);
            int err = pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset);
            if (err) throw std::system_error(err, std::generic_category(), "pthread_setaffinity_np");
        }

        if (config.policy != SCHED_OTHER || config.priority != 0) {
            sched_param param;
            param.sched_priority = config.priority;
            int err = pthread_setschedparam(handle, config.policy, &param);
            if (err) throw std::system_error(err, std::generic_category(), "pthread_setschedparam");
        }

        if (config.name[0] != '\0') {
            int err = pthread_setname_np(handle, config.name);
            if (err) throw std::system_error(err, std::generic_category(), "pthread_setname_np");
        }

        return t;
    }
}
