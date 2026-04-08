#pragma once
#include "constants.h"
#include "world.h"
#include <SFML/Graphics.hpp>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <deque>
#include <algorithm>
#include <cmath>

// ─── Trail storage (one deque per agent, max 8 positions) ────
inline std::vector<std::deque<sf::Vector2f>> g_trails;

inline void initTrails(int numAgents) {
    g_trails.assign(numAgents, std::deque<sf::Vector2f>());
}

// ─── Evacuation history (sampled over time) ───────────────
struct GraphSample { float t; int evacuated; };
inline std::vector<GraphSample> g_evacHistory;
inline float g_lastSampleTime = 0.f;

inline void recordSample(float t, int evacuated) {
    if (t - g_lastSampleTime >= 0.25f || g_evacHistory.empty()) {
        g_evacHistory.push_back({ t, evacuated });
        g_lastSampleTime = t;
        if (g_evacHistory.size() > 300) g_evacHistory.erase(g_evacHistory.begin());
    }
}

inline void resetGraph() {
    g_evacHistory.clear();
    g_lastSampleTime = 0.f;
}

// ─── Draw helpers ────────────────────────────────────────
inline void drawRoundedRect(sf::RenderWindow& w, float x, float y,
    float ww, float hh, sf::Color c, float r = 6.f)
{
    sf::RectangleShape rect({ ww - 2 * r, hh });
    rect.setPosition(x + r, y); rect.setFillColor(c); w.draw(rect);
    rect.setSize({ ww, hh - 2 * r });
    rect.setPosition(x, y + r); w.draw(rect);
    sf::CircleShape corner(r);
    corner.setFillColor(c);
    corner.setPosition(x, y); w.draw(corner);
    corner.setPosition(x + ww - 2 * r, y); w.draw(corner);
    corner.setPosition(x, y + hh - 2 * r); w.draw(corner);
    corner.setPosition(x + ww - 2 * r, y + hh - 2 * r); w.draw(corner);
}

inline void drawText(sf::RenderWindow& w, sf::Font& f,
    const std::string& s, float x, float y,
    sf::Color c, int size = 14)
{
    sf::Text t(s, f, size);
    t.setFillColor(c);
    t.setPosition(x, y);
    w.draw(t);
}

inline void drawTextBold(sf::RenderWindow& w, sf::Font& f,
    const std::string& s, float x, float y,
    sf::Color c, int size = 14)
{
    sf::Text t(s, f, size);
    t.setFillColor(c);
    t.setStyle(sf::Text::Bold);
    t.setPosition(x, y);
    w.draw(t);
}

// ─── Heatmap ─────────────────────────────────────────────
constexpr int HEAT_COLS = 50;
constexpr int HEAT_ROWS = 30;

inline sf::Color heatColor(float norm) {
    // Blue(cold) → Cyan → Green → Yellow → Red(hot)
    norm = std::max(0.f, std::min(1.f, norm));
    if (norm < 0.25f) {
        float t = norm / 0.25f;
        return { 0, (sf::Uint8)(t * 200), (sf::Uint8)(255 - t * 55), 120 };
    }
    else if (norm < 0.5f) {
        float t = (norm - 0.25f) / 0.25f;
        return { 0, (sf::Uint8)(200 + t * 55), (sf::Uint8)(200 - t * 200), 130 };
    }
    else if (norm < 0.75f) {
        float t = (norm - 0.5f) / 0.25f;
        return { (sf::Uint8)(t * 255), 255, 0, 140 };
    }
    else {
        float t = (norm - 0.75f) / 0.25f;
        return { 255, (sf::Uint8)(255 - t * 255), 0, 150 };
    }
}

