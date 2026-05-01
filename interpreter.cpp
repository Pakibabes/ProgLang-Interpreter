// executes statements
#include "interpreter.h"
#include <iostream>
#include <algorithm>
#include <locale> // for smart quotes
#include <stdexcept>
#include <set>

using namespace std;
//helper functions
// remove spacing

set<string> reservedWords = {
    "SCRIPT", "AREA",
    "START", "END",
    "SCRIPT", "DECLARE",
    "PRINT", "INT", "CHAR", "BOOL", "FLOAT"
};

string trim (string str){
    int start = str.find_first_not_of(" \t");
    int end = str.find_last_not_of(" \t");

    if (start == -1) return "";

    return str.substr(start, end-start + 1);
}

bool startsWith(const string& str, const string& prefix) {
    return str.rfind(prefix, 0) == 0;
}

bool isReservedWord(string word) {
    return reservedWords.find(word) != reservedWords.end();
}

bool isAllUpper(const string& s) {
    for (char c : s) {
        if (isalpha(c) && !isupper(c))
            return false;
    }
    return true;
}

bool isQuoted(const string& token) {
    return (token.size() >= 2 &&
           ((token.front() == '"' && token.back() == '"') ||
            (token.front() == '\'' && token.back() == '\'')));
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

bool isValidIdentifier(const string& name) {
    if (name.empty()) return false;
    if (!(isalpha(name[0]) || name[0] == '_'))
        return false;
    for (char c : name) {
        if (!(isalnum(c) || c == '_'))
            return false;
    }
    return true;
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

string getStatementType(const string& line) {
    if (line.find("DECLARE") == 0) return "DECLARE";
    if (line.find("PRINT:") == 0) return "PRINT";
    if (line.find("=") != string::npos) return "ASSIGN";
    return "UNKNOWN";
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
    validateOneStatementPerLine();
    validateDeclarationOrder();

    
    cout << "LEXOR program structure is valid." << endl; // validates if naaay START SCRIPT, SCRIPT AREA, ug END SCRIPT

    processDeclarations();
    processAssignments();
    processPrint();
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

    bool foundStart = false;
    bool foundEnd = false;

    if (lines.size() < 3) {
        error("LEXOR-000", "Program is too short. Missing required structure.");
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

    for(int i=1; i<lines.size(); i++){
        if(lines[i] == "START SCRIPT"){
            if(foundStart) error("LEXOR-004", "Multiple 'START SCRIPT' found.", i+1);
            foundStart = true;
        }
        else if(lines[i] == "END SCRIPT"){
            if(foundEnd) error("LEXOR-005", "Multiple 'END SCRIPT' found.", i+1);
            foundEnd = true;
        }

        if(foundStart && i > 1 && lines[i] == "SCRIPT AREA"){
            error("LEXOR-006", "'SCRIPT AREA' should only appear at the beginning.", i+1);
        }
    }
}

void Interpreter::processDeclarations() {
    int i = 2;

    while (i < lines.size()) {
        string line = trim(lines[i]);

        // stop when DECLARE phase ends
        if (line.find("DECLARE") != 0) {
            break;
        }

        string rest = line.substr(8);

        int spacePos = rest.find(" ");
        string type = trim(rest.substr(0, spacePos));
        string vars = trim(rest.substr(spacePos + 1));

        string var = "";

        for (int j = 0; j <= vars.length(); j++) {
            if (j == vars.length() || vars[j] == ',') {

                string token = trim(var);
                int equalPos = token.find('=');

                string name;
                string value = "";

                if (equalPos != string::npos) {
                    name = trim(token.substr(0, equalPos));
                    value = trim(token.substr(equalPos + 1));
                    value = normalizeQuotes(value);
                }
            
                else {
                    name = token;

                    if (type == "INT" || type == "FLOAT")
                        value = "0";
                    else if (type == "BOOL")
                        value = "FALSE";
                    else if (type == "CHAR")
                        value = "";
                }

                if (isReservedWord(name)) {
                    error("LEXOR-010",
                          "Reserved word cannot be used as variable name: " + name);
                }

                if (!isValidIdentifier(name)) {
                    error("LEXOR-007",
                          "Invalid variable name: " + name);
                }

                symbolTable[name] = value;
                typeTable[name] = type;

                var = "";
            }
            else {
                var += vars[j];
            }
        }

        i++;
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
    setlocale(LC_ALL, "en_US.UTF-8");

    for (string line : lines) {
        if (line.find("PRINT:") != 0)
            continue;

        string content = trim(line.substr(6));

        vector<string> tokens;
        string buffer = "";
        bool inQuotes = false;

        for (size_t i = 0; i < content.size(); i++) {
            char c = content[i];

            // QUOTES HANDLING
            if (c == '"') {
                if (inQuotes) {
                    tokens.push_back("\"" + buffer + "\"");
                    buffer = "";
                }
                inQuotes = !inQuotes;
                continue;
            }
            if (inQuotes) {
                buffer += c;
                continue;
            }
            // NEWLINE OPERATOR $
            if (c == '$') {
                string cleaned = trim(buffer);
                if (!cleaned.empty()) tokens.push_back(cleaned);
                tokens.push_back("$");
                buffer = "";
            } 
            else if (c == '&') {
                string cleaned = trim(buffer);
                if (!cleaned.empty()) tokens.push_back(cleaned);
                buffer = "";
            } 
            else if (isspace(c)) {
                continue; 
            }
            else {
                buffer += c;
            }
        }

        string cleaned = trim(buffer);
        if (!cleaned.empty()) tokens.push_back(cleaned);

        for (string token : tokens) {
            // NEWLINE
            if (token == "$") {
                cout << endl;
                continue;
            }

            // STRING LITERAL
            if (isQuoted(token)) {
                cout << normalizeQuotes(token);
                continue;
            }

            // VARIABLE
            if (symbolTable.find(token) != symbolTable.end()) {
                cout << symbolTable[token];
                continue;
            }
            error("LEXOR-011", "Invalid PRINT token: " + token);
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

void Interpreter::validateOneStatementPerLine() {
    for (int i = 0; i < lines.size(); i++) {
        string line = trim(lines[i]);

        int declareCount = 0;
        int printCount = 0;

        // count DECLARE occurrences
        size_t pos = line.find("DECLARE");
        while (pos != string::npos) {
            declareCount++;
            pos = line.find("DECLARE", pos + 1);
        }

        if (line.find("PRINT:") == 0) printCount = 1;

        // assignment detection (only if NOT DECLARE line)
        bool hasAssign = (line.find("=") != string::npos && line.find("DECLARE") != 0);

        int statementCount = declareCount + printCount + (hasAssign ? 1 : 0);

        if (statementCount > 1) {
            error("LEXOR-008",
                  "Multiple statements in one line are not allowed: " + line,
                  i + 1);
        }
    }
}

void Interpreter::validateDeclarationOrder() {
    bool executionStarted = false;

    for (int i = 0; i < lines.size(); i++) {
        string line = trim(lines[i]);

        // detect real execution statements ONLY
        bool isPrint = line.find("PRINT:") == 0;
        bool isAssign = (!line.empty() &&
                         line.find("DECLARE") != 0 &&
                         line.find("=") != string::npos);

        if (isPrint || isAssign) {
            executionStarted = true;
        }

        // DECLARE after execution is forbidden
        if (executionStarted && line.find("DECLARE") == 0) {
            error("LEXOR-009",
                  "Variable declarations must appear before executable statements: " + line,
                  i + 1);
        }
    }
}