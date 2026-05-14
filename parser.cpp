#include "parser.h"
#include <sstream>
#include <stdexcept>

using namespace std;

Parser::Parser(const vector<Token>& tokens) : toks(tokens), pos(0) {}

const Token& Parser::peek(int offset) const {
    int idx = pos + offset;
    if (idx >= (int)toks.size()) return toks.back(); // EOF
    return toks[idx];
}

const Token& Parser::advance() {
    const Token& t = toks[pos];
    if (pos < (int)toks.size() - 1) pos++;
    return t;
}

bool Parser::check(TokenType t, int offset) const {
    return peek(offset).type == t;
}

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

const Token& Parser::expect(TokenType t, const string& msg) {
    if (!check(t))
        error("PARSE-001", msg + " (got '" + peek().value + "')", peek().line);
    return advance();
}

bool Parser::atEnd() const {
    return peek().type == TOKEN_EOF;
}

vector<Statement> Parser::parse() {

    // 1. Require SCRIPT AREA
    if (!check(TOKEN_SCRIPT_AREA))
        error("PARSE-100", "Program must begin with 'SCRIPT AREA'", peek().line);
    advance();
    while (match(TOKEN_NEWLINE)) {}

    // 2. Require START SCRIPT
    if (!check(TOKEN_START_SCRIPT))
        error("PARSE-101", "Expected 'START SCRIPT' after 'SCRIPT AREA'", peek().line);
    advance();
    while (match(TOKEN_NEWLINE)) {}

    // 3. Collect declarations first (they must all come before exec stmts)
    vector<Statement> stmts;

    // Skip blank lines
    while (match(TOKEN_NEWLINE)) {}

    // Parse DECLARE statements
    while (!atEnd() && check(TOKEN_DECLARE)) {
        stmts.push_back(parseDeclare());
        while (match(TOKEN_NEWLINE)) {}  // consume end-of-declare-line
    }

    // 4. Parse executable statements until END SCRIPT
    while (!atEnd() && !check(TOKEN_END_SCRIPT)) {
        while (match(TOKEN_NEWLINE)) {}  // skip blank lines
        if (check(TOKEN_END_SCRIPT) || atEnd()) break;
        stmts.push_back(parseStatement());
        while (match(TOKEN_NEWLINE)) {}  // consume end-of-statement line
    }

    // 5. Require END SCRIPT
    if (!check(TOKEN_END_SCRIPT))
        error("PARSE-102", "Expected 'END SCRIPT' at end of program", peek().line);
    advance();

    return stmts;
}

Statement Parser::parseStatement() {

    TokenType t = peek().type;

    if (t == TOKEN_DECLARE)      return parseDeclare();
    if (t == TOKEN_PRINT)        return parsePrint();
    if (t == TOKEN_SCAN)         return parseScan();
    if (t == TOKEN_IF)           return parseIf();
    if (t == TOKEN_FOR)          return parseFor();
    if (t == TOKEN_REPEAT_WHEN)  return parseRepeatWhen();

    // Default: treat as assignment  a = b = expr
    return parseAssignOrExpr();
}

Statement Parser::parseDeclare() {
    Statement s;
    s.type = STMT_DECLARE;
    s.line = peek().line;

    expect(TOKEN_DECLARE, "Expected 'DECLARE'");

    // Data type
    TokenType dt = peek().type;
    if (dt != TOKEN_INT_TYPE && dt != TOKEN_FLOAT_TYPE &&
        dt != TOKEN_BOOL_TYPE && dt != TOKEN_CHAR_TYPE)
        error("PARSE-010", "Expected a data type after DECLARE", peek().line);

    s.dataType = advance().value;   // "INT" / "FLOAT" / "BOOL" / "CHAR"

    s.declarations = parseVarDeclList();
    return s;
}

vector<VarDecl> Parser::parseVarDeclList() {
    vector<VarDecl> list;

    while (true) {
        while (match(TOKEN_NEWLINE)) {}

        if (!check(TOKEN_IDENTIFIER))
            error("PARSE-011", "Expected variable name in declaration", peek().line);

        VarDecl vd;
        vd.name    = advance().value;
        vd.hasInit = false;

        if (match(TOKEN_ASSIGN)) {
            vd.hasInit = true;
            string val;
            // Collect until comma, newline, or structural keyword
            while (!atEnd()
                   && !check(TOKEN_COMMA)
                   && !check(TOKEN_NEWLINE)     // ← stop at end of line
                   && !check(TOKEN_DECLARE)
                   && !check(TOKEN_PRINT)
                   && !check(TOKEN_SCAN)
                   && !check(TOKEN_IF)
                   && !check(TOKEN_FOR)
                   && !check(TOKEN_REPEAT_WHEN)
                   && !check(TOKEN_END_SCRIPT)
                   && !check(TOKEN_START_SCRIPT)
                   && !check(TOKEN_EOF))
            {
                const Token& tok = advance();
                if (tok.type == TOKEN_STRING_LITERAL)
                    val += "\"" + tok.value + "\"";
                else if (tok.type == TOKEN_CHAR_LITERAL)
                    val += "'" + tok.value + "'";
                else
                    val += tok.value;
            }
            vd.initValue = val;
        }

        list.push_back(vd);

        if (!check(TOKEN_COMMA)) break;
        advance();  // consume ','
    }

    return list;
}

