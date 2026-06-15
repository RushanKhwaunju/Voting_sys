// ─────────────────────────────────────────────────────────────────────────────
//  admin.h  –  Admin login and credential management
//
//  What this file does:
//    • Keeps track of admin usernames and (hashed) passwords in admin.txt
//    • Lets an admin log in, add a new admin, or change credentials
//
//  admin.txt format (one admin per line):
//    username hashedPassword
//  Example:
//    rushan 8e5b9e4a1c2d3f4a
//
//  DEFAULT account (created on first run):
//    Username : rushan
//    Password : 12345
//
//  The REAL password "12345" is NEVER written to disk.
//  Only its djb2 hash is stored (see hashPassword() in fileutils.h).
// ─────────────────────────────────────────────────────────────────────────────

#ifndef ADMIN_H
#define ADMIN_H

#include <iostream>
#include <fstream>
#include <string>
#include "display.h"
#include "fileutils.h"

#ifdef _WIN32
  #include <conio.h>      // _getch() on Windows
#else
  #include <termios.h>    // tcgetattr / tcsetattr on Linux/macOS
  #include <unistd.h>
#endif

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
//  getHiddenInput()
//  Reads a password character-by-character, printing '*' for every keystroke.
//  Supports Backspace to delete the last character.
//  Works on both Windows (_getch) and POSIX (raw-mode read).
// ─────────────────────────────────────────────────────────────────────────────
static string getHiddenInput() {
    string password;

#ifdef _WIN32
    // ── Windows path ─────────────────────────────────────────────────────────
    char ch;
    while ((ch = _getch()) != '\r') {   // '\r' = Enter on Windows
        if (ch == '\b') {               // '\b' = Backspace
            if (!password.empty()) {
                password.pop_back();
                // Erase the last '*' from the screen: back, space, back
                cout << "\b \b";
                cout.flush();
            }
        } else if (ch >= 32) {          // printable character
            password += ch;
            cout << '*';
            cout.flush();
        }
    }
#else
    // ── POSIX path (Linux / macOS) ────────────────────────────────────────────
    // Save the current terminal settings so we can restore them later
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    // ECHO  off — characters are NOT printed automatically
    // ICANON off — read one character at a time (no buffering until Enter)
    newt.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    char ch;
    while (read(STDIN_FILENO, &ch, 1) == 1 && ch != '\n') {
        if (ch == 127 || ch == '\b') {  // 127 = DEL (Backspace on most terminals)
            if (!password.empty()) {
                password.pop_back();
                cout << "\b \b";
                cout.flush();
            }
        } else if (ch >= 32) {          // printable character
            password += ch;
            cout << '*';
            cout.flush();
        }
    }

    // Restore original terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

    cout << '\n';   // move to next line after Enter
    return password;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Admin class
// ─────────────────────────────────────────────────────────────────────────────

// A class groups related data and functions together.
// The Admin class handles everything about admin accounts.
class Admin {

    // The file where admin usernames and password-hashes are saved.
    // "const" means this value never changes.
    const string ADMIN_FILE = "admin.txt";

    // ── Private helper ────────────────────────────────────────────────────────
    // Returns the stored password hash for `username`.
    // Returns "" (empty string) if the username is not found.
    // "private" means only functions inside Admin class can call this.
    string storedHash(const string& username) const {
        ifstream inFile(ADMIN_FILE);
        string u, h;

        // Each line looks like:  rushan 8e5b9e4a1c2d3f4a
        // We read two words at a time: username and hash
        while (inFile >> u >> h) {
            if (u == username)
                return h;   // found the matching username — return its hash
        }
        return "";   // username not found
    }

public:
    // ── Ensure default admin exists ───────────────────────────────────────────
    // Called ONCE at program startup.
    // If admin.txt is missing or empty, this creates the default account.
    void ensureDefaultAdmin() {
        ifstream inFile(ADMIN_FILE);
        // peek() checks the very first character without consuming it.
        // If the file is empty, peek() returns EOF.
        if (!inFile.is_open() || inFile.peek() == EOF) {
            ofstream outFile(ADMIN_FILE);
            // Write:  rushan <hash of "12345">
            // Note: we call hashPassword() from fileutils.h
            outFile << "rushan " << hashPassword("12345") << "\n";
        }
    }

    // ── Admin login ───────────────────────────────────────────────────────────
    // Asks for username and password, checks them against admin.txt.
    // Returns true if login succeeds, false otherwise.
    bool login() {
        refreshTerminalWidth();   // snapshot width before drawing this screen
        drawBorder();
        drawTitle("ADMIN LOGIN");
        drawBorder();
        drawLine("");

        string username, password;

        // Ask for username — the user types on the same line as the prompt
        printPrompt("Username : ");
        cin >> username;
        cin.ignore();   // clear the newline left by cin >> before hidden input

        // Password is read character-by-character; each keystroke shows '*'
        printPrompt("Password : ");
        password = getHiddenInput();

        drawLine("");

        // Hash what the user typed and compare to what is stored in admin.txt
        string expected = storedHash(username);

        if (!expected.empty() && expected == hashPassword(password)) {
            // Hashes match → login successful
            printSuccess("Login successful! Welcome, " + username + ".");
            drawBorder();
            return true;
        }

        // Hashes do NOT match → login failed
        printError("Invalid username or password.");
        drawBorder();
        return false;
    }

    // ── Add a new admin account ───────────────────────────────────────────────
    // Lets the current admin create an additional admin account.
    void addAdmin() {
        refreshTerminalWidth();
        drawBorder();
        drawTitle("ADD ADMIN");
        drawBorder();
        drawLine("");

        string u, p;
        printPrompt("New Username : "); cin >> u;
        cin.ignore();   // flush newline before hidden input
        printPrompt("New Password : ");
        p = getHiddenInput();

        // Check if username already exists — we don't want duplicates
        if (!storedHash(u).empty()) {
            printError("Username already exists!");
            drawBorder();
            return;
        }

        // Append the new admin to admin.txt (ios::app = open in append mode)
        ofstream outFile(ADMIN_FILE, ios::app);
        outFile << u << " " << hashPassword(p) << "\n";

        printSuccess("Admin '" + u + "' added successfully.");
        drawBorder();
    }

    // ── Change admin credentials ──────────────────────────────────────────────
    // Replaces ALL admin credentials with a single new account.
    // (This is intentional — one admin account is simplest for a school system.)
    void changeAdmin() {
        refreshTerminalWidth();
        drawBorder();
        drawTitle("CHANGE ADMIN CREDENTIALS");
        drawBorder();
        drawLine("");

        string u, p;
        printPrompt("New Username : "); cin >> u;
        cin.ignore();   // flush newline before hidden input
        printPrompt("New Password : ");
        p = getHiddenInput();

        // Opening a file WITHOUT ios::app overwrites (truncates) the file.
        // So this replaces everything in admin.txt with one new entry.
        ofstream outFile(ADMIN_FILE);
        outFile << u << " " << hashPassword(p) << "\n";

        printSuccess("Admin credentials updated successfully.");
        drawBorder();
    }
};

#endif // ADMIN_H