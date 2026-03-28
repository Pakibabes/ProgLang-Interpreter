#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <vector>
#include <string>

using namespace std;

class Interpreter {

private:

    vector<string> lines;

public:

    Interpreter(vector<string> programLines);

    void run();

    void removeComments();

    void validateStructure();

};

#endif