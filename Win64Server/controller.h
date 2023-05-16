// controller.h
#pragma once
#include "user.h"

class Controller : public User {
public:
    Controller(SOCKET clientSocket, int id);

    void update() override;

    int controller_rating = 0, controller_position = 0;
};
