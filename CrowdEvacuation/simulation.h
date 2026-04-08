#pragma once
#include "constants.h"
#include "physics.h"
#include "world.h"
#include <omp.h>
#include <thread>
#include <vector>
#include <cmath>

// ─── Common force computation for one agent ──────────────
static inline void computeForces(int i,
    const std::vector<Agent>& ag,
    const std::vector<Exit>& exits,
    float& outVx, float& outVy)
{
    const Agent& a = ag[i];

    Exit e = nearestExit(a, exits);
    float dx = e.x - a.x, dy = e.y - a.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    outVx = (dist > 0.1f) ? (dx / dist) * AGENT_SPEED : 0.f;
    outVy = (dist > 0.1f) ? (dy / dist) * AGENT_SPEED : 0.f;

    for (int j = 0; j < (int)ag.size(); j++) {
        if (i == j || ag[j].evacuated) continue;
        float ex = a.x - ag[j].x, ey = a.y - ag[j].y;
        float ed = std::sqrt(ex * ex + ey * ey);
        if (ed < AGENT_RADIUS * 3.0f && ed > 0.01f) {
            float push = 0.45f * (1.f - ed / (AGENT_RADIUS * 3.0f));
            outVx += (ex / ed) * push;
            outVy += (ey / ed) * push;
        }
    }
}

// ─── SERIAL ──────────────────────────────────────────────
inline void updateSerial(std::vector<Agent>& ag,
    const std::vector<Obstacle>& obstacles,
    const std::vector<Exit>& exits)
{
    for (int i = 0; i < (int)ag.size(); i++) {
        if (ag[i].evacuated) continue;
        if (isAtExit(ag[i], exits)) { ag[i].evacuated = true; continue; }
        float vx, vy;
        computeForces(i, ag, exits, vx, vy);
        resolveMove(ag[i], vx, vy, obstacles, exits);
    }
}

// ─── OPENMP ──────────────────────────────────────────────
inline void updateOpenMP(std::vector<Agent>& ag,
    const std::vector<Obstacle>& obstacles,
    const std::vector<Exit>& exits)
{
#pragma omp parallel for schedule(dynamic, 16) num_threads(omp_get_max_threads())
    for (int i = 0; i < (int)ag.size(); i++) {
        if (ag[i].evacuated) continue;
        if (isAtExit(ag[i], exits)) { ag[i].evacuated = true; continue; }
        float vx, vy;
        computeForces(i, ag, exits, vx, vy);
        resolveMove(ag[i], vx, vy, obstacles, exits);
    }
}

// ─── MPI-STYLE ───────────────────────────────────────────
inline void updateMPI(std::vector<Agent>& ag,
    const std::vector<Obstacle>& obstacles,
    const std::vector<Exit>& exits)
{
    float midX = (WALL_LEFT + WALL_RIGHT) / 2.f;
    float midY = (WALL_TOP + WALL_BOTTOM) / 2.f;

    std::vector<std::vector<int>> zones(4);
    for (int i = 0; i < (int)ag.size(); i++) {
        if (ag[i].evacuated) continue;
        int zx = (ag[i].x < midX) ? 0 : 1;
        int zy = (ag[i].y < midY) ? 0 : 1;
        zones[zy * 2 + zx].push_back(i);
    }

    auto processZone = [&](int zid) {
        for (int i : zones[zid]) {
            if (ag[i].evacuated) continue;
            if (isAtExit(ag[i], exits)) { ag[i].evacuated = true; continue; }
            float vx, vy;
            computeForces(i, ag, exits, vx, vy);
            resolveMove(ag[i], vx, vy, obstacles, exits);
        }
        };

    std::vector<std::thread> threads;
    threads.reserve(4);
    for (int z = 0; z < 4; z++)
        threads.emplace_back(processZone, z);
    for (auto& t : threads) t.join();
}

// ─── GPU-STYLE ───────────────────────────────────────────
inline void updateGPU(std::vector<Agent>& ag,
    const std::vector<Obstacle>& obstacles,
    const std::vector<Exit>& exits)
{
    int n = (int)ag.size();
    std::vector<float> fx(n, 0.f), fy(n, 0.f);
    std::vector<bool>  evac(n, false);

#pragma omp parallel for schedule(static) num_threads(16)
    for (int i = 0; i < n; i++) {
        if (ag[i].evacuated) continue;
        if (isAtExit(ag[i], exits)) { evac[i] = true; continue; }
        computeForces(i, ag, exits, fx[i], fy[i]);
    }

#pragma omp parallel for schedule(static) num_threads(16)
    for (int i = 0; i < n; i++) {
        if (ag[i].evacuated) continue;
        if (evac[i]) { ag[i].evacuated = true; continue; }
        resolveMove(ag[i], fx[i], fy[i], obstacles, exits);
    }
}