#include "constants.h"
#include "world.h"
#include "physics.h"
#include "simulation.h"
#include "renderer.h"
#include <SFML/Graphics.hpp>
#include <omp.h>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iomanip>

// ─── Completion Report Dialog ─────────────────────────────
static void drawCompletionReport(sf::RenderWindow& win, sf::Font& font,
    Mode mode, const Stats& st)
{
    // Dim overlay
    sf::RectangleShape overlay({ (float)WIDTH, (float)HEIGHT });
    overlay.setFillColor({ 0, 0, 0, 180 });
    win.draw(overlay);

    float dw = 520, dh = 370;
    float dx = WIDTH / 2.f - dw / 2.f;
    float dy = HEIGHT / 2.f - dh / 2.f;

    // Shadow
    sf::RectangleShape shadow({ dw + 8, dh + 8 });
    shadow.setPosition(dx + 6, dy + 6);
    shadow.setFillColor({ 0, 0, 0, 100 });
    win.draw(shadow);

    // Background
    drawRoundedRect(win, dx, dy, dw, dh, { 18, 18, 32 }, 10.f);
    sf::RectangleShape border({ dw, dh });
    border.setPosition(dx, dy);
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(st.modeColor);
    border.setOutlineThickness(2.f);
    win.draw(border);

    // Top color bar
    sf::RectangleShape topBar({ dw, 4 });
    topBar.setPosition(dx, dy);
    topBar.setFillColor(st.modeColor);
    win.draw(topBar);

    // Green check circle
    sf::CircleShape circle(26);
    circle.setFillColor({ 30, 110, 60 });
    circle.setPosition(dx + dw / 2.f - 26, dy + 18);
    win.draw(circle);
    sf::Text check("✓", font, 30);
    check.setFillColor(sf::Color::White);
    check.setStyle(sf::Text::Bold);
    check.setPosition(dx + dw / 2.f - 11, dy + 21);
    win.draw(check);

    // Title
    sf::Text title("EVACUATION COMPLETE", font, 22);
    title.setStyle(sf::Text::Bold);
    title.setFillColor(Palette::GREEN);
    title.setPosition(dx + dw / 2.f - title.getLocalBounds().width / 2.f, dy + 78);
    win.draw(title);

    sf::Text sub(st.modeName, font, 13);
    sub.setFillColor(st.modeColor);
    sub.setPosition(dx + dw / 2.f - sub.getLocalBounds().width / 2.f, dy + 106);
    win.draw(sub);

    // Divider
    sf::RectangleShape div({ dw - 60.f, 1.f });
    div.setPosition(dx + 30, dy + 130);
    div.setFillColor({ 50, 50, 80 });
    win.draw(div);

    // Stats
    auto row = [&](float rx, float ry, const std::string& lbl, const std::string& val, sf::Color vc) {
        sf::Text l(lbl, font, 12); l.setFillColor(Palette::DIMWHITE); l.setPosition(rx, ry); win.draw(l);
        sf::Text v(val, font, 13); v.setFillColor(vc); v.setStyle(sf::Text::Bold); v.setPosition(rx + 160, ry); win.draw(v);
        };

    std::ostringstream ss;
    float ry = dy + 146;
    float c1 = dx + 40, c2 = dx + dw / 2.f + 10;

    ss << std::fixed << std::setprecision(2) << st.totalTime << " s";
    row(c1, ry, "Total Time", ss.str(), Palette::YELLOW);
    row(c2, ry, "Agents", std::to_string(NUM_AGENTS), Palette::WHITE);
    ry += 26;

    ss.str(""); ss << std::fixed << std::setprecision(1) << st.fps << " fps";
    row(c1, ry, "Avg FPS", ss.str(), Palette::CYAN);
    row(c2, ry, "Threads", std::to_string(st.threadCount), Palette::ORANGE);
    ry += 26;

    ss.str(""); ss << std::fixed << std::setprecision(2) << st.frameTime * 1000.0 << " ms";
    row(c1, ry, "Frame Time", ss.str(), Palette::CYAN);

    std::string strat;
    if (mode == Mode::SERIAL)       strat = "Baseline (1.0x)";
    else if (mode == Mode::OPENMP)  strat = std::to_string(st.threadCount) + " cores (OpenMP)";
    else if (mode == Mode::MPI_SIM) strat = "4 spatial zones";
    else                            strat = "2-phase GPU-style";
    row(c2, ry, "Strategy", strat, st.modeColor);
    ry += 34;

    div.setPosition(dx + 30, ry); win.draw(div);
    ry += 14;

    // Buttons
    float bw = 190, bh = 40;
    float b1x = dx + 35, b1y = ry;
    float b2x = dx + dw - 35 - bw, b2y = ry;

    sf::Vector2i mp = sf::Mouse::getPosition(win);
    bool h1 = mp.x >= b1x && mp.x <= b1x + bw && mp.y >= b1y && mp.y <= b1y + bh;
    bool h2 = mp.x >= b2x && mp.x <= b2x + bw && mp.y >= b2y && mp.y <= b2y + bh;

    drawRoundedRect(win, b1x, b1y, bw, bh, h1 ? Palette::ACCENT : sf::Color(35, 35, 65), 6.f);
    sf::Text t1("Back to Menu", font, 13);
    t1.setFillColor(sf::Color::White);
    t1.setPosition(b1x + bw / 2.f - t1.getLocalBounds().width / 2.f, b1y + 11);
    win.draw(t1);

    drawRoundedRect(win, b2x, b2y, bw, bh, h2 ? Palette::GREEN : sf::Color(20, 55, 35), 6.f);
    sf::Text t2("Run Again", font, 13);
    t2.setFillColor(sf::Color::White);
    t2.setPosition(b2x + bw / 2.f - t2.getLocalBounds().width / 2.f, b2y + 11);
    win.draw(t2);
}

