// executes statements
#include "interpreter.h"
#include <iostream>
#include <algorithm>

using namespace std;

// remove spacing
string trim (string str){
    int start = str.find_first_not_of(" \t");
    int end = str.find_last_not_of(" \t");

    if (start == -1) return "";

    return str.substr(start, end-start + 1);
}

Interpreter::Interpreter(vector<string> programLines) {

    lines = programLines;

}

void Interpreter::run() {
    removeComments();
    validateStructure();

    cout << "LEXOR program structure is valid." << endl;
}

void Interpreter::removeComments() {

    vector<string> cleaned;

    for (string line : lines) {
        line = trim(line);

        if (line.empty()) continue;

        if (line.find("%%") == 0) {
            continue;
        }

        cleaned.push_back(line);
    }

    lines = cleaned;
}

void Interpreter::validateStructure() {

    if (lines.size() < 3) {
        cout << "Invalid program." << endl;
        exit(1);
    }

    if (lines[0] != "SCRIPT AREA") {
        cout << "Error: Missing SCRIPT AREA." << endl;
        exit(1);
    }

    if (lines[1] != "START SCRIPT") {
        cout << "Error: Missing START SCRIPT." << endl;
        exit(1);
    }

    if (lines.back() != "END SCRIPT") {
        cout << "Error: Missing END SCRIPT." << endl;
        exit(1);
    }
}