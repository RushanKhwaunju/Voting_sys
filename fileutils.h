// ─────────────────────────────────────────────────────────────────────────────
//  fileutils.h  –  String helpers, file rewriter, roll & password utilities
//
//  These are small tools that many parts of the program need.
//  Putting them here keeps voting_system.cpp clean and easy to read.
// ─────────────────────────────────────────────────────────────────────────────

#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <functional>   // std::function (used by rewriteFile)
#include <iomanip>      // setw, setfill, hex

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
//  String helpers
// ─────────────────────────────────────────────────────────────────────────────

// trim() — removes spaces/tabs/newlines from both ends of a string.
// Example:  "  hello \n" → "hello"
static string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";          // all whitespace → return ""
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// split() — splits a string by a delimiter character.
// Example:  split("a,b,c", ',') → {"a", "b", "c"}
static vector<string> split(const string& s, char delim) {
    vector<string> result;
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim))
        result.push_back(item);
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Roll-number validator
// ─────────────────────────────────────────────────────────────────────────────

// Returns true if the roll number is exactly 6 digit characters (e.g. "810301").
// Used when adding candidates and when a student votes.
static bool isValidRoll(const string& roll) {
    if (roll.length() != 6) return false;        // must be exactly 6 chars
    for (char c : roll)
        if (!isdigit(c)) return false;           // every char must be a digit
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Password hasher (djb2 algorithm)
// ─────────────────────────────────────────────────────────────────────────────

// Converts a plain-text password into a fixed-length hex string.
// The REAL password is NEVER saved to disk — only this hash is saved.
// When a user logs in we hash what they typed and compare to the stored hash.
//
// djb2 is a simple, fast hash invented by Daniel J. Bernstein.
// It is good enough for a school project but NOT for real banking software.
static string hashPassword(const string& password) {
    unsigned long hash = 5381;            // magic starting value for djb2
    for (unsigned char c : password)
        hash = ((hash << 5) + hash) ^ c; // hash * 33 XOR character
    ostringstream oss;
    oss << hex << setw(16) << setfill('0') << hash;  // 16-character hex string
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Generic file rewriter
// ─────────────────────────────────────────────────────────────────────────────

// rewriteFile() reads every line of `path` and keeps only lines for which
// keepLine(trimmed_line) returns true, then writes the survivors back.
//
// This replaces 15+ copy-paste delete/purge blocks in the original code.
// Usage example:
//   rewriteFile("votes.txt", [&](const string& line) {
//       auto parts = split(line, ',');
//       return trim(parts[1]) != deletedEvent;   // drop lines matching the event
//   });
static void rewriteFile(const string& path,
                        function<bool(const string&)> keepLine) {
    // Step 1 — read all lines that pass the filter into a vector
    vector<string> linesToKeep;
    {
        ifstream inFile(path);
        string line;
        while (getline(inFile, line)) {
            string t = trim(line);
            if (!t.empty() && keepLine(t))
                linesToKeep.push_back(t);
        }
    }   // inFile closes here automatically

    // Step 2 — write the survivors back (this overwrites the whole file)
    ofstream outFile(path);
    for (auto& l : linesToKeep)
        outFile << l << "\n";
}   // outFile closes here automatically

#endif // FILEUTILS_H
