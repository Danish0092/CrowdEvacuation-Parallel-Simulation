#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <thread>   // <-- Added to ensure std::thread is available to translation units

// ─── Window & Simulation Constants ───────────────────────
constexpr int   WIDTH = 1600;
constexpr int   HEIGHT = 900;
constexpr int   NUM_AGENTS = 800;
constexpr float AGENT_RADIUS = 4.5f;
constexpr float AGENT_SPEED = 2.2f;       // slightly reduced for stability
constexpr int   PANEL_WIDTH = 300;
constexpr int   WALL_THICKNESS = 15;
constexpr int   EXIT_WIDTH = 80;

// Inner-boundary of the simulation room
constexpr float WALL_LEFT = 150.f + WALL_THICKNESS;
constexpr float WALL_RIGHT = WIDTH - PANEL_WIDTH - WALL_THICKNESS;
constexpr float WALL_TOP = 50.f + WALL_THICKNESS;
constexpr float WALL_BOTTOM = HEIGHT - WALL_THICKNESS - 50.f;

// ─── Simulation Modes ───────────────────────────────────
enum class Mode { MENU, SERIAL, OPENMP, MPI_SIM, GPU_SIM };

// ─── Color Palette ───────────────────────────────────────
namespace Palette {
    inline const sf::Color BG = { 18,  18,  28 };
    inline const sf::Color PANEL = { 28,  28,  45 };
    inline const sf::Color ACCENT = { 94,  129, 255 };
    inline const sf::Color GREEN = { 80,  220, 100 };
    inline const sf::Color RED = { 255, 85,  85 };
    inline const sf::Color YELLOW = { 255, 200, 60 };
    inline const sf::Color CYAN = { 60,  220, 220 };
    inline const sf::Color ORANGE = { 255, 140, 0 };
    inline const sf::Color WHITE = { 240, 240, 255 };
    inline const sf::Color DIMWHITE = { 160, 160, 180 };
    inline const sf::Color SERIAL_C = { 255, 120, 120 };
    inline const sf::Color OMP_C = { 100, 200, 255 };
    inline const sf::Color MPI_C = { 180, 130, 255 };
    inline const sf::Color GPU_C = { 80,  255, 160 };
    inline const sf::Color WALL_C = { 80,  80,  120 };
}

// ─── Data Structures ─────────────────────────────────────
struct Exit {
    float x, y;              // exit center
    float x0, y0, x1, y1;   // opening bounds
    std::string label;
};

struct Obstacle { float x, y, w, h; };

struct Agent {
    float     x = 0, y = 0;
    float     vx = 0, vy = 0;
    bool      evacuated = false;
    int       zone = 0;
    sf::Color color;
    float     speed = AGENT_SPEED;   // ADD THIS
    float     panic = 1.0f;          // ADD THIS (1=calm, >1=panicked)
    int       stuckFrames = 0;
    float     checkX = 0, checkY = 0;
};

struct Stats {
    double      frameTime = 0;
    double      totalTime = 0;
    double      fps = 0;
    int         evacuated = 0;
    int         remaining = 0;
    bool        done = false;
    int         threadCount = 1;
    std::string modeName;
    sf::Color   modeColor;
};
