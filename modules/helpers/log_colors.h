#pragma once

static constexpr const char *COL_RESET = "\033[0m";
static constexpr const char *COL_BLACK = "\033[30m";
static constexpr const char *COL_RED = "\033[31m";
static constexpr const char *COL_GREEN = "\033[32m";
static constexpr const char *COL_YELLOW = "\033[33m";
static constexpr const char *COL_BLUE = "\033[34m";
static constexpr const char *COL_MAGENTA = "\033[3m5";
static constexpr const char *COL_CYAN = "\033[36m";
static constexpr const char *COL_WHITE = "\033[37m";

enum LogColor
{
    LOG_COLOR_BLACK,
    LOG_COLOR_RED,
    LOG_COLOR_GREEN,
    LOG_COLOR_YELLOW,
    LOG_COLOR_BLUE,
    LOG_COLOR_MAGENTA,
    LOG_COLOR_CYAN,
    LOG_COLOR_WHITE
};

static constexpr const char *getColorCode(LogColor c){
    switch (c)
    {
    case LOG_COLOR_BLACK:
        return COL_BLACK;

    case LOG_COLOR_RED:
        return COL_RED;
    
    case LOG_COLOR_GREEN:
        return COL_GREEN;
    
    case LOG_COLOR_YELLOW:
        return COL_YELLOW;
    
    case LOG_COLOR_BLUE:
        return COL_BLUE;
    
    case LOG_COLOR_MAGENTA:
        return COL_MAGENTA;
    
    case LOG_COLOR_CYAN:
        return COL_CYAN;
    
    case LOG_COLOR_WHITE:
        return COL_WHITE;
    
    default:
        return COL_RESET;
    }
}