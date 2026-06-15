// ─────────────────────────────────────────────────────────────────────────────
//  student.h  –  Student record management
//
//  This header handles:
//    • Creating the students.txt file with 30 pre-loaded Nepali students
//    • Looking up whether a roll number belongs to a registered student
//    • Printing the student list to the screen
//
//  students.txt format (one student per line):
//    rollNo,fullName
//  Example:
//    810301,Aarav Sharma
// ─────────────────────────────────────────────────────────────────────────────

#ifndef STUDENT_H
#define STUDENT_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "display.h"
#include "fileutils.h"

using namespace std;

// The name of the file that stores student records
const string STUDENT_FILE = "students.txt";

// ─────────────────────────────────────────────────────────────────────────────
//  Student data structure
// ─────────────────────────────────────────────────────────────────────────────

// A simple struct to hold one student's information.
// A struct is like a container that groups related data together.
struct Student {
    string roll;  // 6-digit roll number, e.g. "810301"
    string name;  // Full name,           e.g. "Aarav Sharma"
};

// ─────────────────────────────────────────────────────────────────────────────
//  Create students.txt with 30 Nepali students (runs only on first launch)
// ─────────────────────────────────────────────────────────────────────────────

// This function checks if students.txt already exists.
// If it does NOT exist (or is empty), it creates it with 30 students.
// Roll numbers go from 810301 to 810330.
void ensureStudentFile() {
    // Try to open the file for reading
    ifstream checkFile(STUDENT_FILE);
    if (checkFile.is_open() && checkFile.peek() != EOF) {
        return;   // File exists and has content — nothing to do
    }
    checkFile.close();

    // File does not exist or is empty — create it now
    ofstream outFile(STUDENT_FILE);

    // 30 Nepali student names (common Nepali first + last names)
    // Roll numbers: 810301 to 810330
    outFile << "810301,Aarav Sharma\n";
    outFile << "810302,Anisha Thapa\n";
    outFile << "810303,Bibek Adhikari\n";
    outFile << "810304,Bimala Rai\n";
    outFile << "810305,Dipesh Karki\n";
    outFile << "810306,Elina Basnet\n";
    outFile << "810307,Gaurav Pandey\n";
    outFile << "810308,Gita Gurung\n";
    outFile << "810309,Hari Prasad Oli\n";
    outFile << "810310,Ishwori Shrestha\n";
    outFile << "810311,Jeevan Tamang\n";
    outFile << "810312,Kabita Magar\n";
    outFile << "810313,Kushal Bhandari\n";
    outFile << "810314,Laxmi Pokhrel\n";
    outFile << "810315,Manish Subedi\n";
    outFile << "810316,Nisha Khadka\n";
    outFile << "810317,Prakash Dahal\n";
    outFile << "810318,Priya Limbu\n";
    outFile << "810319,Rajesh Ghimire\n";
    outFile << "810320,Rita Dhakal\n";
    outFile << "810321,Roshan Acharya\n";
    outFile << "810322,Sabina Koirala\n";
    outFile << "810323,Sagar Poudel\n";
    outFile << "810324,Samjhana Chaudhary\n";
    outFile << "810325,Sandip Bhattarai\n";
    outFile << "810326,Shreya Humagain\n";
    outFile << "810327,Sujan Nepali\n";
    outFile << "810328,Sunita Yadav\n";
    outFile << "810329,Ujjwal Panta\n";
    outFile << "810330,Yogesh Pulami\n";

    // outFile closes automatically here (RAII)
}

// ─────────────────────────────────────────────────────────────────────────────
//  Load all students from students.txt into a vector
// ─────────────────────────────────────────────────────────────────────────────

// Reads students.txt and returns a list of Student structs.
// Returns an empty list if the file cannot be opened.
vector<Student> loadAllStudents() {
    vector<Student> students;

    ifstream inFile(STUDENT_FILE);
    if (!inFile.is_open()) return students;   // file missing → return empty list

    string line;
    while (getline(inFile, line)) {
        line = trim(line);
        if (line.empty()) continue;

        // Each line looks like:  810301,Aarav Sharma
        // split by comma gives:  ["810301", "Aarav Sharma"]
        vector<string> parts = split(line, ',');
        if (parts.size() >= 2) {
            Student s;
            s.roll = trim(parts[0]);
            s.name = trim(parts[1]);
            students.push_back(s);
        }
    }
    return students;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Check if a roll number belongs to a registered student
// ─────────────────────────────────────────────────────────────────────────────

// Returns true if the given roll number is found in students.txt.
// Used before allowing someone to vote — only registered students can vote.
bool isRegisteredStudent(const string& roll) {
    vector<Student> students = loadAllStudents();
    for (const Student& s : students) {
        if (s.roll == roll)
            return true;   // found!
    }
    return false;           // not found
}

// Returns the student's name for a given roll number.
// Returns "(Unknown)" if the roll is not found.
string getStudentName(const string& roll) {
    vector<Student> students = loadAllStudents();
    for (const Student& s : students) {
        if (s.roll == roll)
            return s.name;
    }
    return "(Unknown)";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Print all students (Admin feature)
// ─────────────────────────────────────────────────────────────────────────────

// Prints the complete student list inside a nice box.
// Useful for the admin to check who is registered.
void viewAllStudents() {
    refreshTerminalWidth();   // snapshot terminal width for this screen
    drawBorder();
    drawTitle("REGISTERED STUDENTS");
    drawBorder();

    vector<Student> students = loadAllStudents();

    if (students.empty()) {
        printError("No students found in " + STUDENT_FILE);
        drawBorder();
        return;
    }

    // Print table header
    drawLine("  Roll No    Name");
    drawDivider();

    // Print each student on one row
    for (size_t i = 0; i < students.size(); ++i) {
        // Build a nicely padded row string:  "  810301    Aarav Sharma"
        string row = "  " + students[i].roll + "    " + students[i].name;
        drawLine(row);
    }

    drawDivider();
    drawLine("  Total students: " + to_string(students.size()));
    drawBorder();
}

#endif // STUDENT_H
