#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <stdio.h>
#include <stdarg.h>

void printRedDebug(int debugMode, const char *format, ...);
void printGreenDebug(int debugMode, const char *format, ...);
void printYellowDebug(int debugMode, const char *format, ...);
void printBlueDebug(int debugMode, const char *format, ...);
void printWhiteDebug(int debugMode, const char *format, ...);
void printBoldDebug(int debugMode, const char *format, ...);
void printUnderlinedDebug(int debugMode, const char *format, ...);


void printRed(const char *format, ...);
void printGreen(const char *format, ...);
void printYellow(const char *format, ...);
void printBlue(const char *format, ...);
void printWhite(const char *format, ...);
void printBold(const char *format, ...);
void printUnderlined(const char *format, ...);

#endif // LOG_UTILS_H
