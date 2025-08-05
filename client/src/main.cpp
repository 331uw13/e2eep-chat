#include <stdio.h>
#include <ncurses.h>

#include "client.hpp"
#include "../../shared/colors.hpp"
#include "../../shared/log.hpp"



void init_ncurses() {
    initscr();
    raw();    // Dont generate interupt signals.
    noecho(); // Dont print characters that user types.
    keypad(stdscr, 1); // Enable arrow, backspace keys etc...
    curs_set(0);  // Hide cursor.
}

void close_ncurses() {
    curs_set(1);
    endwin();
}

void pair_colors(int color_num, int r, int g, int b, int bg_color_num) {
    init_color(color_num, r << 2, g << 2, b << 2);
    init_pair(color_num, color_num, bg_color_num);
}

void create_color(int color_num, int r, int g, int b, int bg_color_num = Color::BACKGROUND) {
    pair_colors(color_num, r, g, b, bg_color_num);

    // Few next are the darker version of r, g and b.

    const float factor = 0.5;
    for(int i = 0; i < Color::NUM_DARK_SHADES; i++) {
        r = (int)((float)r * factor);
        g = (int)((float)g * factor);
        b = (int)((float)b * factor);

        color_num++;

        pair_colors(color_num, r, g, b, bg_color_num);
    }

}

void init_all_colors() {

    init_color(Color::BACKGROUND, 47, 43, 40);

    create_color(Color::WHITE, 200, 180, 150);
    create_color(Color::BEIGE, 200, 160, 120);
    create_color(Color::PINK, 230, 130, 180);
    create_color(Color::RED, 200, 40, 40);
    create_color(Color::ORANGE, 240, 80, 45);
    create_color(Color::YELLOW, 240, 180, 60);
    create_color(Color::LIME, 180, 230, 32);
    create_color(Color::GREEN, 50, 200, 50);
    create_color(Color::CYAN, 56, 200, 180);
    create_color(Color::BLUE, 40, 160, 230);
    create_color(Color::PURPLE, 180, 90, 240);
    create_color(Color::MAGENTA, 240, 60, 190);

    // Special colors.
    pair_colors(Color::CURSOR,  40, 200, 40,  Color::DARK_GREEN_1);
}


int main(int argc, char** argv) {
    if(argc != 3) {
        printf("Usage: %s <server public key> <nickname>\n", argv[0]);
        return 1;
    }

    assign_logfile("client.log");

    Client client;
    if(client.connect2server(argv[1], argv[2])) {
        init_ncurses();
        start_color();
        init_all_colors();

        client.enter_interaction_loop();
        close_ncurses();
    }

    client.disconnect();

    close_logfile();
    return 0;
}
