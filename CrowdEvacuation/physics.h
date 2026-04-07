#pragma once
#include "constants.h"
#include "world.h"
#include <vector>
#include <cmath>
#include <cstdlib>

// ─── Collision Checks ────────────────────────────────────

inline bool collidesObstacle(float x, float y,
    const std::vector<Obstacle>& obstacles)
{
    for (const auto& o : obstacles)
        if (x >= o.x && x <= o.x + o.w &&
            y >= o.y && y <= o.y + o.h)
            return true;
    return false;
}

// Returns true when (x,y) is inside a solid wall (not the exit opening).
/// FIX: exits are cut out properly so agents can leave through them.
inline bool collidesWall(float x, float y,
    const std::vector<Exit>& exits)
{
    // West wall
    if (x < WALL_LEFT) {
        if (!(y >= exits[0].y0 && y <= exits[0].y1)) return true;
    }
    // East wall
    if (x > WALL_RIGHT) {
        if (!(y >= exits[1].y0 && y <= exits[1].y1)) return true;
    }
    // North wall
    if (y < WALL_TOP) {
        if (!(x >= exits[2].x0 && x <= exits[2].x1)) return true;
    }
    // South wall
    if (y > WALL_BOTTOM) {
        if (!(x >= exits[3].x0 && x <= exits[3].x1)) return true;
    }
    return false;
}

// FIX: agent is evacuated once it has clearly passed through an exit
// (a small margin beyond the wall edge) — avoids the "stuck-on-wall" bug
// where agents reach the opening but never trip the old in-wall check.
inline bool isAtExit(const Agent& a, const std::vector<Exit>& exits)
{
    // West — agent must have moved left past the inner edge
    if (a.x < WALL_LEFT - 12 &&
        a.y >= exits[0].y0 && a.y <= exits[0].y1) return true;
    // East
    if (a.x > WALL_RIGHT + 12 &&
        a.y >= exits[1].y0 && a.y <= exits[1].y1) return true;
    // North
    if (a.y < WALL_TOP - 12 &&
        a.x >= exits[2].x0 && a.x <= exits[2].x1) return true;
    // South
    if (a.y > WALL_BOTTOM + 12 &&
        a.x >= exits[3].x0 && a.x <= exits[3].x1) return true;
    return false;
}

inline Exit nearestExit(const Agent& a, const std::vector<Exit>& exits)
{
    const Exit* best = &exits[0];
    float bestD = 1e9f;
    for (const auto& e : exits) {
        float dx = e.x - a.x, dy = e.y - a.y;
        float d = dx * dx + dy * dy;
        if (d < bestD) { bestD = d; best = &e; }
    }
    return *best;
}

// ─── Shared move-and-resolve helper (used by all update modes) ────────
// FIX: speed is clamped so crowded agents don't oscillate/freeze.
inline void resolveMove(Agent& a,
    float newVx, float newVy,
    const std::vector<Obstacle>& obstacles,
    const std::vector<Exit>& exits)
{
    // Clamp velocity magnitude
    constexpr float MAX_SPEED = AGENT_SPEED * 2.5f;
    float mag = std::sqrt(newVx * newVx + newVy * newVy);
    if (mag > MAX_SPEED) { newVx = newVx / mag * MAX_SPEED; newVy = newVy / mag * MAX_SPEED; }

    a.vx = newVx;
    a.vy = newVy;

    float nx = a.x + a.vx;
    float ny = a.y + a.vy;

    if (!collidesObstacle(nx, ny, obstacles) && !collidesWall(nx, ny, exits)) {
        a.x = nx; a.y = ny;
    }
    else if (!collidesObstacle(nx, a.y, obstacles) && !collidesWall(nx, a.y, exits)) {
        a.x = nx;
    }
    else if (!collidesObstacle(a.x, ny, obstacles) && !collidesWall(a.x, ny, exits)) {
        a.y = ny;
    }
    // else: fully blocked this frame — agent stays put (won't freeze on wall)
}

// ─── Agent Initialisation ────────────────────────────────
inline std::vector<Agent> initAgents(const std::vector<Obstacle>& obstacles,
    const std::vector<Exit>& exits)
{
    std::vector<sf::Color> zoneColors = {
        Palette::SERIAL_C, Palette::OMP_C,
        Palette::MPI_C,    Palette::GPU_C
    };

    int roomW = (int)(WALL_RIGHT - WALL_LEFT - 20);
    int roomH = (int)(WALL_BOTTOM - WALL_TOP - 20);

    std::vector<Agent> agents(NUM_AGENTS);
    for (int i = 0; i < NUM_AGENTS; i++) {
        float x, y;
        int tries = 0;
        do {
            x = WALL_LEFT + 10 + rand() % roomW;
            y = WALL_TOP + 10 + rand() % roomH;
            tries++;
        } while ((collidesObstacle(x, y, obstacles) ||
            collidesWall(x, y, exits)) && tries < 200);

        agents[i].x = x;  agents[i].y = y;
        agents[i].vx = 0; agents[i].vy = 0;
        agents[i].evacuated = false;

        int zx = (x < (WALL_LEFT + WALL_RIGHT) / 2) ? 0 : 1;
        int zy = (y < (WALL_TOP + WALL_BOTTOM) / 2) ? 0 : 1;
        agents[i].zone = zy * 2 + zx;
        agents[i].color = zoneColors[agents[i].zone];
    }
    return agents;
}
