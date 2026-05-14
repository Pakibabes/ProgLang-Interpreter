#include "interpreter.h"
#include "lexer.h"
#include "parser.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <stack>
#include <cctype>

using namespace std;
Interpreter::Interpreter(const vector<string>& programLines)
    : lines(programLines) {}

void Interpreter::run() {
    Lexer lexer(lines);
    lexer.tokenize();
    const vector<Token>& tokens = lexer.getTokens();

    Parser parser(tokens);
    vector<Statement> stmts = parser.parse();

    execute(stmts);
}
void Interpreter::execute(const vector<Statement>& stmts) {
    for (const Statement& s : stmts) {
        switch (s.type) {
            case STMT_DECLARE:      execDeclare(s);     break;
            case STMT_ASSIGN:       execAssign(s);      break;
            case STMT_PRINT:        execPrint(s);       break;
            case STMT_SCAN:         execScan(s);        break;
            case STMT_IF:           execIf(s);          break;
            case STMT_FOR:          execFor(s);         break;
            case STMT_REPEAT_WHEN:  execRepeatWhen(s);  break;
            default: break;
        }
    }
}
void Interpreter::execDeclare(const Statement& s) {
    for (const VarDecl& vd : s.declarations) {

        if (vd.name.empty() || !(isalpha(vd.name[0]) || vd.name[0] == '_'))
            error("LEXOR-007", "Invalid variable name: " + vd.name, s.line);

        if (typeTable.count(vd.name))
            error("LEXOR-008", "Variable already declared: " + vd.name, s.line);

        typeTable[vd.name] = s.dataType;

        if (vd.hasInit) {
            string raw = trim(vd.initValue);
            string val;

            if (s.dataType == "CHAR") {
                if (raw.size() >= 3 && raw.front() == '\'' && raw.back() == '\'')
                    val = string(1, raw[1]);
                else
                    error("LEXOR-014",
                          "CHAR initialiser must be a single-quoted character: "
                          + raw, s.line);
            }
            else if (s.dataType == "BOOL") {
                if (raw == "\"TRUE\"" || raw == "TRUE")       val = "TRUE";
                else if (raw == "\"FALSE\"" || raw == "FALSE") val = "FALSE";
                else error("LEXOR-013",
                           "BOOL initialiser must be \"TRUE\" or \"FALSE\": "
                           + raw, s.line);
            }
            else {
                val = evaluate(raw, s.line);
            }

            checkType(vd.name, val, s.line);
            symbolTable[vd.name] = val;
        }
        else {
            symbolTable[vd.name] = defaultValue(s.dataType);
        }
    }
}
void Interpreter::execAssign(const Statement& s) {
    string val = evaluate(s.expr, s.line);

    for (int i = (int)s.targets.size() - 1; i >= 0; i--) {
        const string& var = s.targets[i];

        if (!typeTable.count(var))
            error("LEXOR-009", "Undeclared variable: " + var, s.line);

        checkType(var, val, s.line);
        symbolTable[var] = val;
    }
}

void Interpreter::execPrint(const Statement& s) {
    cout << buildPrintOutput(s.printContent, s.line) << "\n";
}

// ─────────────────────────────────────────────────────────────
//  buildPrintOutput
//  Processes the raw printContent string:
//    &   = concatenator (separator token, ignored structurally)
//    $   = newline
//    [x] = escape code: prints x literally (e.g. [[] prints "[")
//    []  = empty escape (prints nothing — used in spec examples)
//    "…" = string literal
//    var = variable lookup
// ─────────────────────────────────────────────────────────────
string Interpreter::buildPrintOutput(const string& content, int line) {
    string output;
    string buffer;
    bool inQuotes = false;
    char quoteChar = 0;

    auto flushBuffer = [&]() {
        string tok = trim(buffer);
        buffer.clear();
        if (tok.empty()) return;

        // Variable lookup
        if (symbolTable.count(tok)) {
            output += symbolTable[tok];
        } else {
            output += tok;
        }
    };

    for (size_t i = 0; i < content.size(); i++) {
        char c = content[i];

        if (inQuotes) {
            if (c == quoteChar) {
                inQuotes = false;
            } else {
                output += c;
            }
            continue;
        }

        if (c == '"' || c == '\'') {
            flushBuffer();
            inQuotes  = true;
            quoteChar = c;
            continue;
        }

        if (c == '$') {
            flushBuffer();
            output += '\n';
            continue;
        }

        if (c == '&') {
            flushBuffer();
            continue;
        }

        if (c == '[') {
            flushBuffer();
            string escaped;
            i++;
            if (i < content.size() && content[i] == ']') {
                if (i + 1 < content.size() && content[i+1] == ']') {
                    escaped = "]";
                    i += 2;
                } else {
                    i++;
                }
            } else {
                while (i < content.size() && content[i] != ']') {
                    escaped += content[i++];
                }
                if (i < content.size()) i++;
            }
            output += escaped;
            i--;
            continue;
        }

        if (c == ']') continue;

        buffer += c;
    }

    flushBuffer();
    return output;
}

