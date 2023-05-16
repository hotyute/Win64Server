#include "simulation.h"

Simulation::Simulation(ThreadPool& threadPool) : threadPool(threadPool) {}

void Simulation::addEntity(std::unique_ptr<User> entity) {
    std::lock_guard<std::mutex> lock(entitiesMutex);
    entities.push_back(std::move(entity));
}

void Simulation::updateEntities() {
    std::lock_guard<std::mutex> lock(entitiesMutex);
    for (auto& entity : entities) {
        threadPool.enqueue([&]() { entity->update(); });
    }
}
