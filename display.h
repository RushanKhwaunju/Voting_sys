#ifndef DISPLAY_H
#define DISPLAY_H

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <sys/ioctl.h>
  #include <unistd.h>
#endif

using namespace std;

const int BOX_WIDTH = 85;

static int g_termWidth = 80;

static void refreshTerminalWidth() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
        g_termWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    else
        g_termWidth = 80;
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        g_termWidth = (int)w.ws_col;
    else
        g_termWidth = 80;
#endif
}

static string centreOffset() {
    int margin = (g_termWidth - BOX_WIDTH) / 2;
    if (margin < 0) margin = 0;
    return string(margin, ' ');
}

void drawBorder() {
    cout << centreOffset() << "+" << string(BOX_WIDTH - 2, '=') << "+\n";
}

void drawDivider() {
    cout << centreOffset() << "+" << string(BOX_WIDTH - 2, '-') << "+\n";
}

void drawTitle(const string& title) {
    const int inner = BOX_WIDTH - 2;
    int len  = min((int)title.length(), inner);
    int lpad = (inner - len) / 2;
    int rpad = inner - lpad - len;
    cout << centreOffset() << "|" << string(lpad, ' ') << title.substr(0, len) << string(rpad, ' ') << "|\n";
}

void drawLine(const string& text) {
    const int inner = BOX_WIDTH - 3;
    string display = ((int)text.length() > inner) ? text.substr(0, inner) : text;
    cout << centreOffset() << "| " << left << setw(inner) << display << "|\n";
}

void printSuccess(const string& msg) { drawLine("[SUCCESS] " + msg); }
void printError  (const string& msg) { drawLine("[ERROR]   " + msg); }
void printWarning(const string& msg) { drawLine("[WARNING] " + msg); }

void printPrompt(const string& msg) {
    cout << centreOffset() << "| " << msg;
    cout.flush();
}

#endif
