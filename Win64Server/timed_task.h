#pragma once

#include "thread_pool.h"
#include <chrono>
#include <atomic>
#include <vector>
#include <mutex>
#include <thread>

class TimedTask {
public:
    TimedTask(ThreadPool& threadPool, std::chrono::milliseconds duration, bool compensateComputeTime = false);
    virtual ~TimedTask();

    void start();
    void stop();

    ThreadPool& get_thread_pool();

    static std::vector<TimedTask*>& get_tasks();
    static std::mutex& get_tasks_mutex();

    bool is_running() const;

    std::chrono::milliseconds get_duration() const;
    void enqueue_execute();



protected:
    virtual void execute() = 0;

private:
    ThreadPool& threadPool;
    std::chrono::milliseconds duration;
    bool compensateComputeTime;
    std::atomic<bool> taskRunning;

    static std::vector<TimedTask*> tasks;
    static std::mutex tasksMutex;

    std::chrono::steady_clock::time_point lastExecutionTime;
};