//Implemented the SCAN Function

void Interpreter::execScan(const Statement& s) {
    string inputLine;
    getline(cin, inputLine);

    // Split on commas
    vector<string> values;
    stringstream ss(inputLine);
    string token;
    while (getline(ss, token, ','))
        values.push_back(trim(token));

    if (values.size() != s.scanVars.size())
        error("LEXOR-020",
              "SCAN expected " + to_string(s.scanVars.size()) +
              " value(s), got " + to_string(values.size()), s.line);

    for (size_t i = 0; i < s.scanVars.size(); i++) {
        const string& var = s.scanVars[i];
        if (!typeTable.count(var))
            error("LEXOR-009", "Undeclared variable: " + var, s.line);

        checkType(var, values[i], s.line);
        symbolTable[var] = values[i];
    }
}

void Interpreter::execIf(const Statement& s) {
    string condResult = evaluate(s.condition, s.line);

    if (condResult == "TRUE") {
        execute(s.body);
    } else {
        if (!s.elseBody.empty()) {
            if (s.elseBody.size() == 1 && s.elseBody[0].type == STMT_IF) {
                execIf(s.elseBody[0]);
            } else {
                execute(s.elseBody);
            }
        }
    }
}

void Interpreter::execFor(const Statement& s) {
    {
        size_t eq = s.forInit.find('=');
        if (eq == string::npos)
            error("PARSE-030", "FOR initializer must be an assignment", s.line);

        string var = trim(s.forInit.substr(0, eq));
        string val = evaluate(trim(s.forInit.substr(eq + 1)), s.line);

        if (!typeTable.count(var))
            error("LEXOR-009", "Undeclared variable in FOR: " + var, s.line);
        symbolTable[var] = val;
    }

    int safety = 1000000;
    while (safety-- > 0) {
        string cond = evaluate(s.forCond, s.line);
        if (cond != "TRUE") break;

        execute(s.body);

        size_t eq = s.forUpdate.find('=');
        if (eq == string::npos)
            error("PARSE-031", "FOR update must be an assignment", s.line);

        string var = trim(s.forUpdate.substr(0, eq));
        string val = evaluate(trim(s.forUpdate.substr(eq + 1)), s.line);
        symbolTable[var] = val;
    }
}

void Interpreter::execRepeatWhen(const Statement& s) {
    int safety = 1000000;
    while (safety-- > 0) {
        string cond = evaluate(s.condition, s.line);
        if (cond != "TRUE") break;
        execute(s.body);
    }
}

string Interpreter::evaluate(const string& rawExpr, int line) {
    string expr = trim(rawExpr);

    if (expr.empty())
        error("LEXOR-030", "Empty expression", line);

    if (expr.size() >= 3 && expr.front() == '\'' && expr.back() == '\'')
        return string(1, expr[1]);

    if (expr.size() >= 2 && expr.front() == '"' && expr.back() == '"')
        return expr.substr(1, expr.size() - 2);

    if (expr == "TRUE" || expr == "FALSE") return expr;

    if (symbolTable.count(expr)) return symbolTable[expr];

    if (expr.find(" AND ") != string::npos ||
        expr.find(" OR ")  != string::npos ||
        expr.find("NOT ")  != string::npos) {
        return evalLogical(expr, line);
    }

    auto hasComparison = [&]() {
        int depth = 0;
        for (size_t i = 0; i < expr.size(); i++) {
            char c = expr[i];
            if (c == '(') depth++;
            else if (c == ')') depth--;
            else if (depth == 0) {
                if (i+1 < expr.size()) {
                    string two = string(1,c) + expr[i+1];
                    if (two=="<>" || two==">=" || two=="<=" || two=="==")
                        return true;
                }
                if ((c == '<' || c == '>') && depth == 0) return true;
            }
        }
        return false;
    };

    if (hasComparison()) {
        auto splitComp = [&](const string& op) -> pair<string,string> {
            size_t p = expr.find(op);
            if (p == string::npos) return {"",""};
            return { trim(expr.substr(0, p)),
                     trim(expr.substr(p + op.size())) };
        };

        string lhs, rhs, op;
        for (auto& twoOp : vector<string>{"<>",">=","<=","=="}) {
            size_t p = expr.find(twoOp);
            if (p != string::npos) {
                lhs = trim(expr.substr(0, p));
                rhs = trim(expr.substr(p + 2));
                op  = twoOp;
                break;
            }
        }
        if (op.empty()) {
            for (char sOp : {'<','>'}) {
                size_t p = expr.find(sOp);
                if (p != string::npos) {
                    lhs = trim(expr.substr(0, p));
                    rhs = trim(expr.substr(p + 1));
                    op  = string(1, sOp);
                    break;
                }
            }
        }

        if (!op.empty()) {
            double l = evalArith(substituteVars(lhs), line);
            double r = evalArith(substituteVars(rhs), line);
            bool result;
            if      (op == "<")  result = l <  r;
            else if (op == ">")  result = l >  r;
            else if (op == "<=") result = l <= r;
            else if (op == ">=") result = l >= r;
            else if (op == "==") result = l == r;
            else                 result = l != r;   // <>
            return result ? "TRUE" : "FALSE";
        }
    }

    string substituted = substituteVars(expr);
    double d = evalArith(substituted, line);

    if (d == (long long)d) return to_string((long long)d);

    ostringstream oss;
    oss << d;
    return oss.str();
}