// Returns button clicked: 0 = none, 1 = back to menu, 2 = run again
static int reportHitTest(sf::RenderWindow& win, const sf::Event& ev, const Stats& st)
{
    if (ev.type != sf::Event::MouseButtonPressed ||
        ev.mouseButton.button != sf::Mouse::Left) return 0;

    float dw = 520, dh = 370;
    float dx = WIDTH / 2.f - dw / 2.f;
    float dy = HEIGHT / 2.f - dh / 2.f;
    float bw = 190, bh = 40;
    // ry mirrors the calculation in drawCompletionReport
    float ry = dy + 146 + 26 + 26 + 34 + 14;
    float b1x = dx + 35, b1y = ry;
    float b2x = dx + dw - 35 - bw, b2y = ry;

    int mx = ev.mouseButton.x, my = ev.mouseButton.y;
    if (mx >= b1x && mx <= b1x + bw && my >= b1y && my <= b1y + bh) return 1;
    if (mx >= b2x && mx <= b2x + bw && my >= b2y && my <= b2y + bh) return 2;
    return 0;
}

// ─── Run one simulation ───────────────────────────────────
static Mode runSimulation(sf::RenderWindow& win, sf::Font& font, Mode mode)
{
    srand((unsigned)time(nullptr));
    auto exits = makeExits();
    auto obstacles = makeObstacles();
    auto agents = initAgents(obstacles, exits);

    initTrails((int)agents.size());
    resetGraph();

    Stats st;
    switch (mode) {
    case Mode::SERIAL:  st.modeName = "SERIAL MODE";    st.modeColor = Palette::SERIAL_C; st.threadCount = 1;                    break;
    case Mode::OPENMP:  st.modeName = "OpenMP MODE";    st.modeColor = Palette::OMP_C;   st.threadCount = omp_get_max_threads(); break;
    case Mode::MPI_SIM: st.modeName = "MPI-STYLE MODE"; st.modeColor = Palette::MPI_C;   st.threadCount = 4;                    break;
    case Mode::GPU_SIM: st.modeName = "GPU-STYLE MODE"; st.modeColor = Palette::GPU_C;   st.threadCount = 16;                   break;
    default: break;
    }

    bool showHeatmap = true;
    bool showReport = false;
    double evacuTime = 0.0;

    double startTime = omp_get_wtime();
    sf::Clock fpsClock;
    int frameCount = 0;

    while (win.isOpen()) {
        sf::Event ev;
        while (win.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) { win.close(); return Mode::MENU; }
            if (ev.type == sf::Event::KeyPressed) {
                if (ev.key.code == sf::Keyboard::Escape) return Mode::MENU;
                if (ev.key.code == sf::Keyboard::H)      showHeatmap = !showHeatmap;
            }
            if (showReport) {
                int btn = reportHitTest(win, ev, st);
                if (btn == 1) return Mode::MENU;
                if (btn == 2) return runSimulation(win, font, mode); // restart
            }
        }

        // Simulate only while not done
        if (!st.done) {
            double t0 = omp_get_wtime();
            switch (mode) {
            case Mode::SERIAL:  updateSerial(agents, obstacles, exits); break;
            case Mode::OPENMP:  updateOpenMP(agents, obstacles, exits); break;
            case Mode::MPI_SIM: updateMPI(agents, obstacles, exits); break;
            case Mode::GPU_SIM: updateGPU(agents, obstacles, exits); break;
            default: break;
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

            if (st.evacuated == NUM_AGENTS) {
                st.done = true;
                evacuTime = st.totalTime;   // freeze clock
                showReport = true;
            }

            updateTrails(agents);
            recordSample((float)st.totalTime, st.evacuated);
        }

        // Freeze displayed time once done
        if (st.done) st.totalTime = evacuTime;

        drawSim(win, font, agents, st, mode, exits, obstacles, showHeatmap);

        if (showReport) {
            drawCompletionReport(win, font, mode, st);
            win.display();
        }
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
            if (hasEv && ev.type == sf::Event::Closed) window.close();
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