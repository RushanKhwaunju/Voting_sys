// ─────────────────────────────────────────────────────────────────────────────
//  election.h  –  ElectionSystem class
//
//  This is the biggest part of the program. It handles:
//    • Creating and managing election events
//    • Adding positions and candidates
//    • Setting deadlines for voting
//    • Letting students cast their votes
//    • Tallying results and declaring winners
//    • Purging expired events (saving winners first)
//
//  Files used:
//    events.txt     — event name + list of positions
//    candidates.txt — candidate roll, name, event, position
//    votes.txt      — voter roll, event, position, chosen candidate's roll
//    deadlines.txt  — event name + deadline timestamp
//    winners.txt    — final results for closed elections
// ─────────────────────────────────────────────────────────────────────────────

#ifndef ELECTION_H
#define ELECTION_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>
#include "display.h"
#include "fileutils.h"
#include "student.h"
#include "admin.h"

using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
//  Candidate data structure
// ─────────────────────────────────────────────────────────────────────────────

// A simple struct to hold one candidate's information
struct Candidate {
    string roll;      // student roll number, e.g. "810301"
    string name;      // candidate name,      e.g. "Aarav Sharma"
    string event;     // which event,          e.g. "Class Election 2025"
    string position;  // which position,       e.g. "President"

    // Default constructor (no arguments) — used when creating empty Candidates
    Candidate() {}

    // Constructor with all four fields — used when loading from file
    Candidate(const string& r, const string& n,
              const string& e, const string& p)
        : roll(r), name(n), event(e), position(p) {}
};

// ─────────────────────────────────────────────────────────────────────────────
//  ElectionSystem class
// ─────────────────────────────────────────────────────────────────────────────

class ElectionSystem {

    // ── File name constants ───────────────────────────────────────────────────
    // These never change; using constants avoids typos in file names.
    const string EVENTS_FILE    = "events.txt";
    const string CANDIDATE_FILE = "candidates.txt";
    const string VOTE_FILE      = "votes.txt";
    const string DEADLINE_FILE  = "deadlines.txt";
    const string WINNERS_FILE   = "winners.txt";