string Interpreter::substituteVars(const string& expr) const {
    string result;
    string buf;

    auto flushBuf = [&]() {
        if (!buf.empty()) {
            if (symbolTable.count(buf)) result += symbolTable.at(buf);
            else                         result += buf;
            buf.clear();
        }
    };

    for (size_t i = 0; i < expr.size(); i++) {
        char c = expr[i];
        if (isalpha(c) || c == '_') {
            buf += c;
        } else if (!buf.empty() && (isalnum(c) || c == '_')) {
            buf += c;
        } else {
            flushBuf();
            result += c;
        }
    }
    flushBuf();
    return result;
}

string Interpreter::evalLogical(const string& expr, int line) {
    // Strip outer parens
    string e = trim(expr);
    while (e.size() >= 2 && e.front() == '(' && e.back() == ')') {
        // Verify they match
        int depth = 0;
        bool matched = true;
        for (size_t i = 0; i < e.size()-1; i++) {
            if (e[i]=='(') depth++;
            else if (e[i]==')') { depth--; if (depth==0){matched=false;break;} }
        }
        if (matched) e = trim(e.substr(1, e.size()-2));
        else break;
    }

    if (e.substr(0, 4) == "NOT ") {
        string sub = trim(e.substr(4));
        string val = evaluate(sub, line);
        if (val != "TRUE" && val != "FALSE")
            error("LEXOR-031", "NOT requires a BOOL expression", line);
        return (val == "TRUE") ? "FALSE" : "TRUE";
    }

    int depth = 0;
    for (size_t i = 0; i < e.size(); i++) {
        if (e[i]=='(') depth++;
        else if (e[i]==')') depth--;
        else if (depth==0 && e.substr(i,4)==" OR ") {
            string lhs = evaluate(trim(e.substr(0,i)), line);
            string rhs = evaluate(trim(e.substr(i+4)), line);
            return (lhs=="TRUE" || rhs=="TRUE") ? "TRUE" : "FALSE";
        }
    }

    depth = 0;
    for (size_t i = 0; i < e.size(); i++) {
        if (e[i]=='(') depth++;
        else if (e[i]==')') depth--;
        else if (depth==0 && e.substr(i,5)==" AND ") {
            string lhs = evaluate(trim(e.substr(0,i)), line);
            string rhs = evaluate(trim(e.substr(i+5)), line);
            return (lhs=="TRUE" && rhs=="TRUE") ? "TRUE" : "FALSE";
        }
    }

    return evaluate(e, line);
}

int Interpreter::precedence(char op) {
    if (op=='+' || op=='-') return 1;
    if (op=='*' || op=='/' || op=='%') return 2;
    return 0;
}

double Interpreter::applyOp(double a, double b, char op, int line) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/':
            if (b == 0) error("LEXOR-040", "Division by zero", line);
            return a / b;
        case '%':
            if (b == 0) error("LEXOR-041", "Modulo by zero", line);
            return fmod(a, b);
    }
    error("LEXOR-042", string("Unknown operator: ") + op, line);
    return 0;
}

