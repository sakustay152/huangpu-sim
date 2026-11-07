#include "ui/ConsoleUI.h"

#include <iostream>

#include "core/Game.h"
#include "util/Console.h"

void ConsoleScreen::render() {
    console::clearScreen();
    draw();
}

void ConsoleScreen::header(const std::string& title) const {
    std::cout << console::paint("════════════════════════════════════════════════", console::DIM) << "\n";
    std::cout << console::paint("  " + title, console::BOLD) << "\n";
    std::cout << console::paint("════════════════════════════════════════════════", console::DIM) << "\n";
}

void ConsoleScreen::rule() {
    std::cout << console::paint("────────────────────────────────────────────────", console::DIM) << "\n";
}

void ConsoleScreen::line(const std::string& s) {
    std::cout << (s.empty() ? "" : s) << "\n";
}
