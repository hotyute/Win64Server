#ifndef CLIENT_UPDATER
#define CLIENT_UPDATER

#include "boost/date_time/posix_time/posix_time.hpp"
#include <thread>
#include <stdio.h>
#include "client_manager.h"
#include "constants.h"
#include "user.h"

#include "timed_task.h"
#include "thread_pool.h"

class UpdateTask : public TimedTask {
public:
    UpdateTask(ThreadPool& threadPool, std::chrono::milliseconds duration);
    virtual ~UpdateTask();

protected:
    virtual void execute() override;
};

void globalUpdate();

void updateLocalUsers(const std::shared_ptr<User>&);

void update(const std::shared_ptr<User>&);

void send_scripts(const std::shared_ptr<User>& user, const std::shared_ptr<User>& scripts_user);

void send_remove_scripts(const std::shared_ptr<User>& user, const std::shared_ptr<User>& scripts_user);

void send_pos_update(const std::shared_ptr<User>& updated_user, const std::shared_ptr<User>& about_user);

void updateOnlyQueues(User& user);

double dist(double lat1, double lon1, double lat2, double lon2);

bool is_digits(const std::string& str);

bool contains_user(const std::shared_ptr<User>& user, const std::shared_ptr<User>& user2);

std::shared_ptr<long long> timeUpdate(const std::shared_ptr<User>&, const std::shared_ptr<User>&);

void check_timeout(const std::shared_ptr<User>& user);

#endif
