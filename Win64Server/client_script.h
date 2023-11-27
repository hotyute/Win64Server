#pragma once

#include <any>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <queue>
#include <mutex>

struct ClientScript {
    int idx = -1;
    ClientScript(std::string assem) : assembly(assem) {};
    std::string assembly;
    std::vector<std::any> objects;
};

class ScriptIndex {
public:
    static const int SIZE = 512;
    std::shared_ptr<ClientScript> scripts[SIZE];
    std::queue<int> available_indices_;
    std::mutex mtx;

    ScriptIndex() {
        // Initialize the queue with all available indices
        for (int i = 0; i < SIZE; ++i) {
            available_indices_.push(i);
        }
    }

    void addScript(std::shared_ptr<ClientScript> script) {
        std::unique_lock<std::mutex> lock(mtx);
        if (!available_indices_.empty()) {
            int idx = available_indices_.front(); // Get the next available index
            available_indices_.pop(); // Remove the index from the queue
            scripts[idx] = script;
            script->idx = idx; // Set the index in the Script object
        }
        else {
            throw std::runtime_error("No free space available");
        }
    }

    std::shared_ptr<ClientScript> getScript(int idx) {
        std::unique_lock<std::mutex> lock(mtx);
        if (idx >= 0 && idx < SIZE) {
            return scripts[idx];
        }
        else {
            throw std::out_of_range("Index out of range");
        }
    }

    void deleteScript(int idx) {
        std::unique_lock<std::mutex> lock(mtx);
        if (idx >= 0 && idx < SIZE) {
            scripts[idx].reset();
            available_indices_.push(idx); // Add the index back to the queue
        }
        else {
            throw std::out_of_range("Index out of range");
        }
    }
};