    // ─────────────────────────────────────────────────────────────────────────
    //  Private helper: numbered pick list
    //  Prints a numbered list and asks the user to pick one.
    //  Returns the 0-based index of the chosen item, or -1 if invalid.
    // ─────────────────────────────────────────────────────────────────────────
    int pickFrom(const vector<string>& items) {
        // Print each item with its number
        for (size_t i = 0; i < items.size(); ++i)
            drawLine("  " + to_string(i + 1) + ". " + items[i]);

        drawDivider();
        printPrompt("Choice : ");

        int choice;
        cin >> choice;
        cin.ignore(1000, '\n');  // flush newline so getline callers aren't skipped

        // Validate: must be between 1 and the number of items
        if (choice < 1 || choice > (int)items.size()) {
            printError("Invalid choice!");
            return -1;   // -1 signals "bad input" to the caller
        }

        return choice - 1;   // convert to 0-based index
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Private: load events list from events.txt
    // ─────────────────────────────────────────────────────────────────────────
    // events.txt format:  EventName|position1;position2;position3
    // This function returns just the event NAMES (not positions).
    vector<string> loadEvents() {
        vector<string> events;
        ifstream inFile(EVENTS_FILE);
        string line;
        while (getline(inFile, line)) {
            line = trim(line);
            if (line.empty()) continue;
            size_t bar = line.find('|');
            // Take everything before the '|' as the event name
            events.push_back(bar == string::npos ? line : trim(line.substr(0, bar)));
        }
        return events;
    }

    // Returns true if an event with the given name already exists
    bool eventExists(const string& name) {
        for (auto& e : loadEvents())
            if (e == name) return true;
        return false;
    }

    // Returns the list of positions for a given event name
    // events.txt line example:  Class Election 2025|President;Secretary;Treasurer
    vector<string> loadPositions(const string& eventName) {
        ifstream inFile(EVENTS_FILE);
        string line;
        while (getline(inFile, line)) {
            line = trim(line);
            size_t bar = line.find('|');
            if (bar == string::npos) continue;
            if (trim(line.substr(0, bar)) != eventName) continue;
            // Found our event — split positions by ';'
            auto parts = split(line.substr(bar + 1), ';');
            for (auto& p : parts) p = trim(p);
            return parts;
        }
        return {};   // empty vector if event not found
    }

    // Saves (or updates) an event's name and positions to events.txt
    void saveEvent(const string& name, const vector<string>& positions) {
        // Build the positions as a semicolon-separated string
        string joined;
        for (size_t i = 0; i < positions.size(); ++i) {
            if (i) joined += ";";
            joined += positions[i];
        }

        // Read existing lines, replace this event if already there
        bool found = false;
        vector<string> lines;
        {
            ifstream inFile(EVENTS_FILE);
            string line;
            while (getline(inFile, line)) {
                string t = trim(line);
                size_t bar = t.find('|');
                string ev = (bar == string::npos) ? t : trim(t.substr(0, bar));
                if (ev == name) {
                    found = true;
                    lines.push_back(name + "|" + joined);   // replace
                } else {
                    lines.push_back(t);                     // keep as-is
                }
            }
        }
        if (!found) lines.push_back(name + "|" + joined);   // add new

        // Write all lines back
        ofstream outFile(EVENTS_FILE);
        for (auto& l : lines) outFile << l << "\n";
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Private: candidate helpers
    // ─────────────────────────────────────────────────────────────────────────

    // Returns true if a candidate with `roll` is already registered for ANY
    // position in the given event — a student may only stand for one position
    // per election, regardless of how many positions the event has.
    bool candidateExists(const string& roll, const string& ev, const string& pos) {
        ifstream inFile(CANDIDATE_FILE);
        string line;
        while (getline(inFile, line)) {
            auto p = split(trim(line), ',');
            if (p.size() >= 4 &&
                trim(p[0]) == roll &&
                trim(p[2]) == ev)          // position check intentionally removed
                return true;
        }
        return false;
    }

    // Returns which position the candidate is already registered for in this event.
    // Returns "" if not found.
    string candidatePosition(const string& roll, const string& ev) {
        ifstream inFile(CANDIDATE_FILE);
        string line;
        while (getline(inFile, line)) {
            auto p = split(trim(line), ',');
            if (p.size() >= 4 &&
                trim(p[0]) == roll &&
                trim(p[2]) == ev)
                return trim(p[3]);
        }
        return "";
    }

    // Loads all candidates for a specific event AND position
    vector<Candidate> loadCandidatesFor(const string& ev, const string& pos) {
        vector<Candidate> list;
        ifstream inFile(CANDIDATE_FILE);
        string line;
        while (getline(inFile, line)) {
            auto p = split(trim(line), ',');
            // candidates.txt format:  roll,name,event,position
            if (p.size() >= 4 && trim(p[2]) == ev && trim(p[3]) == pos)
                list.emplace_back(trim(p[0]), trim(p[1]), trim(p[2]), trim(p[3]));
        }
        return list;
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Private: deadline helpers
    // ─────────────────────────────────────────────────────────────────────────

    // Returns the voting deadline timestamp for an event (0 if not set)
    time_t loadDeadline(const string& ev) {
        ifstream inFile(DEADLINE_FILE);
        string line;
        while (getline(inFile, line)) {
            line = trim(line);
            auto p = split(line, '|');
            if (p.size() < 2 || trim(p[0]) != ev) continue;
            try {
                return (time_t)stoll(trim(p[1]));
            } catch (...) {}   // if stoll throws, just continue
        }
        return 0;   // deadline not found
    }

    // Saves (or updates) a deadline for an event
    void saveDeadline(const string& ev, time_t t) {
        bool found = false;
        vector<string> lines;
        {
            ifstream inFile(DEADLINE_FILE);
            string line;
            while (getline(inFile, line)) {
                string copy = trim(line);
                auto p = split(copy, '|');
                if (p.size() >= 1 && trim(p[0]) == ev) {
                    found = true;
                    lines.push_back(ev + "|" + to_string((long long)t));
                } else {
                    lines.push_back(copy);
                }
            }
        }
        if (!found) lines.push_back(ev + "|" + to_string((long long)t));
        ofstream outFile(DEADLINE_FILE);
        for (auto& l : lines) outFile << l << "\n";
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Private: winner helpers
    // ─────────────────────────────────────────────────────────────────────────

    // Returns true if a winner record has already been saved for this event+position
    bool winnerAlreadySaved(const string& ev, const string& pos) {
        ifstream inFile(WINNERS_FILE);
        string line;
        while (getline(inFile, line)) {
            auto p = split(trim(line), '|');
            if (p.size() >= 2 && trim(p[0]) == ev && trim(p[1]) == pos)
                return true;
        }
        return false;
    }

    // Tallies votes for one event/position and appends the result to winners.txt
    // winners.txt format:  event|position|winnerName|winnerRoll|votes|timestamp
    void saveWinnerForPosition(const string& ev, const string& pos, time_t closedAt) {
        if (winnerAlreadySaved(ev, pos)) return;   // already saved — skip

        // Count votes: map from candidateRoll → vote count
        map<string, int> tally;
        {
            ifstream inFile(VOTE_FILE);
            string line;
            while (getline(inFile, line)) {
                auto p = split(trim(line), ',');
                // votes.txt format:  voterRoll,event,position,candidateRoll
                if (p.size() >= 4 &&
                    trim(p[1]) == ev &&
                    trim(p[2]) == pos)
                    tally[trim(p[3])]++;   // increment this candidate's count
            }
        }

        // Build a map from candidateRoll → candidateName (for the output line)
        map<string, string> rollToName;
        for (auto& c : loadCandidatesFor(ev, pos))
            rollToName[c.roll] = c.name;

        // Find the candidate with the most votes
        string winRoll, winName;
        int    winVotes = -1;
        bool   isTie    = false;

        for (auto& kv : tally) {
            if (kv.second > winVotes) {
                // New leader found
                winVotes = kv.second;
                winRoll  = kv.first;
                winName  = rollToName.count(kv.first)
                               ? rollToName[kv.first]
                               : "Unknown(" + kv.first + ")";
                isTie    = false;
            } else if (kv.second == winVotes) {
                isTie = true;   // two candidates tied at the top
            }
        }

        // Append result to winners.txt
        ofstream outFile(WINNERS_FILE, ios::app);
        if (winVotes <= 0) {
            // Nobody voted for this position
            outFile << ev << "|" << pos << "|NO VOTES|--|0|"
                    << (long long)closedAt << "\n";
        } else if (isTie) {
            outFile << ev << "|" << pos << "|TIE|--|"
                    << winVotes << "|" << (long long)closedAt << "\n";
        } else {
            // Clear winner
            outFile << ev << "|" << pos << "|" << winName << "|" << winRoll
                    << "|" << winVotes << "|" << (long long)closedAt << "\n";
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Private: hasVoted — check if a voter already voted in a position
    // ─────────────────────────────────────────────────────────────────────────
    bool hasVoted(const string& voterRoll, const string& ev, const string& pos) {
        ifstream inFile(VOTE_FILE);
        string line;
        while (getline(inFile, line)) {
            auto p = split(trim(line), ',');
            if (p.size() >= 3 &&
                trim(p[0]) == voterRoll &&
                trim(p[1]) == ev        &&
                trim(p[2]) == pos)
                return true;
        }
        return false;
    }

public:

    // ─────────────────────────────────────────────────────────────────────────
    //  Purge expired events
    //  Called every time the main menu is shown.
    //  1. Find events whose deadline has passed.
    //  2. Save winner records FIRST (so results are not lost).
    //  3. Remove the expired event from all data files.
    //  Returns the list of newly-expired event names so main() can print a banner.
    // ─────────────────────────────────────────────────────────────────────────
    vector<string> purgeExpiredEvents() {
        time_t now = time(nullptr);   // current time as a Unix timestamp
        vector<string> expired;       // events that have just expired

        // Step 1: find expired events
        {
            ifstream inFile(DEADLINE_FILE);
            string line;
            while (getline(inFile, line)) {
                line = trim(line);
                if (line.empty()) continue;
                auto p = split(line, '|');
                if (p.size() < 2) continue;

                long long ts = 0;
                try { ts = stoll(trim(p[1])); } catch (...) {}

                if (ts > 0 && now > (time_t)ts)
                    expired.push_back(trim(p[0]));
            }
        }

        if (expired.empty()) return {};   // nothing expired

        // A helper lambda: returns true if event name is in the expired list
        auto isExpired = [&](const string& ev) {
            return find(expired.begin(), expired.end(), ev) != expired.end();
        };

        // Step 2: read close timestamps for expired events
        map<string, time_t> closedAt;
        {
            ifstream inFile(DEADLINE_FILE);
            string line;
            while (getline(inFile, line)) {
                auto p = split(trim(line), '|');
                if (p.size() >= 2 && isExpired(trim(p[0]))) {
                    long long ts = 0;
                    try { ts = stoll(trim(p[1])); } catch (...) {}
                    closedAt[trim(p[0])] = (time_t)ts;
                }
            }
        }

        // Save winners for every expired event/position BEFORE deleting data
        for (auto& ev : expired)
            for (auto& pos : loadPositions(ev))
                saveWinnerForPosition(ev, pos, closedAt[ev]);

        // Step 3: delete expired events from active database files
        rewriteFile(EVENTS_FILE, [&](const string& l) {
            size_t bar = l.find('|');
            string ev = (bar == string::npos) ? l : trim(l.substr(0, bar));
            return !isExpired(ev);
        });
        rewriteFile(CANDIDATE_FILE, [&](const string& l) {
            auto p = split(l, ',');
            return !(p.size() >= 3 && isExpired(trim(p[2])));
        });
        rewriteFile(VOTE_FILE, [&](const string& l) {
            auto p = split(l, ',');
            return !(p.size() >= 2 && isExpired(trim(p[1])));
        });
        rewriteFile(DEADLINE_FILE, [&](const string& l) {
            auto p = split(l, '|');
            return !(p.size() >= 1 && isExpired(trim(p[0])));
        });

        // BUG FIXED: Removed the rewriteFile(WINNERS_FILE, ...) block that was instantly 
        // wiping the freshly stored election results from history.

        return expired;   // main() will print a winner banner for these
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Print winner banner
    //  Called after purgeExpiredEvents() returns a non-empty list.
    // ─────────────────────────────────────────────────────────────────────────
    void printWinnerBanner(const vector<string>& justExpired) {
        if (justExpired.empty()) return;

        refreshTerminalWidth();
        drawBorder();
        drawTitle("*** ELECTION RESULTS ANNOUNCEMENT ***");
        drawBorder();

        for (const string& ev : justExpired) {
            drawLine("EVENT: " + ev);
            drawDivider();

            ifstream inFile(WINNERS_FILE);
            string line;
            bool any = false;

            while (getline(inFile, line)) {
                auto p = split(trim(line), '|');
                if (p.size() < 5 || trim(p[0]) != ev) continue;

                string pos    = trim(p[1]);
                string winner = trim(p[2]);
                string roll   = trim(p[3]);
                string votes  = trim(p[4]);

                if (winner == "NO VOTES")
                    drawLine("  " + pos + ": (no votes cast)");
                else if (winner == "TIE")
                    drawLine("  " + pos + ": TIE  (" + votes + " votes each)");
                else
                    drawLine("  " + pos + " WINNER: " + winner +
                             "  [Roll: " + roll + "]  (" + votes + " votes)");
                any = true;
            }
            if (!any) drawLine("  (no results recorded)");
            drawDivider();
        }
        drawBorder();
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Admin: Create Event
    // ─────────────────────────────────────────────────────────────────────────
    void createEvent() {
        refreshTerminalWidth();
        drawBorder();
        drawTitle("CREATE EVENT");
        drawBorder();
        drawLine("");

        printPrompt("Event Name : ");
        string name;
        getline(cin, name);
        name = trim(name);

        if (name.empty())      { printError("Name cannot be empty!");  return; }
        if (eventExists(name)) { printError("Event already exists!");  return; }

        drawLine("");
        refreshTerminalWidth();
        printPrompt("Positions (comma-separated, e.g. President,Secretary) : ");
        string posLine;
        getline(cin, posLine);

        // Split by comma and remove empty entries
        vector<string> positions;
        for (auto& p : split(posLine, ',')) {
            string t = trim(p);
            if (!t.empty()) positions.push_back(t);
        }

        if (positions.empty()) { printError("At least one position required!"); return; }

        saveEvent(name, positions);
        printSuccess("Event \"" + name + "\" created with " +
                     to_string(positions.size()) + " position(s).");
        drawBorder();
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Admin: Add Position to existing Event
    // ─────────────────────────────────────────────────────────────────────────
    void addPositionToEvent() {
        refreshTerminalWidth();
        drawBorder();
        drawTitle("ADD POSITION TO EVENT");
        drawBorder();

        auto events = loadEvents();
        if (events.empty()) { printError("No events exist. Create one first."); return; }

        drawLine("Select Event:");
        int idx = pickFrom(events);
        if (idx < 0) return;
        string ev = events[idx];

        drawLine("");
        refreshTerminalWidth();
        printPrompt("New position name : ");
        string pos;
        getline(cin, pos);
        pos = trim(pos);

        if (pos.empty()) { printError("Position cannot be empty!"); return; }

        // Check if position already exists in this event
        auto positions = loadPositions(ev);
        for (auto& p : positions)
            if (p == pos) { printError("Position already exists!"); return; }

        positions.push_back(pos);
        saveEvent(ev, positions);
        printSuccess("Position \"" + pos + "\" added to \"" + ev + "\".");
        drawBorder();
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Admin: Add Candidate
    // ─────────────────────────────────────────────────────────────────────────
    void addCandidate() {
        refreshTerminalWidth();
        drawBorder();
        drawTitle("ADD CANDIDATE");
        drawBorder();

        auto events = loadEvents();
        if (events.empty()) { printError("No events. Create one first."); return; }

        drawLine("Select Event:");
        int eIdx = pickFrom(events);
        if (eIdx < 0) return;
        string ev = events[eIdx];

        auto positions = loadPositions(ev);
        if (positions.empty()) { printError("No positions for this event!"); return; }

        drawBorder();
        drawTitle("SELECT POSITION");
        drawBorder();
        int pIdx = pickFrom(positions);
        if (pIdx < 0) return;
        string pos = positions[pIdx];

        drawLine("");

        // Roll number must be exactly 6 digits
        string roll;
        while (true) {
            printPrompt("Candidate Roll No (6 digits) : ");
            cin >> roll;
            roll = trim(roll);
            if (isValidRoll(roll)) break;
            printError("Roll must be exactly 6 numeric digits (e.g. 810301).");
        }

        // The candidate must be a registered student
        if (!isRegisteredStudent(roll)) {
            printError("Roll " + roll + " is not in the student register!");
            drawBorder();
            return;
        }

        // Look up the official name from students.txt — no need to ask again
        string officialName = getStudentName(roll);

        // Show the admin the name that will be registered
        refreshTerminalWidth();
        drawLine("Student on record : " + officialName + "  [Roll: " + roll + "]");

        if (candidateExists(roll, ev, pos)) {
            string takenPos = candidatePosition(roll, ev);
            printError("This student is already registered for \"" + takenPos + "\" in this event.");
            drawLine("  A student may only stand for ONE position per election.");
            drawBorder();
            return;
        }

        // Append to candidates.txt using the official name directly
        ofstream outFile(CANDIDATE_FILE, ios::app);
        outFile << roll << "," << officialName << "," << ev << "," << pos << "\n";

        printSuccess("Candidate \"" + officialName + "\" added to " + pos + " / " + ev + ".");
        drawBorder();
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Admin: Set Voting Deadline
    // ─────────────────────────────────────────────────────────────────────────
    void setDeadline() {
        refreshTerminalWidth();
        drawBorder();
        drawTitle("SET VOTING DEADLINE");
        drawBorder();

        auto events = loadEvents();
        if (events.empty()) { printError("No events exist."); return; }

        drawLine("Select Event:");
        int idx = pickFrom(events);
        if (idx < 0) return;
        string ev = events[idx];

        drawLine("");
        refreshTerminalWidth();
        printPrompt("Deadline  YYYY MM DD HH MM : ");
        int y, mo, d, h, mi;
        cin >> y >> mo >> d >> h >> mi;

        // Build a tm struct and convert to a Unix timestamp
        struct tm t = {};
        t.tm_year = y - 1900;   // tm_year counts from 1900
        t.tm_mon  = mo - 1;     // tm_mon is 0-based (0 = January)
        t.tm_mday = d;
        t.tm_hour = h;
        t.tm_min  = mi;

        time_t deadline = mktime(&t);
        if (deadline == -1) { printError("Invalid date/time!"); return; }

        saveDeadline(ev, deadline);

        // Format the deadline nicely for the success message
        char buf[64];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &t);
        printSuccess("Deadline for \"" + ev + "\" set to " + string(buf) + ".");
        drawBorder();
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Student: Vote
    // ─────────────────────────────────────────────────────────────────────────
    void vote() {
        auto events = loadEvents();
        if (events.empty()) {
            refreshTerminalWidth();
            printError("No events available for voting.");
            return;
        }

        refreshTerminalWidth();
        drawBorder();
        drawTitle("VOTE - SELECT EVENT");
        drawBorder();

        int eIdx = pickFrom(events);
        if (eIdx < 0) return;
        string ev = events[eIdx];

        // Check that a deadline is set and voting is still open
        time_t deadline = loadDeadline(ev);
        if (deadline == 0) {
            printError("No deadline set for this event. Contact admin.");
            return;
        }
        if (time(nullptr) > deadline) {
            printError("Voting is closed - deadline has passed.");
            return;
        }

        refreshTerminalWidth();
        drawBorder();
        drawTitle("VOTER IDENTIFICATION");
        drawBorder();
        drawLine("");

        // Ask for the voter's roll number
        string voterRoll;
        while (true) {
            printPrompt("Your Roll No (6 digits) : ");
            getline(cin, voterRoll);
            voterRoll = trim(voterRoll);
            if (isValidRoll(voterRoll)) break;
            printError("Roll must be exactly 6 numeric digits (e.g. 810301).");
        }

        // Only registered students can vote
        if (!isRegisteredStudent(voterRoll)) {
            printError("Roll " + voterRoll + " is not in the student register!");
            drawBorder();
            return;
        }

        // Vote for each position in this event
        auto positions = loadPositions(ev);
        for (auto& pos : positions) {
            // Skip if student already voted for this position
            if (hasVoted(voterRoll, ev, pos)) {
                printWarning("Already voted for " + pos + ". Skipping.");
                continue;
            }

            // Skip if no candidates registered for this position
            auto list = loadCandidatesFor(ev, pos);
            if (list.empty()) {
                printWarning("No candidates for " + pos + ". Skipping.");
                continue;
            }

            refreshTerminalWidth();
            drawBorder();
            drawTitle("VOTE FOR: " + pos);
            drawBorder();

            bool isCandidate = false;
            for (auto& c : list) {
                if (c.roll == voterRoll) {
                    isCandidate = true;
                    break;
                }
            }

            if (isCandidate) {
                printWarning("You are a candidate for " + pos + ". Cannot vote!");
                continue;
            }

            // Show candidate list
            vector<string> labels;
            for (auto& c : list)
                labels.push_back(c.name + "  [Roll: " + c.roll + "]");

            int cIdx = pickFrom(labels);
            if (cIdx < 0) continue;

            // Record the vote: voterRoll,event,position,candidateRoll
            ofstream outFile(VOTE_FILE, ios::app);
            outFile << voterRoll << "," << ev << "," << pos << ","
                    << list[cIdx].roll << "\n";

            printSuccess("Vote recorded for " + pos + "!");
        }
        drawBorder();
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Public: View Results
    //  Shows final results for closed elections and live tallies for open ones.
    // ─────────────────────────────────────────────────────────────────────────
    void results() {
        refreshTerminalWidth();
        drawBorder();
        drawTitle("ELECTION RESULTS");
        drawBorder();

        // --- Save winners for any expired-but-not-yet-purged events ---
        auto events = loadEvents();
        for (const string& ev : events) {
            time_t deadline = loadDeadline(ev);
            if (deadline != 0 && time(nullptr) > deadline) {
                for (const string& pos : loadPositions(ev)) {
                    if (!winnerAlreadySaved(ev, pos)) {
                        saveWinnerForPosition(ev, pos, deadline);
                    }
                }
            }
        }

        bool anything = false;

        // ── Section 1: Closed elections — read from winners.txt ──────────────
        {
            map<string, vector<string>> closedResults;  // event → display lines
            {
                ifstream inFile(WINNERS_FILE);
                string line;
                while (getline(inFile, line)) {
                    auto p = split(trim(line), '|');
                    if (p.size() < 5) continue;
                    string ev     = trim(p[0]);
                    string pos    = trim(p[1]);
                    string winner = trim(p[2]);
                    string roll   = trim(p[3]);
                    string votes  = trim(p[4]);

                    string entry;
                    if (winner == "NO VOTES")
                        entry = "  " + pos + ": (no votes cast)";
                    else if (winner == "TIE")
                        entry = "  " + pos + ": TIE  (" + votes + " votes each)";
                    else
                        entry = "  " + pos + " WINNER: " + winner +
                                "  [Roll: " + roll + "]  (" + votes + " votes)";

                    closedResults[ev].push_back(entry);
                }
            }

            if (!closedResults.empty()) {
                drawLine("[ CLOSED ELECTIONS - FINAL RESULTS ]");
                drawDivider();
                for (auto& kv : closedResults) {
                    drawLine("EVENT: " + kv.first);
                    for (auto& l : kv.second) drawLine(l);
                    drawDivider();
                }
                anything = true;
            }
        }

        // ── Section 2: Open elections — live tally from votes.txt ────────────
        events = loadEvents();
        if (!events.empty()) {
            // tally[event][position][candidateRoll] = vote count
            map<string, map<string, map<string, int>>> tally;
            {
                ifstream inFile(VOTE_FILE);
                string line;
                while (getline(inFile, line)) {
                    auto p = split(trim(line), ',');
                    // votes.txt:  voterRoll, event, position, candidateRoll
                    if (p.size() >= 4)
                        tally[trim(p[1])][trim(p[2])][trim(p[3])]++;
                }
            }

            drawLine("[ OPEN ELECTIONS - LIVE TALLY ]");
            drawDivider();

            for (auto& ev : events) {
                time_t dl     = loadDeadline(ev);
                bool   closed = (dl > 0 && time(nullptr) > dl);

                drawLine("EVENT: " + ev + (closed ? "  [VOTING CLOSED]" : ""));

                for (auto& pos : loadPositions(ev)) {
                    drawLine("  Position: " + pos);

                    if (tally[ev][pos].empty()) {
                        drawLine("    (no votes yet)");
                    } else {
                        // Find highest vote count (to mark the leader)
                        int topVotes = 0;
                        for (auto& kv : tally[ev][pos])
                            topVotes = max(topVotes, kv.second);

                        // Build roll → name lookup for display
                        map<string, string> rollName;
                        for (auto& c : loadCandidatesFor(ev, pos))
                            rollName[c.roll] = c.name;

                        for (auto& kv : tally[ev][pos]) {
                            string nm   = rollName.count(kv.first)
                                              ? rollName[kv.first]
                                              : "Unknown(" + kv.first + ")";
                            string lead = (kv.second == topVotes) ? " <-- LEADING" : "";
                            drawLine("    " + nm + " : " +
                                     to_string(kv.second) + " vote(s)" + lead);
                        }
                    }
                }
                drawDivider();
            }
            anything = true;
        }

        if (!anything) drawLine("No results available yet.");
        drawBorder();
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Public: View Candidates (anyone can use this)
    // ─────────────────────────────────────────────────────────────────────────
    void viewCandidates() {
        auto events = loadEvents();
        if (events.empty()) {
            refreshTerminalWidth();
            printError("No events available.");
            return;
        }

        refreshTerminalWidth();
        drawBorder();
        drawTitle("VIEW CANDIDATES - SELECT EVENT");
        drawBorder();

        int eIdx = pickFrom(events);
        if (eIdx < 0) return;
        string ev = events[eIdx];

        auto positions = loadPositions(ev);
        if (positions.empty()) {
            printError("No positions found for this event.");
            return;
        }

        refreshTerminalWidth();
        drawBorder();
        drawTitle("CANDIDATES: " + ev);
        drawBorder();

        bool any = false;
        for (auto& pos : positions) {
            drawLine("Position: " + pos);
            auto list = loadCandidatesFor(ev, pos);
            if (list.empty()) {
                drawLine("  (no candidates registered)");
            } else {
                for (size_t i = 0; i < list.size(); ++i)
                    drawLine("  " + to_string(i + 1) + ". " +
                             list[i].name + "  [Roll: " + list[i].roll + "]");
                any = true;
            }
            drawDivider();
        }
        if (!any) printWarning("No candidates registered for any position yet.");
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Admin: Delete Event
    // ─────────────────────────────────────────────────────────────────────────
    void deleteEvent() {
        refreshTerminalWidth();
        drawBorder();
        drawTitle("DELETE EVENT");
        drawBorder();

        auto events = loadEvents();
        if (events.empty()) { printError("No events exist."); return; }

        drawLine("Select Event to Delete:");
        int idx = pickFrom(events);
        if (idx < 0) return;
        string ev = events[idx];

        drawLine("");
        printPrompt("Delete \"" + ev + "\" and ALL its data? (y/n) : ");
        char confirm;
        cin >> confirm;
        if (confirm != 'y' && confirm != 'Y') {
            printWarning("Deletion cancelled.");
            return;
        }

        // Remove this event from every data file
        rewriteFile(EVENTS_FILE, [&](const string& l) {
            size_t bar = l.find('|');
            return (bar == string::npos ? l : trim(l.substr(0, bar))) != ev;
        });
        rewriteFile(CANDIDATE_FILE, [&](const string& l) {
            auto p = split(l, ',');
            return !(p.size() >= 3 && trim(p[2]) == ev);
        });
        rewriteFile(VOTE_FILE, [&](const string& l) {
            auto p = split(l, ',');
            return !(p.size() >= 2 && trim(p[1]) == ev);
        });
        rewriteFile(DEADLINE_FILE, [&](const string& l) {
            auto p = split(l, '|');
            return !(p.size() >= 1 && trim(p[0]) == ev);
        });
        rewriteFile(WINNERS_FILE, [&](const string& l) {
            auto p = split(l, '|');
            return !(p.size() >= 1 && trim(p[0]) == ev);
        });

        printSuccess("Event \"" + ev + "\" and all related data deleted.");
        drawBorder();
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Admin: Delete Candidate
    // ─────────────────────────────────────────────────────────────────────────
    void deleteCandidate() {
        refreshTerminalWidth();
        drawBorder();
        drawTitle("DELETE CANDIDATE");
        drawBorder();

        auto events = loadEvents();
        if (events.empty()) { printError("No events exist."); return; }

        drawLine("Select Event:");
        int eIdx = pickFrom(events);
        if (eIdx < 0) return;
        string ev = events[eIdx];

        auto positions = loadPositions(ev);
        if (positions.empty()) { printError("No positions for this event."); return; }

        refreshTerminalWidth();
        drawBorder();
        drawTitle("SELECT POSITION");
        drawBorder();
        int pIdx = pickFrom(positions);
        if (pIdx < 0) return;
        string pos = positions[pIdx];

        auto list = loadCandidatesFor(ev, pos);
        if (list.empty()) { printError("No candidates for this position."); return; }

        refreshTerminalWidth();
        drawBorder();
        drawTitle("SELECT CANDIDATE TO DELETE");
        drawBorder();

        vector<string> labels;
        for (auto& c : list)
            labels.push_back(c.name + "  [Roll: " + c.roll + "]");

        int cIdx = pickFrom(labels);
        if (cIdx < 0) return;
        Candidate target = list[cIdx];

        drawLine("");
        printPrompt("Delete \"" + target.name + "\" (" + target.roll +
                    ") from " + pos + "? (y/n) : ");
        char confirm;
        cin >> confirm;
        if (confirm != 'y' && confirm != 'Y') {
            printWarning("Deletion cancelled.");
            return;
        }

        // Remove candidate from candidates.txt and their votes from votes.txt
        rewriteFile(CANDIDATE_FILE, [&](const string& l) {
            auto p = split(l, ',');
            return !(p.size() >= 4 &&
                     trim(p[0]) == target.roll &&
                     trim(p[2]) == ev &&
                     trim(p[3]) == pos);
        });
        rewriteFile(VOTE_FILE, [&](const string& l) {
            auto p = split(l, ',');
            return !(p.size() >= 4 &&
                     trim(p[1]) == ev &&
                     trim(p[2]) == pos &&
                     trim(p[3]) == target.roll);
        });

        printSuccess("Candidate \"" + target.name + "\" deleted successfully.");
        drawBorder();
    }

    // ─────────────────────────────────────────────────────────────────────────
    //  Admin Panel menu loop
    // ─────────────────────────────────────────────────────────────────────────
    void adminPanel(Admin& admin) {
        while (true) {
            refreshTerminalWidth();   // snapshot width at the start of every menu draw
            drawBorder();
            drawTitle("ADMIN PANEL");
            drawBorder();
            drawLine("  1. Create Event");
            drawLine("  2. Add Position to Event");
            drawLine("  3. Add Candidate");
            drawLine("  4. Set Voting Deadline");
            drawLine("  5. Delete Event");
            drawLine("  6. Delete Candidate");
            drawLine("  7. View All Students");
            drawLine("  8. Add Admin Account");
            drawLine("  9. Change Admin Credentials");
            drawLine(" 10. Logout");
            drawDivider();
            printPrompt("Choice : ");

            int c;
            // Clear any failbit left by a previous bad read (e.g. after
            // getHiddenInput switches the terminal to raw mode and back).
            // Without this, cin >> c would instantly return 0 every iteration
            // causing the infinite loop.
            if (cin.fail()) { cin.clear(); cin.ignore(1000, '\n'); }
            cin >> c;
            cin.ignore(1000, '\n');  // flush rest of line (prevents stale newlines)
            cout << "\n";

            switch (c) {
                case  1: createEvent();        break;
                case  2: addPositionToEvent(); break;
                case  3: addCandidate();       break;
                case  4: setDeadline();        break;
                case  5: deleteEvent();        break;
                case  6: deleteCandidate();    break;
                case  7: viewAllStudents();    break;
                case  8: admin.addAdmin();     break;
                case  9: admin.changeAdmin();  break;
                case 10: return;               // logout → return to main menu
                default: printError("Invalid choice. Enter 1-10.");
            }
        }
    }
};

#endif // ELECTION_H