inline void drawHeatmap(sf::RenderWindow& win,
    const std::vector<Agent>& ag,
    int simW)
{
    float roomW = WALL_RIGHT - WALL_LEFT;
    float roomH = WALL_BOTTOM - WALL_TOP;
    float cellW = roomW / HEAT_COLS;
    float cellH = roomH / HEAT_ROWS;

    // Count agents per cell
    std::vector<int> grid(HEAT_COLS * HEAT_ROWS, 0);
    for (const auto& a : ag) {
        if (a.evacuated) continue;
        int col = (int)((a.x - WALL_LEFT) / cellW);
        int row = (int)((a.y - WALL_TOP) / cellH);
        col = std::max(0, std::min(HEAT_COLS - 1, col));
        row = std::max(0, std::min(HEAT_ROWS - 1, row));
        grid[row * HEAT_COLS + col]++;
    }

    int maxDensity = 1;
    for (int v : grid) maxDensity = std::max(maxDensity, v);

    sf::RectangleShape cell({ cellW, cellH });
    for (int r = 0; r < HEAT_ROWS; r++) {
        for (int c = 0; c < HEAT_COLS; c++) {
            int cnt = grid[r * HEAT_COLS + c];
            if (cnt == 0) continue;
            float norm = (float)cnt / maxDensity;
            cell.setPosition(WALL_LEFT + c * cellW, WALL_TOP + r * cellH);
            cell.setFillColor(heatColor(norm));
            win.draw(cell);
        }
    }
}

// ─── Agent Trails ─────────────────────────────────────────
inline void updateTrails(const std::vector<Agent>& ag) {
    if (g_trails.size() != ag.size()) {
        g_trails.resize(ag.size());
    }
    for (int i = 0; i < (int)ag.size(); i++) {
        if (ag[i].evacuated) { g_trails[i].clear(); continue; }
        g_trails[i].push_back({ ag[i].x, ag[i].y });
        if (g_trails[i].size() > 8) g_trails[i].pop_front();
    }
}

inline void drawTrails(sf::RenderWindow& win, const std::vector<Agent>& ag) {
    sf::CircleShape dot(1.5f);
    for (int i = 0; i < (int)ag.size(); i++) {
        if (ag[i].evacuated || g_trails[i].size() < 2) continue;
        sf::Color base = ag[i].color;
        int sz = (int)g_trails[i].size();
        for (int j = 0; j < sz - 1; j++) {
            float alpha = 20.f + 60.f * (float)j / sz;
            dot.setFillColor({ base.r, base.g, base.b, (sf::Uint8)alpha });
            dot.setPosition(g_trails[i][j].x - 1.5f, g_trails[i][j].y - 1.5f);
            win.draw(dot);
        }
    }
}

// ─── Evacuation Line Graph ────────────────────────────────
inline void drawEvacGraph(sf::RenderWindow& win, sf::Font& font,
    float px, float& py, float panelW)
{
    float gw = panelW - 30;
    float gh = 70.f;

    // Background
    sf::RectangleShape bg({ gw, gh });
    bg.setPosition(px, py);
    bg.setFillColor({ 15, 15, 30, 220 });
    bg.setOutlineColor({ 60, 60, 90 });
    bg.setOutlineThickness(1.f);
    win.draw(bg);

    drawTextBold(win, font, "EVACUATION GRAPH", px, py - 16, Palette::ACCENT, 11);

    if (g_evacHistory.size() < 2) { py += gh + 20; return; }

    float maxT = g_evacHistory.back().t;
    float minT = g_evacHistory.front().t;
    float tRange = std::max(1.f, maxT - minT);

    sf::VertexArray line(sf::LineStrip, g_evacHistory.size());
    for (int i = 0; i < (int)g_evacHistory.size(); i++) {
        float nx = (g_evacHistory[i].t - minT) / tRange;
        float ny = 1.f - (float)g_evacHistory[i].evacuated / NUM_AGENTS;
        float fx = px + nx * gw;
        float fy = py + ny * gh;
        float pct = (float)g_evacHistory[i].evacuated / NUM_AGENTS;
        sf::Color c = pct > 0.7f ? Palette::GREEN : pct > 0.3f ? Palette::YELLOW : Palette::OMP_C;
        line[i] = sf::Vertex({ fx, fy }, c);
    }
    win.draw(line);

    // Axis labels
    drawText(win, font, "0%", px, py + gh - 12, Palette::DIMWHITE, 9);
    drawText(win, font, "100%", px, py + 2, Palette::DIMWHITE, 9);
    drawText(win, font, "0s", px, py + gh + 1, Palette::DIMWHITE, 9);
    std::ostringstream ss; ss << std::fixed << std::setprecision(0) << maxT << "s";
    drawText(win, font, ss.str(), px + gw - 20, py + gh + 1, Palette::DIMWHITE, 9);

    py += gh + 26;
}