double Interpreter::evalArith(const string& rawExpr, int line) {
    string expr = trim(rawExpr);

    stack<double> vals;
    stack<char>   ops;

    auto doOp = [&]() {
        if (vals.size() < 2)
            error("LEXOR-043", "Malformed expression: " + expr, line);
        double b = vals.top(); vals.pop();
        double a = vals.top(); vals.pop();
        char   o = ops.top();  ops.pop();
        vals.push(applyOp(a, b, o, line));
    };

    size_t i = 0;
    bool expectUnary = true;   // true at start or after '('

    while (i < expr.size()) {
        char c = expr[i];

        // Skip whitespace
        if (isspace(c)) { i++; continue; }

        // Number (possibly with leading sign handled as unary)
        if (isdigit(c) || (c=='.' && i+1<expr.size() && isdigit(expr[i+1]))) {
            string numStr;
            while (i<expr.size() && (isdigit(expr[i]) || expr[i]=='.'))
                numStr += expr[i++];
            vals.push(stod(numStr));
            expectUnary = false;
            continue;
        }

        // Unary +/-
        if ((c=='+' || c=='-') && expectUnary) {
            string numStr(1, c);
            i++;
            while (i<expr.size() && (isdigit(expr[i]) || expr[i]=='.'))
                numStr += expr[i++];
            if (numStr.size()==1)
                error("LEXOR-044", "Dangling unary sign in: " + expr, line);
            vals.push(stod(numStr));
            expectUnary = false;
            continue;
        }

        // Left paren
        if (c == '(') {
            ops.push('(');
            i++;
            expectUnary = true;
            continue;
        }

        // Right paren
        if (c == ')') {
            while (!ops.empty() && ops.top() != '(') doOp();
            if (ops.empty())
                error("LEXOR-045", "Mismatched parentheses in: " + expr, line);
            ops.pop();  // pop '('
            i++;
            expectUnary = false;
            continue;
        }

        // Binary operator
        if (c=='+' || c=='-' || c=='*' || c=='/' || c=='%') {
            while (!ops.empty() && ops.top()!='(' &&
                   precedence(ops.top()) >= precedence(c))
                doOp();
            ops.push(c);
            i++;
            expectUnary = true;
            continue;
        }

        error("LEXOR-046", string("Unexpected character '") + c
              + "' in arithmetic expression: " + expr, line);
    }

    while (!ops.empty()) {
        if (ops.top() == '(')
            error("LEXOR-045", "Mismatched parentheses in: " + expr, line);
        doOp();
    }

    if (vals.empty())
        error("LEXOR-047", "Empty arithmetic expression: " + expr, line);

    return vals.top();
}

void Interpreter::checkType(const string& var,
                             const string& value, int line) {
    const string& type = typeTable.at(var);

    if (type == "INT") {
        if (!isInteger(value))
            error("LEXOR-011",
                  "Type error: INT variable '" + var +
                  "' cannot hold '" + value + "'", line);
    }
    else if (type == "FLOAT") {
        if (!isInteger(value) && !isFloat(value))
            error("LEXOR-012",
                  "Type error: FLOAT variable '" + var +
                  "' cannot hold '" + value + "'", line);
    }
    else if (type == "BOOL") {
        if (value != "TRUE" && value != "FALSE")
            error("LEXOR-013",
                  "Type error: BOOL variable '" + var +
                  "' cannot hold '" + value + "'", line);
    }
    else if (type == "CHAR") {
        if (value.size() != 1)
            error("LEXOR-014",
                  "Type error: CHAR variable '" + var +
                  "' cannot hold '" + value + "'", line);
    }
}

string Interpreter::defaultValue(const string& type) {
    if (type == "INT")   return "0";
    if (type == "FLOAT") return "0.0";
    if (type == "BOOL")  return "FALSE";
    if (type == "CHAR")  return " ";
    return "";
}

string Interpreter::trim(const string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == string::npos) return "";
    size_t e = s.find_last_not_of (" \t\r\n");
    return s.substr(b, e - b + 1);
}
bool Interpreter::isInteger(const string& s) {
    if (s.empty()) return false;
    int i = (s[0]=='-'||s[0]=='+') ? 1 : 0;
    if (i==(int)s.size()) return false;
    for (; i<(int)s.size(); i++) if (!isdigit(s[i])) return false;
    return true;
}
bool Interpreter::isFloat(const string& s) {
    if (s.empty()) return false;
    int i = (s[0]=='-'||s[0]=='+') ? 1 : 0;
    bool dot = false;
    for (; i<(int)s.size(); i++) {
        if (s[i]=='.') { if(dot) return false; dot=true; }
        else if (!isdigit(s[i])) return false;
    }
    return dot;
}
bool Interpreter::isBool(const string& s){return s=="TRUE"||s=="FALSE";}
bool Interpreter::isChar(const string& s){return s.size()==1;}
