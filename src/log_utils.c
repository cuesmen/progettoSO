#include "log_utils.h"

#define RESET_COLOR "\033[0m"

void printColorDebug(int debugMode, const char *colorCode, const char *format, va_list args) {
    if (debugMode) {
        printf("%s", colorCode);
        vprintf(format, args);
        printf("%s", RESET_COLOR);
        fflush(stdout);
    }
}

void printColor(const char *colorCode, const char *format, va_list args) {
    printf("%s", colorCode);
    vprintf(format, args);
    printf("%s", RESET_COLOR);
    fflush(stdout);
}


void printRedDebug(int debugMode, const char *format, ...) {
    va_list args;
    va_start(args, format);
    printColorDebug(debugMode, "\033[1;31m", format, args);
    va_end(args);
}

void printGreenDebug(int debugMode, const char *format, ...) {
    va_list args;
    va_start(args, format);
    printColorDebug(debugMode, "\033[1;32m", format, args);
    va_end(args);
}

void printYellowDebug(int debugMode, const char *format, ...) {
    va_list args;
    va_start(args, format);
    printColorDebug(debugMode, "\033[1;33m", format, args);
    va_end(args);
}

void printBlueDebug(int debugMode, const char *format, ...) {
    va_list args;
    va_start(args, format);
    printColorDebug(debugMode, "\033[1;34m", format, args);
    va_end(args);
}

void printWhiteDebug(int debugMode, const char *format, ...) {
    va_list args;
    va_start(args, format);
    printColorDebug(debugMode, "\033[1;37m", format, args);
    va_end(args);
}

void printBoldDebug(int debugMode, const char *format, ...) {
    va_list args;
    va_start(args, format);
    printColorDebug(debugMode, "\033[1m", format, args);
    va_end(args);
}

void printUnderlinedDebug(int debugMode, const char *format, ...) {
    va_list args;
    va_start(args, format);
    printColorDebug(debugMode, "\033[4m", format, args);
    va_end(args);
}


void printRed(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printColor("\033[1;31m", format, args);
    va_end(args);
}

void printGreen(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printColor("\033[1;32m", format, args);
    va_end(args);
}

void printYellow(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printColor("\033[1;33m", format, args);
    va_end(args);
}

void printBlue(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printColor("\033[1;34m", format, args);
    va_end(args);
}

void printWhite(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printColor("\033[1;37m", format, args);
    va_end(args);
}

void printBold(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printColor("\033[1m", format, args);
    va_end(args);
}

void printUnderlined(const char *format, ...) {
    va_list args;
    va_start(args, format);
    printColor("\033[4m", format, args);
    va_end(args);
}
