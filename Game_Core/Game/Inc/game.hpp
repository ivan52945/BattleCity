#pragma once

#include "disp_iface.hpp"
#include "lobby.hpp"
#include "point.hpp"
#include "tank.hpp"
#include "shell.hpp"
#include "config.hpp"

class Game {
public:
    int unit_n_cur[2] = {0, 0};
    /* Simultaneous tanks quota per side (gameplay config). */
    int unit_n_max[2] = {cfg::units_per_side, cfg::units_per_side};
    int fld_x_end;
    int fld_y_end;

    /* Spawn slots: computed in init() from the real field geometry
     * (columns near the edges, slots spread evenly over the height). */
    point_t spawn_coords[2][cfg::spawn_slots] = {};

    Lobby<Tank, 20> tanks;
    Lobby<Shell, 96> shells;

    Game(Disp_Iface* d);

    void init();

    void loop();
private:
    void spawn();

    /* Display driver injected from the platform layer. Every Game
     * instance carries its own - the per-entity pointers of its
     * pools are stamped from it in init(). */
    Disp_Iface* disp = nullptr;

    /* Frame counter of this Game instance; per-instance pacing. */
    int game_i = 0;

    /* True if a live tank sits too close to the spawn point. */
    static bool is_booked(const point_t &pt, const Tank &tank);

    static void resolve_collision(Tank &t_moved, const Tank &t_stayed);

    void resolve_collision(Shell &shell, Tank &tank);
};