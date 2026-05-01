// executes statements
#include "interpreter.h"
#include <iostream>
#include <algorithm>
#include <locale> // for smart quotes
#include <stdexcept>
#include <set>

//temporary for evaluationg expressions
#include <sstream>
#include <stack>
#include <cctype>
#include <cmath>

using namespace std;


set<string> reservedWords = {
    "SCRIPT", "AREA",
    "START", "END",
    "SCRIPT", "DECLARE",
    "PRINT", "INT", "CHAR", "BOOL", "FLOAT"
};


//helper functions
// remove spacing
string trim (string str){
    int start = str.find_first_not_of(" \t");
    int end = str.find_last_not_of(" \t");

    if (start == -1) return "";

    return str.substr(start, end-start + 1);
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

// TEMPORARY to solve expressions
double applyOp(double a, double b, char op) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return a / b;
    }
    return 0;
}

// precedence
int precedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    return 0;
}



double evaluateExpression(string expr) {
    stack<double> values;
    stack<char> ops;

    auto applyTop = [&]() {
        if (values.size() < 2)
            error("EVAL-003", "Invalid expression (not enough operands)");

        double b = values.top(); values.pop();

        if (values.empty())
            error("EVAL-004", "Missing operand in expression");

        double a = values.top(); values.pop();

        char op = ops.top(); ops.pop();

        values.push(applyOp(a, b, op));
    };

    for (int i = 0; i < expr.length(); i++) {
        char c = expr[i];

        if (isspace(c)) continue;

        // =====================
        // NUMBER PARSING
        // =====================
        if (isdigit(c)) {
            string num = "";

            while (i < expr.length() &&
                   (isdigit(expr[i]) || expr[i] == '.')) {
                num += expr[i];
                i++;
            }

            i--;
            values.push(stod(num));
        }

        // =====================
        // OPEN PARENTHESIS
        // =====================
        else if (c == '(') {
            ops.push(c);
        }

        // =====================
        // CLOSE PARENTHESIS
        // =====================
        else if (c == ')') {
            while (!ops.empty() && ops.top() != '(') {
                applyTop();
            }

            if (!ops.empty()) ops.pop(); // remove '('
        }

        // =====================
        // OPERATORS
        // =====================
        else if (c == '+' || c == '-' || c == '*' || c == '/') {

            // unary minus
            if (c == '-' &&
                (i == 0 ||
                 expr[i - 1] == '(' ||
                 expr[i - 1] == '+' ||
                 expr[i - 1] == '-' ||
                 expr[i - 1] == '*' ||
                 expr[i - 1] == '/') &&
                (i + 1 < expr.length() &&
                 (isdigit(expr[i + 1]) || expr[i + 1] == '('))) {

                values.push(0);
            }

            // precedence handling
            while (!ops.empty() &&
                   ops.top() != '(' &&
                   precedence(ops.top()) >= precedence(c)) {
                applyTop();
            }

            ops.push(c);
        }
    }

    // =====================
    // FINAL CLEANUP PHASE
    // =====================
    while (!ops.empty()) {

        if (ops.top() == '(') {
            ops.pop();
            continue;
        }

        applyTop();
    }

    // =====================
    // RESULT CHECK
    // =====================
    if (values.empty())
        error("EVAL-005", "Expression evaluation failed");

    return values.top();
}

//VALIDATION for data types
bool isInteger(const string& s) {
    if (s.empty()) return false;

    int i = 0;
    if (s[0] == '-') i = 1;

    for (; i < s.size(); i++) {
        if (!isdigit(s[i])) return false;
    }
    return true;
}

bool isFloat(const string& s) {
    bool dotFound = false;
    int i = 0;

    if (s.empty()) return false;
    if (s[0] == '-') i = 1;

    for (; i < s.size(); i++) {
        if (s[i] == '.') {
            if (dotFound) return false;
            dotFound = true;
        }
        else if (!isdigit(s[i])) return false;
    }

    return dotFound;
}

bool isBool(const string& s) {
    return s == "TRUE" || s == "FALSE";
}

bool isChar(const string& s) {
    return s.size() == 1;
}
bool isStrictChar(const string& s) {
    return s.size() == 3 &&
           s.front() == '\'' &&
           s.back() == '\'';
}

