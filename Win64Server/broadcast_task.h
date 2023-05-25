#pragma once

#include <winsock2.h>
#include <unordered_map>
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
extern std::shared_mutex lastPacketReceivedMutex;
extern std::vector<SOCKET> clientSockets;
extern std::unordered_map<SOCKET, std::chrono::steady_clock::time_point> lastPacketReceived;