// ─── MENU ────────────────────────────────────────────────
inline Mode drawMenu(sf::RenderWindow& win, sf::Font& font, sf::Event& ev)
{
    win.clear(Palette::BG);

    int W = win.getSize().x;
    int H = win.getSize().y;

    // Subtle grid background
    sf::RectangleShape gridLine;
    gridLine.setFillColor({ 255, 255, 255, 5 });
    for (int x = 0; x < W; x += 60) {
        gridLine.setSize({ 1, (float)H });
        gridLine.setPosition((float)x, 0);
        win.draw(gridLine);
    }
    for (int y = 0; y < H; y += 60) {
        gridLine.setSize({ (float)W, 1 });
        gridLine.setPosition(0, (float)y);
        win.draw(gridLine);
    }

    // Top accent bar
    sf::RectangleShape topBar({ (float)W, 3 });
    topBar.setPosition(0, 0);
    topBar.setFillColor(Palette::ACCENT);
    win.draw(topBar);

    // Title
    sf::Text title("CROWD EVACUATION SIMULATOR", font, 30);
    title.setFillColor(Palette::ACCENT);
    title.setStyle(sf::Text::Bold);
    title.setPosition(W / 2.f - title.getLocalBounds().width / 2.f, 35);
    win.draw(title);

    sf::Text sub("Select a Parallel Computing Mode to Begin", font, 15);
    sub.setFillColor(Palette::DIMWHITE);
    sub.setPosition(W / 2.f - sub.getLocalBounds().width / 2.f, 78);
    win.draw(sub);

    // Mode buttons
    struct Btn { std::string label, desc, tag; sf::Color col; Mode mode; };
    std::vector<Btn> btns = {
        {"SERIAL",  "Single CPU core  \u2022  Baseline reference",        "1 Thread",   Palette::SERIAL_C, Mode::SERIAL },
        {"OpenMP",  "Multi-core CPU   \u2022  Dynamic work-stealing",     "N Threads",  Palette::OMP_C,   Mode::OPENMP },
        {"MPI",     "Spatial Zones    \u2022  Reduced communication",     "4 Threads",  Palette::MPI_C,   Mode::MPI_SIM},
        {"GPU",     "2-Phase Parallel \u2022  Barrier synchronisation",   "16 Threads", Palette::GPU_C,   Mode::GPU_SIM},
    };

    const float bw = 310, bh = 100, gap = 24;
    const float startX = W / 2.f - (bw + gap / 2.f);
    const float startY = 120;

    sf::Vector2i mp = sf::Mouse::getPosition(win);

    for (int i = 0; i < 4; i++) {
        float bx = startX + (i % 2) * (bw + gap);
        float by = startY + (i / 2) * (bh + gap);

        bool hover = mp.x >= bx && mp.x <= bx + bw && mp.y >= by && mp.y <= by + bh;
        sf::Color base = btns[i].col;
        sf::Color bg = hover
            ? sf::Color(base.r, base.g, base.b, 220)
            : sf::Color(base.r / 6, base.g / 6, base.b / 6, 200);

        drawRoundedRect(win, bx, by, bw, bh, bg, 8.f);

        // Border
        sf::RectangleShape border({ bw, bh });
        border.setPosition(bx, by);
        border.setFillColor(sf::Color::Transparent);
        border.setOutlineColor(hover ? base : sf::Color(base.r, base.g, base.b, 100));
        border.setOutlineThickness(hover ? 2.f : 1.f);
        win.draw(border);

        // Left color strip
        sf::RectangleShape strip({ 4, bh - 16 });
        strip.setPosition(bx + 8, by + 8);
        strip.setFillColor(hover ? sf::Color::Black : base);
        win.draw(strip);

        sf::Text lbl(btns[i].label, font, 22);
        lbl.setStyle(sf::Text::Bold);
        lbl.setFillColor(hover ? sf::Color::Black : base);
        lbl.setPosition(bx + 22, by + 14);
        win.draw(lbl);

        sf::Text dsc(btns[i].desc, font, 12);
        dsc.setFillColor(hover ? sf::Color(40, 40, 40) : Palette::DIMWHITE);
        dsc.setPosition(bx + 22, by + 46);
        win.draw(dsc);

        // Tag badge
        drawRoundedRect(win, bx + bw - 80, by + 12, 68, 20,
            hover ? sf::Color(0, 0, 0, 60) : sf::Color(base.r, base.g, base.b, 40), 4.f);
        sf::Text tag(btns[i].tag, font, 11);
        tag.setFillColor(hover ? sf::Color::Black : base);
        tag.setPosition(bx + bw - 76, by + 14);
        win.draw(tag);

        if (hover && ev.type == sf::Event::MouseButtonPressed
            && ev.mouseButton.button == sf::Mouse::Left)
            return btns[i].mode;
    }

    // Info panel
    float iy = startY + 2 * (bh + gap) + 16;
    float iw = 2 * bw + gap;
    drawRoundedRect(win, startX, iy, iw, 130, Palette::PANEL, 8.f);
    sf::RectangleShape infoAccent({ 3, 130 });
    infoAccent.setPosition(startX, iy);
    infoAccent.setFillColor(Palette::ACCENT);
    win.draw(infoAccent);

    drawTextBold(win, font, "HOW IT WORKS", startX + 18, iy + 10, Palette::ACCENT, 13);
    drawText(win, font,
        "\u2022 Each mode simulates " + std::to_string(NUM_AGENTS) + " agents evacuating a building through 4 exits.",
        startX + 18, iy + 32, Palette::DIMWHITE, 12);
    drawText(win, font,
        "\u2022 Agents use separation forces + exit steering. Obstacles create realistic bottlenecks.",
        startX + 18, iy + 52, Palette::DIMWHITE, 12);
    drawText(win, font,
        "\u2022 Heatmap overlay \u2022  Live evacuation graph \u2022  Agent trails show crowd dynamics.",
        startX + 18, iy + 72, Palette::DIMWHITE, 12);
    drawText(win, font,
        "\u2022 Compare FPS, frame time, and total evacuation time across all parallel strategies.",
        startX + 18, iy + 92, Palette::DIMWHITE, 12);
    drawText(win, font,
        "Press ESC during simulation to return here.",
        startX + 18, iy + 112, Palette::YELLOW, 12);

    win.display();
    return Mode::MENU;
}

