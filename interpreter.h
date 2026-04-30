#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <vector>
#include <string>
#include <unordered_map>

using namespace std;

class Interpreter {

private:
    // stores values
    unordered_map<string, string> symbolTable;
    //stores data types
    unordered_map<string, string> typeTable;
    vector<string> lines;

public:

    Interpreter(vector<string> programLines);

    void run();

    void removeComments();
    void validateStructure();
    void processDeclarations();
    void processAssignments();
    void processPrint();
    void validateDeclarationOrder();

    string replaceVariables(string expr);
};


#endif