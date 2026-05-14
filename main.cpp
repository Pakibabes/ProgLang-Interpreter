#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include "interpreter.h"

using namespace std;

vector<string> readFile(const string& fName) {
    vector<string> lines;
    ifstream file(fName);

    if (!file.is_open()) {
        cerr << "Error: cannot open file '" << fName << "'" << endl;
        exit(1);
    }

    string line;
    while (getline(file, line))
        lines.push_back(line);

    return lines;
}

int main(int argc, char* argv[]) {
    string filename = "program.lexor";   // default
    if (argc >= 2) filename = argv[1];

    vector<string> program = readFile(filename);
    Interpreter lexor(program);
    lexor.run();
    return 0;
}

// Running the program
// g++ is the compiler main  and interpreter needs to be compiled
// g++ main.cpp interpreter.cpp -o lexor
// ./lexor