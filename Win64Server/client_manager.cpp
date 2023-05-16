#include "client_manager.h"
#include "outgoing_packets.h"

ClientManager clientManager(MAX_CLIENTS);

void adjust_time(const std::shared_ptr<User>& user) {
    long long lastTime = UPDATE_TIMES::DEFAULT;

    user->iterateLocalUsers([&](const auto& other) {
	    const long long otherReqInterval = other.first->getRequestedInterval(user->getType());
        if (lastTime == UPDATE_TIMES::DEFAULT || otherReqInterval < lastTime) {
            lastTime = otherReqInterval;
        }
        });

    if (lastTime != user->getUpdateInterval()) {
        user->setUpdateInterval(lastTime);
        send_time_change(*user, lastTime);
    }
}