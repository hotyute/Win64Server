#include "broadcast_task.h"
#include "packet_handler.h"
#include "client_manager.h"

#include "outgoing_packets.h"


BroadcastTask::BroadcastTask(ThreadPool& threadPool, std::chrono::milliseconds duration)
	: TimedTask(threadPool, duration) {}

BroadcastTask::~BroadcastTask() = default;

void BroadcastTask::execute() {
	//std::cout << "Ping to all clients..." << std::endl;

	auto& threadPool = get_thread_pool();
	clientManager.iterate_users([&](const std::shared_ptr<User>& user) {
		threadPool.enqueue([&user]() { send_ping(*user); });
		});
}
