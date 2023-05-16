#pragma once

#include "thread_pool.h"
#include "aircraft.h"
#include "controller.h"
#include <vector>
#include <memory>
#include <mutex>

class Simulation {
public:
    Simulation(ThreadPool& threadPool);
    void addEntity(std::unique_ptr<User> entity);

    void updateEntities();

private:
    ThreadPool& threadPool;
    std::vector<std::unique_ptr<User>> entities;
    std::mutex entitiesMutex;
};