Statement Parser::parseAssignOrExpr() {
    Statement s;
    s.type = STMT_ASSIGN;
    s.line = peek().line;

    if (!check(TOKEN_IDENTIFIER))
        error("PARSE-020", "Unexpected token: '" + peek().value + "'", peek().line);

    while (check(TOKEN_IDENTIFIER) && check(TOKEN_ASSIGN, 1)) {
        s.targets.push_back(advance().value);   // identifier
        advance();                               // consume '='
    }

    if (s.targets.empty())
        error("PARSE-021", "Expected assignment statement", peek().line);

    // Collect RHS: collectExpression stops at newline automatically
    s.expr = collectExpression({});  // empty stopAt — newline stops it

    return s;
}

Statement Parser::parsePrint() {
    Statement s;
    s.type = STMT_PRINT;
    s.line = peek().line;

    expect(TOKEN_PRINT,  "Expected 'PRINT'");
    expect(TOKEN_COLON,  "Expected ':' after PRINT");

    string content;
    while (!atEnd()
           && !check(TOKEN_NEWLINE)
           && !check(TOKEN_END_SCRIPT)
           && !check(TOKEN_END_IF)
           && !check(TOKEN_END_FOR)
           && !check(TOKEN_END_REPEAT)
           && !check(TOKEN_ELSE)
           && !check(TOKEN_ELSE_IF)
           && !check(TOKEN_DECLARE)
           && !check(TOKEN_IF)
           && !check(TOKEN_FOR)
           && !check(TOKEN_REPEAT_WHEN)
           && !check(TOKEN_PRINT)
           && !check(TOKEN_SCAN)
           && !check(TOKEN_EOF))
    {
        const Token& tok = advance();

        switch (tok.type) {
            case TOKEN_CONCAT:      content += "&";                       break;
            case TOKEN_DOLLAR:      content += "$";                       break;
            case TOKEN_LBRACKET:    content += "[";                       break;
            case TOKEN_RBRACKET:    content += "]";                       break;
            case TOKEN_STRING_LITERAL:
                content += "\"" + tok.value + "\"";                      break;
            case TOKEN_CHAR_LITERAL:
                content += "'" + tok.value + "'";                        break;
            default:
                content += tok.value;                                     break;
        }
    }

    s.printContent = content;
    return s;
}


Statement Parser::parseScan() {
    Statement s;
    s.type = STMT_SCAN;
    s.line = peek().line;

    expect(TOKEN_SCAN,  "Expected 'SCAN'");
    expect(TOKEN_COLON, "Expected ':' after SCAN");

    s.scanVars = parseIdentifierList();
    return s;
}

Statement Parser::parseIf() {
    Statement s;
    s.type = STMT_IF;
    s.line = peek().line;

    expect(TOKEN_IF, "Expected 'IF'");
    expect(TOKEN_LPAREN, "Expected '(' after IF");

    s.condition = collectExpression({TOKEN_RPAREN});
    expect(TOKEN_RPAREN, "Expected ')' after IF condition");

    while (match(TOKEN_NEWLINE)) {}
    expect(TOKEN_START_IF, "Expected 'START IF'");
    while (match(TOKEN_NEWLINE)) {}

    s.body = parseBlock({TOKEN_END_IF, TOKEN_ELSE_IF, TOKEN_ELSE});

    // ELSE IF chain
    while (check(TOKEN_ELSE_IF)) {
        Statement elif;
        elif.type = STMT_IF;
        elif.line = peek().line;

        advance();  // consume ELSE IF
        expect(TOKEN_LPAREN, "Expected '(' after ELSE IF");
        elif.condition = collectExpression({TOKEN_RPAREN});
        expect(TOKEN_RPAREN, "Expected ')' after ELSE IF condition");
        while (match(TOKEN_NEWLINE)) {}
        expect(TOKEN_START_IF, "Expected 'START IF'");
        while (match(TOKEN_NEWLINE)) {}

        elif.body = parseBlock({TOKEN_END_IF, TOKEN_ELSE_IF, TOKEN_ELSE});

        s.elseBody.push_back(elif);

        if (check(TOKEN_END_IF)) break;
    }

    if (check(TOKEN_ELSE)) {
        advance();  // consume ELSE
        while (match(TOKEN_NEWLINE)) {}
        expect(TOKEN_START_IF, "Expected 'START IF' after ELSE");
        while (match(TOKEN_NEWLINE)) {}

        vector<Statement> elsePart = parseBlock({TOKEN_END_IF});

        if (!s.elseBody.empty()) {
            s.elseBody.back().elseBody = elsePart;
        } else {
            s.elseBody = elsePart;
        }
    }

    while (match(TOKEN_NEWLINE)) {}
    expect(TOKEN_END_IF, "Expected 'END IF'");
    return s;
}

