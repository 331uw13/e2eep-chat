#ifndef E2EEP_CLIENT_COLORS_HPP
#define E2EEP_CLIENT_COLORS_HPP



namespace Color {
    static constexpr int NUM_DARK_SHADES = 3;
    enum NCursesColor : int {
        BACKGROUND = 0,
        WHITE,   DARK_WHITE_0,    DARK_WHITE_1, DARK_WHITE_2,
        BEIGE,   DARK_BEIGE_0,    DARK_BEIGE_1, DARK_BEIGE_2,
        PINK,    DARK_PINK_0,     DARK_PINK_1,  DARK_PINK_2,
        RED,     DARK_RED_0,      DARK_RED_1,   DARK_RED_2,
        ORANGE,  DARK_ORANGE_0,   DARK_ORANGE_1, DARK_ORANGE_2,
        YELLOW,  DARK_YELLOW_0,   DARK_YELLOW_1, DARK_YELLOW_2,
        LIME,    DARK_LIME_0,     DARK_LIME_1,   DARK_LIME_2,
        GREEN,   DARK_GREEN_0,    DARK_GREEN_1,  DARK_GREEN_2,
        CYAN,    DARK_CYAN_0,     DARK_CYAN_1,   DARK_CYAN_2,
        BLUE,    DARK_BLUE_0,     DARK_BLUE_1,   DARK_BLUE_2,
        PURPLE,  DARK_PURPLE_0,   DARK_PURPLE_1, DARK_PURPLE_2,
        MAGENTA, DARK_MAGENTA_0,  DARK_MAGENTA_1, DARK_MAGENTA_2,

        // Special colors.
        CURSOR,
        CMD_CURSOR,
        MSG_TITLE,
        SELECT,

        NUM_COLORS
    };
}


#endif