// ─── SIMULATION FRAME ─────────────────────────────────────
inline void drawSim(sf::RenderWindow& win, sf::Font& font,
    std::vector<Agent>& ag, Stats& st, Mode mode,
    const std::vector<Exit>& exits,
    const std::vector<Obstacle>& obstacles,
    bool showHeatmap = true)
{
    win.clear(Palette::BG);
    int simW = WIDTH - PANEL_WIDTH;

    // Sim area background
    sf::RectangleShape simArea({ (float)simW, (float)HEIGHT });
    simArea.setPosition(0, 0);
    simArea.setFillColor({ 16, 16, 28 });
    win.draw(simArea);

    // Subtle dot grid
    sf::CircleShape dot(1.f);
    dot.setFillColor({ 255, 255, 255, 8 });
    for (int x = (int)WALL_LEFT; x < (int)WALL_RIGHT; x += 40) {
        for (int y = (int)WALL_TOP; y < (int)WALL_BOTTOM; y += 40) {
            dot.setPosition((float)x, (float)y);
            win.draw(dot);
        }
    }

    // ── Walls ──
    auto drawWall = [&](float x, float y, float w, float h) {
        sf::RectangleShape r({ w, h });
        r.setPosition(x, y);
        r.setFillColor(Palette::WALL_C);
        r.setOutlineColor({ 110, 110, 160 });
        r.setOutlineThickness(1.f);
        win.draw(r);
        };

    drawWall(150, 0, WALL_THICKNESS, (float)HEIGHT);
    { sf::RectangleShape gap({ (float)WALL_THICKNESS, (float)EXIT_WIDTH }); gap.setPosition(150, exits[0].y0); gap.setFillColor({ 16,16,28 }); win.draw(gap); }

    drawWall(WALL_RIGHT, 0, WALL_THICKNESS, (float)HEIGHT);
    { sf::RectangleShape gap({ (float)WALL_THICKNESS, (float)EXIT_WIDTH }); gap.setPosition(WALL_RIGHT, exits[1].y0); gap.setFillColor({ 16,16,28 }); win.draw(gap); }

    drawWall(0, 50, (float)simW, WALL_THICKNESS);
    { sf::RectangleShape gap({ (float)EXIT_WIDTH, (float)WALL_THICKNESS }); gap.setPosition(exits[2].x0, 50); gap.setFillColor({ 16,16,28 }); win.draw(gap); }

    drawWall(0, HEIGHT - WALL_THICKNESS - 50, (float)simW, WALL_THICKNESS);
    { sf::RectangleShape gap({ (float)EXIT_WIDTH, (float)WALL_THICKNESS }); gap.setPosition(exits[3].x0, HEIGHT - WALL_THICKNESS - 50); gap.setFillColor({ 16,16,28 }); win.draw(gap); }

    // ── Heatmap (below agents) ──
    if (showHeatmap) drawHeatmap(win, ag, simW);

    // ── MPI zone dividers ──
    if (mode == Mode::MPI_SIM) {
        sf::RectangleShape vline({ 1, (float)HEIGHT });
        vline.setPosition(simW / 2.f, 0);
        vline.setFillColor({ 180, 130, 255, 40 });
        win.draw(vline);
        sf::RectangleShape hline({ (float)simW, 1 });
        hline.setPosition(0, HEIGHT / 2.f);
        hline.setFillColor({ 180, 130, 255, 40 });
        win.draw(hline);
        for (int i = 0; i < 4; i++) {
            float zx = (i % 2) * (simW / 2.f) + 60;
            float zy = (i / 2) * (HEIGHT / 2.f) + 60;
            drawText(win, font, "Zone " + std::to_string(i), zx, zy, { 180, 130, 255, 80 }, 12);
        }
    }

    // ── Obstacles ──
    for (const auto& o : obstacles) {
        sf::RectangleShape r({ o.w, o.h });
        r.setPosition(o.x, o.y);
        r.setFillColor({ 45, 45, 70 });
        r.setOutlineColor({ 90, 90, 130 });
        r.setOutlineThickness(1.5f);
        win.draw(r);
        // Hatching lines
        sf::RectangleShape hatch({ o.w, 1 });
        hatch.setFillColor({ 90, 90, 130, 60 });
        for (float hy = o.y + 12; hy < o.y + o.h - 4; hy += 14) {
            hatch.setPosition(o.x, hy);
            win.draw(hatch);
        }
        drawText(win, font, "WALL", o.x + 4, o.y + o.h / 2 - 7, { 100, 100, 140 }, 10);
    }

    // ── Exits ──
    for (const auto& e : exits) {
        // Glow pulse effect (static glow)
        sf::CircleShape glow(30);
        glow.setFillColor({ 80, 220, 100, 30 });
        glow.setPosition(e.x - 30, e.y - 30);
        win.draw(glow);
        glow.setRadius(18);
        glow.setFillColor({ 80, 220, 100, 50 });
        glow.setPosition(e.x - 18, e.y - 18);
        win.draw(glow);

        sf::CircleShape exitDot(10);
        exitDot.setFillColor(Palette::GREEN);
        exitDot.setPosition(e.x - 10, e.y - 10);
        win.draw(exitDot);
        drawTextBold(win, font, "EXIT " + e.label, e.x - 20, e.y + 12, Palette::GREEN, 11);
    }

    // ── Trails ──
    drawTrails(win, ag);

    // ── Agents ──
    sf::CircleShape agGlow(AGENT_RADIUS + 3), agShape(AGENT_RADIUS);
    for (const auto& a : ag) {
        if (a.evacuated) continue;
        sf::Color c = a.color;

        agGlow.setFillColor({ c.r, c.g, c.b, 25 });
        agGlow.setPosition(a.x - AGENT_RADIUS - 3, a.y - AGENT_RADIUS - 3);
        win.draw(agGlow);

        agShape.setFillColor(c);
        agShape.setPosition(a.x - AGENT_RADIUS, a.y - AGENT_RADIUS);
        win.draw(agShape);
    }

    // ─── RIGHT PANEL ──────────────────────────────────────
    sf::RectangleShape panel({ (float)PANEL_WIDTH, (float)HEIGHT });
    panel.setPosition((float)simW, 0);
    panel.setFillColor({ 14, 14, 24 });
    win.draw(panel);

    // Panel top accent
    sf::RectangleShape panelAccent({ (float)PANEL_WIDTH, 3 });
    panelAccent.setPosition((float)simW, 0);
    panelAccent.setFillColor(st.modeColor);
    win.draw(panelAccent);

    // Separator line
    sf::RectangleShape sep({ 1, (float)HEIGHT });
    sep.setPosition((float)simW, 0);
    sep.setFillColor({ 50, 50, 80 });
    win.draw(sep);

    float px = simW + 14, py = 12;

    // Mode badge
    drawRoundedRect(win, px, py, PANEL_WIDTH - 28, 34, st.modeColor, 6.f);
    sf::Text modeLabel(st.modeName, font, 15);
    modeLabel.setStyle(sf::Text::Bold);
    modeLabel.setFillColor(sf::Color::Black);
    modeLabel.setPosition(px + (PANEL_WIDTH - 28) / 2.f - modeLabel.getLocalBounds().width / 2.f, py + 8);
    win.draw(modeLabel);
    py += 44;

    // Section helper
    auto sectionHeader = [&](const std::string& s) {
        sf::RectangleShape line({ PANEL_WIDTH - 28.f, 1 });
        line.setPosition(px, py);
        line.setFillColor({ 50, 50, 80 });
        win.draw(line);
        py += 5;
        drawTextBold(win, font, s, px, py, Palette::ACCENT, 11);
        py += 18;
        };

    auto statRow = [&](const std::string& lbl, const std::string& val, sf::Color vc) {
        drawText(win, font, lbl, px, py, Palette::DIMWHITE, 12);
        sf::Text v(val, font, 12);
        v.setFillColor(vc);
        v.setStyle(sf::Text::Bold);
        v.setPosition(px + PANEL_WIDTH - 28 - v.getLocalBounds().width, py);
        win.draw(v);
        py += 19;
        };

    std::ostringstream ss;

    sectionHeader("PERFORMANCE");
    ss << std::fixed << std::setprecision(1) << st.fps << " fps";
    statRow("FPS", ss.str(), st.fps > 30 ? Palette::GREEN : st.fps > 15 ? Palette::YELLOW : Palette::RED);
    ss.str(""); ss << std::fixed << std::setprecision(2) << st.frameTime * 1000 << " ms";
    statRow("Frame Time", ss.str(), Palette::CYAN);
    ss.str(""); ss << std::fixed << std::setprecision(1) << st.totalTime << " s";
    statRow("Elapsed", ss.str(), Palette::WHITE);
    if (st.done) {
        ss.str(""); ss << std::fixed << std::setprecision(2) << st.totalTime << " s";
        statRow("Evac Time", ss.str(), Palette::YELLOW);
    }
    statRow("Threads", std::to_string(st.threadCount), Palette::ORANGE);
    py += 4;

    sectionHeader("AGENTS");
    statRow("Total", std::to_string(NUM_AGENTS), Palette::WHITE);
    statRow("Evacuated", std::to_string(st.evacuated), Palette::GREEN);
    statRow("Remaining", std::to_string(st.remaining), Palette::RED);
    py += 6;

    // Progress bar
    float pct = (float)st.evacuated / NUM_AGENTS;
    drawText(win, font, "Progress", px, py, Palette::DIMWHITE, 11); py += 14;
    float barW = PANEL_WIDTH - 28.f;
    sf::RectangleShape barBg({ barW, 12 });
    barBg.setPosition(px, py); barBg.setFillColor({ 35, 35, 55 }); win.draw(barBg);
    if (pct > 0) {
        sf::RectangleShape barFg({ barW * pct, 12 });
        barFg.setPosition(px, py);
        barFg.setFillColor(pct > 0.7f ? Palette::GREEN : pct > 0.3f ? Palette::YELLOW : Palette::RED);
        win.draw(barFg);
    }
    ss.str(""); ss << (int)(pct * 100) << "%";
    drawText(win, font, ss.str(), px + barW / 2 - 10, py, sf::Color::White, 10);
    py += 22;

    // ── Live Graph ──
    sectionHeader("EVACUATION GRAPH");
    drawEvacGraph(win, font, px, py, PANEL_WIDTH);

    // ── Heatmap Legend ──
    sectionHeader("DENSITY LEGEND");
    float lx = px, lw = PANEL_WIDTH - 28.f, lh = 10.f;
    sf::RectangleShape lseg({ lw / 20.f, lh });
    for (int k = 0; k < 20; k++) {
        lseg.setPosition(lx + k * lw / 20.f, py);
        lseg.setFillColor(heatColor((float)k / 19.f));
        win.draw(lseg);
    }
    py += 12;
    drawText(win, font, "Low", px, py, Palette::DIMWHITE, 9);
    drawText(win, font, "High", px + lw - 18, py, Palette::DIMWHITE, 9);
    py += 16;

    // ── Algorithm note ──
    sectionHeader("ALGORITHM");
    std::vector<std::string> lines;
    if (mode == Mode::SERIAL)      lines = { "Sequential loop.", "Baseline for", "speedup comparison." };
    else if (mode == Mode::OPENMP) lines = { "Dynamic scheduling.", "Work-stealing across", "all CPU cores." };
    else if (mode == Mode::MPI_SIM)lines = { "4 spatial zones.", "Threads own zones,", "reduce communication." };
    else                           lines = { "Phase 1: compute forces.", "Phase 2: apply forces.", "GPU-style barriers." };
    for (const auto& l : lines) { drawText(win, font, l, px, py, Palette::DIMWHITE, 11); py += 15; }

    // ── Done banner ──
    if (st.done) {
        py += 8;
        drawRoundedRect(win, px, py, PANEL_WIDTH - 28, 44, { 40, 120, 60, 220 }, 6.f);
        sf::RectangleShape doneLine({ PANEL_WIDTH - 28.f, 2 });
        doneLine.setPosition(px, py);
        doneLine.setFillColor(Palette::GREEN);
        win.draw(doneLine);
        drawTextBold(win, font, "EVACUATION COMPLETE", px + 8, py + 8, Palette::GREEN, 13);
        ss.str(""); ss << std::fixed << std::setprecision(2) << st.totalTime << "s total";
        drawText(win, font, ss.str(), px + 8, py + 26, sf::Color::White, 11);
    }

    drawText(win, font, "[ESC] Back to Menu", px, HEIGHT - 22, { 80, 80, 110 }, 11);
    win.display();
}