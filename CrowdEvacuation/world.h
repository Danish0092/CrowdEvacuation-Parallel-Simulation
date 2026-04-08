#pragma once
#include "constants.h"
#include <vector>

// ─── Exits (4 openings in the boundary walls) ────────────
inline std::vector<Exit> makeExits() {
    return {
        // West exit (left wall)
        { WALL_LEFT,  HEIGHT / 2.f,
          WALL_LEFT - 20.f,  HEIGHT / 2.f - EXIT_WIDTH / 2.f,
          WALL_LEFT + 10.f,  HEIGHT / 2.f + EXIT_WIDTH / 2.f, "W" },

        // East exit (right wall)
        { WALL_RIGHT, HEIGHT / 2.f,
          WALL_RIGHT - 10.f,  HEIGHT / 2.f - EXIT_WIDTH / 2.f,
          WALL_RIGHT + 20.f,  HEIGHT / 2.f + EXIT_WIDTH / 2.f, "E" },

        // North exit (top wall)
        { WIDTH / 2.f - 100.f, WALL_TOP,
          WIDTH / 2.f - 100.f - EXIT_WIDTH / 2.f, WALL_TOP - 10.f,
          WIDTH / 2.f - 100.f + EXIT_WIDTH / 2.f, WALL_TOP + 10.f, "N" },

        // South exit (bottom wall)
        { WIDTH / 2.f - 100.f, WALL_BOTTOM,
          WIDTH / 2.f - 100.f - EXIT_WIDTH / 2.f, WALL_BOTTOM - 10.f,
          WIDTH / 2.f - 100.f + EXIT_WIDTH / 2.f, WALL_BOTTOM + 10.f, "S" }
    };
}

// ─── Obstacles ───────────────────────────────────────────
inline std::vector<Obstacle> makeObstacles() {
    return {
        {400.f, 250.f, 100.f, 120.f},
        {750.f, 350.f, 100.f, 150.f},
        {520.f, 500.f, 140.f,  80.f},
        {300.f, 550.f,  80.f, 100.f},
        {200.f, 300.f,  70.f,  90.f},
        {600.f, 150.f,  90.f, 100.f},
        {850.f, 550.f, 110.f, 120.f},
        {350.f, 700.f, 150.f,  80.f}
    };
}
