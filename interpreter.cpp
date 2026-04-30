// executes statements
#include "interpreter.h"
#include <iostream>
#include <algorithm>
#include <locale> // for smart quotes
#include <stdexcept>

using namespace std;
//helper functions
// remove spacing
string trim (string str){
    int start = str.find_first_not_of(" \t");
    int end = str.find_last_not_of(" \t");

    if (start == -1) return "";

    return str.substr(start, end-start + 1);
}

bool startsWith(const string& str, const string& prefix) {
    return str.rfind(prefix, 0) == 0;
}

bool endsWith(const string& str, const string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool isExpression(string value) {
    return value.find('+') != string::npos ||
           value.find('-') != string::npos ||
           value.find('*') != string::npos ||
           value.find('/') != string::npos ||
           value.find('(') != string::npos;
}

string normalizeQuotes(string value) {
    value = trim(value);
    if (value.empty()) return value;

    if ((value.front() == '"' && value.back() == '"') ||
        (value.front() == '\'' && value.back() == '\'')) {
        return value.substr(1, value.size() - 2);
    }
    string openD = "”"; string closeD = "”";
    string openS = "’"; string closeS = "’";

    if (value.size() >= 6) { 
        if (startsWith(value, openD) && endsWith(value, closeD)) {
            return value.substr(3, value.size() - 6);
        }
        if (startsWith(value, openS) && endsWith(value, closeS)) {
            return value.substr(3, value.size() - 6);
        }
    }

    return value;
}

//errors
void error(const string& code, const string& message, int line = -1) {
    cout << "LEXOR Error [" << code << "]\n";
    if (line != -1) {
        cout << "Line " << line << ": ";
    }
    cout << message << endl;
    exit(1);
}
// main functnions
Interpreter::Interpreter(vector<string> programLines) {

    lines = programLines;

}

void Interpreter::run() {
    removeComments();
    validateStructure();

    
    cout << "LEXOR program structure is valid." << endl; // validates if naaay START SCRIPT, SCRIPT AREA, ug END SCRIPT

    processDeclarations();
    processAssignments();
    processPrint();
    validateDeclarationOrder();
}


void Interpreter::removeComments() {
    vector<string> cleaned;

    for (string line : lines) {
        int commentPos = line.find("%%");

        if (commentPos != string::npos) {
            line = line.substr(0, commentPos);
        }

        line = trim(line);

        if (!line.empty()) {
            cleaned.push_back(line);
        }
    }

    lines = cleaned;
}


void Interpreter::validateStructure() {

    if (lines.size() < 3) {
        error("LEXOR-000", "Invalid structure. Missing required structure.");
    }

    if (lines[0] != "SCRIPT AREA") {
        error("LEXOR-001",
              "Expected 'SCRIPT AREA' at the beginning of program, found: " + lines[0], 1);
    }

    if (lines[1] != "START SCRIPT") {
        error("LEXOR-002",
              "Expected 'START SCRIPT' after SCRIPT AREA, found: " + lines[1], 2);
    }

    if (lines.back() != "END SCRIPT") {
        error("LEXOR-003",
              "Expected 'END SCRIPT' at end of program, found: " + lines.back(),
              lines.size());
    }
}
void Interpreter::validateDeclarationOrder() {
    bool seenExecutable = false;
    for (int i = 2; i < lines.size() - 1; i++) {
        string line = trim(lines[i]);
        if (line.empty()) continue;
        if (line.find("DECLARE") == 0) {
            if (seenExecutable) {
                error("LEXOR-004",
                      "Variable declarations must appear before executable statements.",
                      i + 1);
            }
        }
        else {
            seenExecutable = true;
        }
    }
}
void Interpreter::processDeclarations(){
    for (int i = 2; i < lines.size(); i++){
        string line = lines[i];

        // when declaration ends
        if(line.find("DECLARE") != 0) break;

        string rest = line.substr(8); // removes the word declare

        int spacePos = rest.find(" ");
        string type = rest.substr(0, spacePos);
        string vars = rest.substr(spacePos + 1);

        string var = "";
        for (int j = 0; j <= vars.length(); j++) {
            if (j == vars.length() || vars[j] == ',') {
                int equalPos = var.find('=');
                if (equalPos != string::npos) {
                    string name = trim(var.substr(0, equalPos));
                    string value = trim(var.substr(equalPos + 1));
                    value = normalizeQuotes(value);
                    symbolTable[name] = value;
                    typeTable[name] = type;
                }
                else {
                    string name = trim(var);
                    if (type == "INT" || type == "FLOAT")
                        symbolTable[name] = "0";
                    else if (type == "BOOL"){
                        
                        symbolTable[name] = "FALSE";
                    }
                    else if (type == "CHAR")
                        symbolTable[name] = "";

                    typeTable[name] = type;
                }
                var = "";
            }
            else {
                var += vars[j];
            }
        }
    }
}

void Interpreter::processAssignments(){
    for (int i = 2; i < lines.size(); i++){
        string line = trim(lines[i]);

        if(line.find("DECLARE") == 0) continue;
        if(line.find("PRINT") == 0) continue;
        if(line.find("IF") == 0) continue;
        if(line.find("=") != string::npos){
            vector<string>parts;
            string temp ="";
            for (char c : line){
                if(c == '='){
                    parts.push_back(trim(temp));
                    temp = "";
                }else temp += c;
            }
            parts.push_back(trim(temp));
            string value = trim(parts.back());
            if(isExpression(value)){
                value = replaceVariables(value);
                cout << "After replace: " << value << endl;
            }
            value = normalizeQuotes(value);
            
            for (int j = parts.size() - 2; j >= 0; j--) {
                string var = trim(parts[j]);
                // if value is another variable
                if (symbolTable.find(value) != symbolTable.end()) {
                    value = symbolTable[value];
                }
                symbolTable[var] = value;
            }
        }
        
    }
}

void Interpreter::processPrint() {
    for (int lineIndex = 0; lineIndex < lines.size(); lineIndex++) {
        string line = trim(lines[lineIndex]);

        if (line.find("PRINT:") != 0) continue;

        string content = trim(line.substr(6));
        string part = "";

        for (int i = 0; i <= content.length(); i++) {
            if (i == content.length() || content[i] == '&') {

                string token = trim(part);

                // ✅ Rule 1: newline
                if (token == "$") {
                    cout << endl;
                }

                // ✅ Rule 2: escape [ ... ]
                else if (token.size() >= 2 &&
                         token.front() == '[' &&
                         token.back() == ']') {

                    cout << token.substr(1, token.size() - 2);
                }

                // ✅ Rule 3: variable
                else if (symbolTable.find(token) != symbolTable.end()) {
                    cout << symbolTable[token];
                }

                // ✅ Rule 4: literal (string/char already normalized)
                else {
                    cout << normalizeQuotes(token);
                }

                part = "";
            } 
            else {
                part += content[i];
            }
        }

        cout << endl;
    }
}
string Interpreter::replaceVariables(string expr) {
    string result = "";
    string current = "";

    for (int i = 0; i < expr.length(); i++) {
        char c = expr[i];
        if (isalnum(c) || c == '_') {
            current += c;
        }
        else {
            if (!current.empty()) {
                if (symbolTable.find(current) != symbolTable.end()) {
                    result += symbolTable[current]; // replace with value
                } else {
                    result += current; // just in case
                }
                current = "";
            }
            result += c; // keep operator
        }
    }
    if (!current.empty()) {
        if (symbolTable.find(current) != symbolTable.end())result += symbolTable[current];
        else result += current;
    }

    return result;
}