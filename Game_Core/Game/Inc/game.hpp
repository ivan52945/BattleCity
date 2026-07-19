#pragma once

#include "lobby.hpp"
#include "point.hpp"
#include "tank.hpp"
#include "shell.hpp"

class Game {
public:
    int unit_n_cur[2] = {0, 0};
    int unit_n_max[2] = {4, 4};
    int fld_x_end;
    int fld_y_end;

    point_t spawn_coords[2] = {};

    Lobby<Tank, 20> tanks;
    Lobby<Shell, 96> shells;

    Game();

    void loop();
private:
    void spawn();


    static bool is_booked(const int &spawn_x, const int &spawn_y, const Tank &tank);

    static void resolve_collision(Tank &t_moved, const Tank &t_stayed);

    void resolve_collision(Shell &shell, Tank &tank);
};