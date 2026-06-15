STUDENT COUNCIL VOTING SYSTEM
=============================

COMPILATION:
============
g++ -o voting_system voting_system.cpp -std=c++11

RUN:
====
./voting_system

DEFAULT ADMIN LOGIN:
====================
Username: rushan
Password: 12345

IMPORTANT NOTES:
================

1. BOX_WIDTH is now 85 (increased from 62)
   - This prevents message truncation
   - Full success messages are now displayed completely

2. refreshTerminalWidth() is called before every prompt that follows user input
   - This fixes the console shifting left problem
   - The box stays perfectly centered from first character

3. students.txt contains 30 Nepali students with 6-digit roll numbers (810301-810330)
   - Created automatically on first run
   - Only registered students can vote

FILES INCLUDED:
===============
- display.h       : Box drawing and centering functions (BOX_WIDTH = 85)
- fileutils.h     : String utilities and file operations
- student.h       : Student record management
- admin.h         : Admin login and credentials
- election.h      : All election and voting logic
- voting_system.cpp : Main program
- students.txt    : 30 pre-loaded student records

FIXES APPLIED:
==============
✓ Message truncation fixed (BOX_WIDTH = 85)
✓ Console shift left fixed (refreshTerminalWidth() before prompts)
✓ All 30 Nepali students with correct roll numbers
✓ Proper file separation for easy code management
✓ Compiles cleanly without warnings
