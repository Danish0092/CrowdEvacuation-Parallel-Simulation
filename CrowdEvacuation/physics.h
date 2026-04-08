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

inline bool collidesWall(float x, float y,
    const std::vector<Exit>& exits)
{
    if (x < WALL_LEFT) { if (!(y >= exits[0].y0 && y <= exits[0].y1)) return true; }
    if (x > WALL_RIGHT) { if (!(y >= exits[1].y0 && y <= exits[1].y1)) return true; }
    if (y < WALL_TOP) { if (!(x >= exits[2].x0 && x <= exits[2].x1)) return true; }
    if (y > WALL_BOTTOM) { if (!(x >= exits[3].x0 && x <= exits[3].x1)) return true; }
    return false;
}

// FIX: agent is evacuated once clearly outside the room through an exit opening,
// OR if it somehow escaped entirely (catch-all prevents stuck invisible agents).
inline bool isAtExit(const Agent& a, const std::vector<Exit>& exits)
{
    if (a.x < WALL_LEFT - 5.f &&
        a.y >= exits[0].y0 - AGENT_RADIUS && a.y <= exits[0].y1 + AGENT_RADIUS) return true;
    if (a.x > WALL_RIGHT + 5.f &&
        a.y >= exits[1].y0 - AGENT_RADIUS && a.y <= exits[1].y1 + AGENT_RADIUS) return true;
    if (a.y < WALL_TOP - 5.f &&
        a.x >= exits[2].x0 - AGENT_RADIUS && a.x <= exits[2].x1 + AGENT_RADIUS) return true;
    if (a.y > WALL_BOTTOM + 5.f &&
        a.x >= exits[3].x0 - AGENT_RADIUS && a.x <= exits[3].x1 + AGENT_RADIUS) return true;
    // Catch-all: escaped entirely outside the room
    if (a.x < WALL_LEFT - EXIT_WIDTH || a.x > WALL_RIGHT + EXIT_WIDTH ||
        a.y < WALL_TOP - EXIT_WIDTH || a.y > WALL_BOTTOM + EXIT_WIDTH)
        return true;
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

inline void resolveMove(Agent& a,
    float newVx, float newVy,
    const std::vector<Obstacle>& obstacles,
    const std::vector<Exit>& exits)
{
    constexpr float MAX_SPEED = AGENT_SPEED * 2.5f;
    float mag = std::sqrt(newVx * newVx + newVy * newVy);
    if (mag > MAX_SPEED) { newVx = newVx / mag * MAX_SPEED; newVy = newVy / mag * MAX_SPEED; }

    a.vx = newVx; a.vy = newVy;
    float nx = a.x + a.vx, ny = a.y + a.vy;

    if (!collidesObstacle(nx, ny, obstacles) && !collidesWall(nx, ny, exits)) { a.x = nx; a.y = ny; }
    else if (!collidesObstacle(nx, a.y, obstacles) && !collidesWall(nx, a.y, exits)) { a.x = nx; }
    else if (!collidesObstacle(a.x, ny, obstacles) && !collidesWall(a.x, ny, exits)) { a.y = ny; }

    // Stuck detection: if barely moved for 300 frames, force-evacuate
    a.stuckFrames++;
    if (a.stuckFrames % 60 == 0) {
        float dx = a.x - a.checkX, dy = a.y - a.checkY;
        if (dx * dx + dy * dy < 4.0f) {   // less than 2px net movement in 60 frames
            a.evacuated = true; return;
        }
        a.checkX = a.x; a.checkY = a.y;
    }
}

// ─── Agent Initialisation ────────────────────────────────
inline std::vector<Agent> initAgents(const std::vector<Obstacle>& obstacles,
    const std::vector<Exit>& exits)
{
    std::vector<sf::Color> zoneColors = {
        Palette::SERIAL_C, Palette::OMP_C, Palette::MPI_C, Palette::GPU_C
    };
    int roomW = (int)(WALL_RIGHT - WALL_LEFT - 20);
    int roomH = (int)(WALL_BOTTOM - WALL_TOP - 20);
    std::vector<Agent> agents(NUM_AGENTS);
    for (int i = 0; i < NUM_AGENTS; i++) {
        float x, y; int tries = 0;
        do {
            x = WALL_LEFT + 10 + rand() % roomW;
            y = WALL_TOP + 10 + rand() % roomH;
            tries++;
        } while ((collidesObstacle(x, y, obstacles) || collidesWall(x, y, exits)) && tries < 200);
        agents[i].x = x; agents[i].y = y;
        agents[i].vx = 0; agents[i].vy = 0;
        agents[i].evacuated = false;
        int zx = (x < (WALL_LEFT + WALL_RIGHT) / 2) ? 0 : 1;
        int zy = (y < (WALL_TOP + WALL_BOTTOM) / 2) ? 0 : 1;
        agents[i].zone = zy * 2 + zx;
        agents[i].color = zoneColors[agents[i].zone];
    }
    return agents;
}