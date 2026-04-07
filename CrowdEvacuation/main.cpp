#include "constants.h"
#include "world.h"
#include "physics.h"
#include "simulation.h"
#include "renderer.h"
#include <SFML/Graphics.hpp>
#include <omp.h>
#include <cstdlib>
#include <ctime>

// ─── Run one simulation until ESC or all evacuated ───────
static Mode runSimulation(sf::RenderWindow& win, sf::Font& font, Mode mode)
{
    srand((unsigned)time(nullptr));
    auto exits = makeExits();
    auto obstacles = makeObstacles();
    auto agents = initAgents(obstacles, exits);

    // Reset visualization state
    initTrails((int)agents.size());
    resetGraph();

    Stats st;
    switch (mode) {
    case Mode::SERIAL:
        st.modeName = "SERIAL MODE";
        st.modeColor = Palette::SERIAL_C;
        st.threadCount = 1;
        break;
    case Mode::OPENMP:
        st.modeName = "OpenMP MODE";
        st.modeColor = Palette::OMP_C;
        st.threadCount = omp_get_max_threads();
        break;
    case Mode::MPI_SIM:
        st.modeName = "MPI-STYLE MODE";
        st.modeColor = Palette::MPI_C;
        st.threadCount = 4;
        break;
    case Mode::GPU_SIM:
        st.modeName = "GPU-STYLE MODE";
        st.modeColor = Palette::GPU_C;
        st.threadCount = 16;
        break;
    default: break;
    }

    bool showHeatmap = true;

    double startTime = omp_get_wtime();
    sf::Clock fpsClock;
    int frameCount = 0;

    while (win.isOpen()) {
        sf::Event ev;
        while (win.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed)
            {
                win.close(); return Mode::MENU;
            }
            if (ev.type == sf::Event::KeyPressed) {
                if (ev.key.code == sf::Keyboard::Escape) return Mode::MENU;
                if (ev.key.code == sf::Keyboard::H)     showHeatmap = !showHeatmap;
            }
        }

        double t0 = omp_get_wtime();
        if (!st.done) {
            switch (mode) {
            case Mode::SERIAL:  updateSerial(agents, obstacles, exits); break;
            case Mode::OPENMP:  updateOpenMP(agents, obstacles, exits); break;
            case Mode::MPI_SIM: updateMPI(agents, obstacles, exits); break;
            case Mode::GPU_SIM: updateGPU(agents, obstacles, exits); break;
            default: break;
            }
        }
        double t1 = omp_get_wtime();

        st.frameTime = t1 - t0;
        st.totalTime = t1 - startTime;
        frameCount++;
        if (fpsClock.getElapsedTime().asSeconds() >= 0.5f) {
            st.fps = frameCount / fpsClock.getElapsedTime().asSeconds();
            frameCount = 0;
            fpsClock.restart();
        }

        st.evacuated = 0;
        for (const auto& a : agents) if (a.evacuated) st.evacuated++;
        st.remaining = NUM_AGENTS - st.evacuated;
        if (!st.done && st.evacuated == NUM_AGENTS) st.done = true;

        // Update visualization helpers
        updateTrails(agents);
        recordSample((float)st.totalTime, st.evacuated);

        drawSim(win, font, agents, st, mode, exits, obstacles, showHeatmap);
    }
    return Mode::MENU;
}

// ─── Entry point ─────────────────────────────────────────
int main()
{
    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT),
        "Parallel Crowd Evacuation Simulator",
        sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf"))
        if (!font.loadFromFile("C:\\Windows\\Fonts\\segoeui.ttf"))
            return -1;

    Mode current = Mode::MENU;
    while (window.isOpen()) {
        if (current == Mode::MENU) {
            sf::Event ev;
            bool hasEv = window.pollEvent(ev);
            if (hasEv && ev.type == sf::Event::Closed)
                window.close();
            else {
                if (!hasEv) ev.type = sf::Event::Count;
                Mode next = drawMenu(window, font, ev);
                if (next != Mode::MENU) current = next;
            }
        }
        else {
            current = runSimulation(window, font, current);
        }
    }
    return 0;
}