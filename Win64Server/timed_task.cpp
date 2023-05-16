#include "timed_task.h"

std::vector<TimedTask*> TimedTask::tasks;
std::mutex TimedTask::tasksMutex;

TimedTask::TimedTask(ThreadPool& threadPool, std::chrono::milliseconds duration, bool compensateComputeTime)
    : threadPool(threadPool), duration(duration), compensateComputeTime(compensateComputeTime), taskRunning(false) {
}

TimedTask::~TimedTask() {
    stop();
}

void TimedTask::start() {
    taskRunning = true;
    {
        std::unique_lock<std::mutex> lock(tasksMutex);
        tasks.push_back(this);
    }
}

void TimedTask::stop() {
    taskRunning = false;
    {
        std::unique_lock<std::mutex> lock(tasksMutex);
        tasks.erase(std::remove(tasks.begin(), tasks.end(), this), tasks.end());
    }
}

ThreadPool& TimedTask::get_thread_pool() {
    return threadPool;
}

std::vector<TimedTask*>& TimedTask::get_tasks() {
    return tasks;
}

std::mutex& TimedTask::get_tasks_mutex() {
    return tasksMutex;
}

bool TimedTask::is_running() const {
    return taskRunning;
}

std::chrono::milliseconds TimedTask::get_duration() const {
    return duration;
}

void TimedTask::enqueue_execute() {
    threadPool.enqueue([this]() { execute(); });
}