Statement Parser::parseFor() {
    Statement s;
    s.type = STMT_FOR;
    s.line = peek().line;

    expect(TOKEN_FOR,    "Expected 'FOR'");
    expect(TOKEN_LPAREN, "Expected '(' after FOR");

    s.forInit   = collectExpression({TOKEN_COMMA});
    expect(TOKEN_COMMA,  "Expected ',' after FOR initializer");

    s.forCond   = collectExpression({TOKEN_COMMA});
    expect(TOKEN_COMMA,  "Expected ',' after FOR condition");

    s.forUpdate = collectExpression({TOKEN_RPAREN});
    expect(TOKEN_RPAREN, "Expected ')' after FOR update");

    while (match(TOKEN_NEWLINE)) {}
    expect(TOKEN_START_FOR, "Expected 'START FOR'");
    while (match(TOKEN_NEWLINE)) {}
    s.body = parseBlock({TOKEN_END_FOR});
    while (match(TOKEN_NEWLINE)) {}
    expect(TOKEN_END_FOR,   "Expected 'END FOR'");

    return s;
}

Statement Parser::parseRepeatWhen() {
    Statement s;
    s.type = STMT_REPEAT_WHEN;
    s.line = peek().line;

    expect(TOKEN_REPEAT_WHEN, "Expected 'REPEAT WHEN'");
    expect(TOKEN_LPAREN,      "Expected '(' after REPEAT WHEN");
    s.condition = collectExpression({TOKEN_RPAREN});
    expect(TOKEN_RPAREN,      "Expected ')' after REPEAT WHEN condition");

    while (match(TOKEN_NEWLINE)) {}
    expect(TOKEN_START_REPEAT, "Expected 'START REPEAT'");
    while (match(TOKEN_NEWLINE)) {}
    s.body = parseBlock({TOKEN_END_REPEAT});
    while (match(TOKEN_NEWLINE)) {}
    expect(TOKEN_END_REPEAT,   "Expected 'END REPEAT'");

    return s;
}

vector<Statement> Parser::parseBlock(const vector<TokenType>& stopTokens) {
    vector<Statement> body;

    auto isStop = [&]() {
        for (TokenType st : stopTokens)
            if (check(st)) return true;
        return atEnd();
    };

    while (!isStop()) {
        while (match(TOKEN_NEWLINE)) {}
        if (isStop()) break;
        body.push_back(parseStatement());
        while (match(TOKEN_NEWLINE)) {}
    }

    return body;
}

string Parser::collectExpression(const vector<TokenType>& stopAt) {
    string expr;

    auto isStop = [&]() {
        if (atEnd()) return true;
        if (check(TOKEN_NEWLINE)) return true;
        for (TokenType st : stopAt)
            if (check(st)) return true;
        return false;
    };

    bool first = true;
    while (!isStop()) {
        const Token& tok = advance();

        if (!first) {
            switch (tok.type) {
                case TOKEN_RPAREN:
                case TOKEN_LBRACKET:
                case TOKEN_RBRACKET:
                case TOKEN_COMMA:
                    break;
                default:
                    if (expr.size() && expr.back() != '(' && expr.back() != '[')
                        expr += " ";
            }
        }
        first = false;

        switch (tok.type) {
            case TOKEN_STRING_LITERAL:
                expr += "\"" + tok.value + "\""; break;
            case TOKEN_CHAR_LITERAL:
                expr += "'" + tok.value + "'";   break;
            default:
                expr += tok.value;               break;
        }
    }

    size_t s = expr.find_first_not_of(" \t");
    size_t e = expr.find_last_not_of (" \t");
    if (s == string::npos) return "";
    return expr.substr(s, e - s + 1);
}

vector<string> Parser::parseIdentifierList() {
    vector<string> list;

    while (check(TOKEN_IDENTIFIER)) {
        list.push_back(advance().value);
        if (!match(TOKEN_COMMA)) break;
    }

    return list;
}
