#pragma once
#include "constants.h"
#include "world.h"
#include <vector>
#include <cmath>
#include <cstdlib>

constexpr int   STUCK_FRAMES = 90;   // ~1.5 s at 60 fps before reassigning exit
constexpr float STUCK_DIST_SQ = 1.5f * 1.5f; // moved less than this → considered stuck

inline bool collidesObstacle(float x, float y, const std::vector<Obstacle>& obstacles)
{
    for (const auto& o : obstacles)
        if (x >= o.x && x <= o.x + o.w && y >= o.y && y <= o.y + o.h)
            return true;
    return false;
}

inline bool collidesWall(float x, float y, const std::vector<Exit>& exits)
{
    constexpr float M = AGENT_RADIUS * 2.f;
    if (x < WALL_LEFT) { if (!(y >= exits[0].y0 - M && y <= exits[0].y1 + M)) return true; }
    if (x > WALL_RIGHT) { if (!(y >= exits[1].y0 - M && y <= exits[1].y1 + M)) return true; }
    if (y < WALL_TOP) { if (!(x >= exits[2].x0 - M && x <= exits[2].x1 + M)) return true; }
    if (y > WALL_BOTTOM) { if (!(x >= exits[3].x0 - M && x <= exits[3].x1 + M)) return true; }
    return false;
}

inline bool isAtExit(const Agent& a, const std::vector<Exit>& exits)
{
    constexpr float M = AGENT_RADIUS * 2.f;
    if (a.x < WALL_LEFT - 1.f && a.y >= exits[0].y0 - M && a.y <= exits[0].y1 + M) return true;
    if (a.x > WALL_RIGHT + 1.f && a.y >= exits[1].y0 - M && a.y <= exits[1].y1 + M) return true;
    if (a.y < WALL_TOP - 1.f && a.x >= exits[2].x0 - M && a.x <= exits[2].x1 + M) return true;
    if (a.y > WALL_BOTTOM + 1.f && a.x >= exits[3].x0 - M && a.x <= exits[3].x1 + M) return true;
    if (a.x < WALL_LEFT - EXIT_WIDTH || a.x > WALL_RIGHT + EXIT_WIDTH ||
        a.y < WALL_TOP - EXIT_WIDTH || a.y > WALL_BOTTOM + EXIT_WIDTH) return true;
    return false;
}

inline int nearestExitIndex(float x, float y, const std::vector<Exit>& exits)
{
    int best = 0; float bestD = 1e9f;
    for (int i = 0; i < (int)exits.size(); i++) {
        float dx = exits[i].x - x, dy = exits[i].y - y;
        float d = dx * dx + dy * dy;
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

inline Exit nearestExit(const Agent& a, const std::vector<Exit>& exits)
{
    return exits[nearestExitIndex(a.x, a.y, exits)];
}

// ── Stuck detection: call once per frame per agent ───────
// Returns true if the exit was reassigned.
inline bool updateStuck(Agent& a, const std::vector<Exit>& exits)
{
    float dx = a.x - a.lastX, dy = a.y - a.lastY;
    if (dx * dx + dy * dy < STUCK_DIST_SQ) {
        a.stuckTimer++;
        if (a.stuckTimer >= STUCK_FRAMES) {
            // Try each exit in order of distance, skip current one
            int numExits = (int)exits.size();
            // Build sorted indices by distance
            float dists[4]; int idx[4];
            for (int i = 0; i < numExits; i++) {
                float ex = exits[i].x - a.x, ey = exits[i].y - a.y;
                dists[i] = ex * ex + ey * ey;
                idx[i] = i;
            }
            // Simple insertion sort (only 4 elements)
            for (int i = 1; i < numExits; i++)
                for (int j = i; j > 0 && dists[idx[j]] < dists[idx[j - 1]]; j--)
                    std::swap(idx[j], idx[j - 1]);

            // Pick nearest that isn't current
            for (int k = 0; k < numExits; k++) {
                if (idx[k] != a.exitIndex) {
                    a.exitIndex = idx[k];
                    break;
                }
            }
            a.stuckTimer = 0;
            a.lastX = a.x; a.lastY = a.y;
            return true;
        }
    }
    else {
        a.stuckTimer = 0;
        a.lastX = a.x; a.lastY = a.y;
    }
    return false;
}

inline void resolveMove(Agent& a, float newVx, float newVy,
    const std::vector<Obstacle>& obstacles, const std::vector<Exit>& exits)
{
    constexpr float MAX_SPEED = AGENT_SPEED * 2.5f;
    float mag = std::sqrt(newVx * newVx + newVy * newVy);
    if (mag > MAX_SPEED) { newVx = newVx / mag * MAX_SPEED; newVy = newVy / mag * MAX_SPEED; }

    a.vx = newVx; a.vy = newVy;
    float nx = a.x + a.vx, ny = a.y + a.vy;

    if (!collidesObstacle(nx, ny, obstacles) && !collidesWall(nx, ny, exits)) { a.x = nx; a.y = ny; }
    else if (!collidesObstacle(nx, a.y, obstacles) && !collidesWall(nx, a.y, exits)) { a.x = nx; }
    else if (!collidesObstacle(a.x, ny, obstacles) && !collidesWall(a.x, ny, exits)) { a.y = ny; }
}

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
        agents[i].lastX = x; agents[i].lastY = y;
        agents[i].vx = 0; agents[i].vy = 0;
        agents[i].evacuated = false;
        agents[i].stuckTimer = 0;
        int zx = (x < (WALL_LEFT + WALL_RIGHT) / 2) ? 0 : 1;
        int zy = (y < (WALL_TOP + WALL_BOTTOM) / 2) ? 0 : 1;
        agents[i].zone = zy * 2 + zx;
        agents[i].color = zoneColors[agents[i].zone];
        agents[i].exitIndex = nearestExitIndex(x, y, exits);
    }
    return agents;
}