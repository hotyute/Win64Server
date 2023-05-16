// controller.cpp
#include "controller.h"
#include <iostream>

Controller::Controller(SOCKET clientSocket, int id)
    : User(clientSocket, id) {
}

void Controller::update() {
    // Perform controller-specific updates
    std::cout << "Controller updated." << std::endl;
}
