// ─────────────────────────────────────────────────────────────────────────────
//  voting_system.cpp  –  Student Council Voting System
//
//  HOW TO COMPILE:
//    g++ -o voting_system voting_system.cpp -std=c++11
//
//  HOW TO RUN:
//    ./voting_system
//
//  DEFAULT ADMIN LOGIN:
//    Username : rushan
//    Password : 12345
//
//  FILES CREATED AT RUNTIME:
//    admin.txt      — admin username + hashed password
//    students.txt   — 30 Nepali students (roll 810301–810330)
//    events.txt     — election events and their positions
//    candidates.txt — registered candidates
//    votes.txt      — cast votes
//    deadlines.txt  — voting deadlines
//    winners.txt    — final results for closed elections
//
//  HOW THE CODE IS ORGANISED:
//    display.h   — box-drawing and print helpers (THE CENTERING FIX is here)
//    fileutils.h — string/file utility functions
//    student.h   — student record management + students.txt creation
//    admin.h     — admin login and credential management
//    election.h  — all election logic (events, candidates, voting, results)
//    voting_system.cpp — just main(); ties everything together
//
//  FIX EXPLAINED (centering shift):
//    Before the fix every draw function called terminalWidth() on its own.
//    If the OS returned slightly different values between calls the left margin
//    changed per line → the box shifted horizontally row by row.
//    Now: refreshTerminalWidth() is called ONCE at the top of every menu draw,
//    storing the width in g_termWidth. All draw functions read that ONE cached
//    value → every row has the same margin → perfect, stable centering.
// ─────────────────────────────────────────────────────────────────────────────

// ── Include our own header files ──────────────────────────────────────────────
// The order matters: each file uses things defined by the previous ones.
#include "display.h"    // must come first (g_termWidth, centreOffset, etc.)
#include "fileutils.h"  // trim, split, hashPassword, rewriteFile
#include "student.h"    // Student struct, ensureStudentFile, isRegisteredStudent
#include "admin.h"      // Admin class
#include "election.h"   // Candidate struct, ElectionSystem class

// ── Standard library includes ─────────────────────────────────────────────────
#include <iostream>     // cout, cin
using namespace std;

// ═════════════════════════════════════════════════════════════════════════════
//  main()  –  Program entry point
//  This function is the first thing that runs when the program starts.
//  It sets everything up and then runs the main menu loop.
// ═════════════════════════════════════════════════════════════════════════════
int main() {

    // ── Create our two main objects ───────────────────────────────────────────
    // "Admin admin" creates an Admin object called "admin".
    // "ElectionSystem es" creates an ElectionSystem object called "es".
    Admin          admin;
    ElectionSystem es;

    // ── First-run setup ───────────────────────────────────────────────────────
    // If admin.txt is missing → create default account (rushan / 12345)
    admin.ensureDefaultAdmin();

    // If students.txt is missing → create it with 30 Nepali students
    ensureStudentFile();

    // ── Check for elections that expired before the program started ───────────
    // purgeExpiredEvents() returns a list of just-closed election names.
    // We pass that list to printWinnerBanner() to announce results on startup.
    auto justExpired = es.purgeExpiredEvents();
    es.printWinnerBanner(justExpired);

    // ── Main menu loop ────────────────────────────────────────────────────────
    // "while (true)" runs forever until the user chooses Exit (case 5).
    while (true) {

        // Check again for newly-expired elections every time the menu appears
        justExpired = es.purgeExpiredEvents();
        es.printWinnerBanner(justExpired);

        // Print a blank line before the box for visual breathing room
        cout << "\n";

        // ── Snapshot terminal width ONCE for this menu draw ───────────────────
        // This is THE FIX for the centering bug.
        // All subsequent draw calls in this loop use the cached g_termWidth.
        refreshTerminalWidth();

        // ── Draw the main menu ────────────────────────────────────────────────
        drawBorder();
        drawTitle("STUDENT COUNCIL VOTING SYSTEM");
        drawBorder();
        drawLine("  1. Admin Login");
        drawLine("  2. Vote");
        drawLine("  3. View Results");
        drawLine("  4. View Candidates");
        drawLine("  5. Exit");
        drawDivider();
        printPrompt("Choice : ");

        // Read the user's choice
        int choice;
        if (cin.fail()) { cin.clear(); cin.ignore(1000, '\n'); }
        cin >> choice;
        cin.ignore(1000, '\n');  // flush rest of line
        cout << "\n";

        // ── Handle the user's choice ──────────────────────────────────────────
        switch (choice) {

            case 1:
                // Try to log in as admin.
                // login() returns true on success → open the admin panel.
                if (admin.login())
                    es.adminPanel(admin);
                break;

            case 2:
                // Let a student vote in an open election
                es.vote();
                break;

            case 3:
                // Show election results (closed = final winners, open = live tally)
                es.results();
                break;

            case 4:
                // Show all registered candidates for a chosen event
                es.viewCandidates();
                break;

            case 5:
                // Exit the program cleanly
                refreshTerminalWidth();
                drawBorder();
                drawTitle("Thank you for using the Voting System!");
                drawTitle("Goodbye.");
                drawBorder();
                cout << "\n";
                return 0;   // returning 0 from main() ends the program

            default:
                // The user typed something not in the menu
                printError("Invalid choice. Please enter a number from 1 to 5.");
                break;
        }
    }

    // We never reach here because the loop only ends via "return 0" above,
    // but it is good practice to have a return at the end of main().
    return 0;
}