bool isStrictBool(const string& s) {
    return (s == "\"TRUE\"" || s == "\"FALSE\"");
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


// main functnions
Interpreter::Interpreter(vector<string> programLines) {

    lines = programLines;

}



void Interpreter::run() {
    removeComments();
    validateStructure();
    validateOneStatementPerLine();
    validateDeclarationOrder();
    validateStatements();
    
    cout << "LEXOR program structure is valid." << endl; // validates if naaay START SCRIPT, SCRIPT AREA, ug END SCRIPT

    processDeclarations();
    processAssignments();
    processPrint();
}

void Interpreter::validateStatements() {
    for (int i = 0; i < lines.size(); i++) {
        string line = trim(lines[i]);

        if (line == "SCRIPT AREA" ||
            line == "START SCRIPT" ||
            line == "END SCRIPT") {
            continue;
        }

        if (line.empty()) continue;

        bool isDeclare = line.find("DECLARE") == 0;
        bool isPrint   = line.find("PRINT:") == 0;
        bool isAssign  = (!isDeclare && line.find("=") != string::npos);

        if (!isDeclare && !isPrint && !isAssign) {
            error("LEXOR-005",
                  "Invalid or unknown statement: " + line,
                  i + 1);
        }
    }
}


// now removes comments everywhere
void Interpreter::removeComments() {
    vector<string> cleaned;

    for (string line : lines) {
        size_t pos = line.find("%%");

        if (pos != string::npos) {
            line = line.substr(0, pos); 
        }

        line = trim(line);

        if (!line.empty()) {
            cleaned.push_back(line);
        }
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

        if (line.find("DECLARE") != 0) {
            break;
        }

        string rest = line.substr(8);

        int spacePos = rest.find(" ");
        string type = trim(rest.substr(0, spacePos));
        string vars = trim(rest.substr(spacePos + 1));
        if (type != "INT" && type != "FLOAT" && type != "BOOL" && type != "CHAR") {
            error("LEXOR-015", "Invalid data type used in DECLARE: " + type);
        }
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

                    if (type == "CHAR") {
                        if (!(value.size() >= 3 && value.front() == '\'' && value.back() == '\''))
                            error("LEXOR-014", "CHAR must be enclosed in single quotes: " + value);

                        value = value.substr(1, 1);
                    }

                    else if (type == "BOOL") {
                        if (!(value == "\"TRUE\"" || value == "\"FALSE\""))
                            error("LEXOR-013", "BOOL must be enclosed in double quotes: " + value);

                        value = (value == "\"TRUE\"") ? "TRUE" : "FALSE";
                    }
                }
                else {
                    name = token;

                    if (type == "INT")
                        value = "0";
                    else if (type == "FLOAT")
                        value = "0";
                    else if (type == "BOOL")
                        value = "FALSE";
                    else if (type == "CHAR")
                        value = "";
                }

                if (isReservedWord(name)) {
                    error("LEXOR-010", "Reserved word cannot be used as variable name: " + name);
                }

                if (!isValidIdentifier(name)) {
                    error("LEXOR-007", "Invalid variable name: " + name);
                }

                if (type == "INT") {
                    if (!isInteger(value))
                        error("LEXOR-011", "Invalid INT value: " + value);
                }
                else if (type == "FLOAT") {
                    if (!isInteger(value) && !isFloat(value))
                        error("LEXOR-012", "Invalid FLOAT value: " + value);
                }
                else if (type == "BOOL") {
                    if (!isBool(value))
                        error("LEXOR-013", "Invalid BOOL value: " + value);
                }
                else if (type == "CHAR") {
                    if (!isChar(value))
                        error("LEXOR-014", "Invalid CHAR value: " + value);
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
                double result = evaluateExpression(value);
                value = to_string((long long)result);
            }

            value = normalizeQuotes(value);

            for (int j = parts.size() - 2; j >= 0; j--) {
                string var = trim(parts[j]);

                string type = typeTable[var];

                if (type == "INT") {
                    if (!isInteger(value))
                        error("LEXOR-011", "Invalid INT assignment: " + value);
                }
                else if (type == "FLOAT") {
                    if (!isInteger(value) && !isFloat(value))
                        error("LEXOR-012", "Invalid FLOAT assignment: " + value);
                }
                else if (type == "BOOL") {
                    if (!isBool(value))
                        error("LEXOR-013", "Invalid BOOL assignment: " + value);
                }
                else if (type == "CHAR") {
                    if (!isChar(value))
                        error("LEXOR-014", "Invalid CHAR assignment: " + value);
                }

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

        string output = "";
        string buffer = "";
        bool inQuotes = false;

        for (size_t i = 0; i < content.size(); i++) {
            char c = content[i];

            // =====================
            // QUOTES
            // =====================
            if (c == '"') {
                if (inQuotes) {
                    output += buffer;
                    buffer = "";
                }
                inQuotes = !inQuotes;
                continue;
            }

            if (inQuotes) {
                buffer += c;
                continue;
            }

            // =====================
            // NEWLINE TOKEN ($)
            // =====================
            if (c == '$') {
                if (!buffer.empty()) {
                    string token = trim(buffer);

                    if (symbolTable.find(token) != symbolTable.end())
                        output += symbolTable[token];
                    else
                        output += normalizeQuotes(token);

                    buffer = "";
                }

                output += "\n"; 

                continue;
            }

            // EMPTY BRACKETS []
            if (c == '[' && i + 1 < content.size() && content[i + 1] == ']') {
                i++;
                continue;
            }

            if (c == '[' || c == ']') {
                continue;
            }

            // CONCAT OPERATOR
            if (c == '&') {
                if (!buffer.empty()) {
                    string token = trim(buffer);

                    if (symbolTable.find(token) != symbolTable.end())
                        output += symbolTable[token];
                    else
                        output += normalizeQuotes(token);

                    buffer = "";
                }
                continue;
            }

            buffer += c;
        }

        // flush last buffer
        if (!buffer.empty()) {
            string token = trim(buffer);

            if (symbolTable.find(token) != symbolTable.end())
                output += symbolTable[token];
            else
                output += normalizeQuotes(token);
        }

        cout << output << endl;
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