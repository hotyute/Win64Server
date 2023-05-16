#pragma once

#include <winsock2.h>
#include <shared_mutex>

#include "timed_task.h"
#include "thread_pool.h"

class BroadcastTask : public TimedTask {
public:
    BroadcastTask(ThreadPool& threadPool, std::chrono::milliseconds duration);
    virtual ~BroadcastTask();

protected:
    virtual void execute() override;
};

extern std::shared_mutex clientSocketsMutex;
extern std::vector<SOCKET> clientSockets;
