#include "lexer.h"
#include <cctype>
#include <stdexcept>

using namespace std;

// ─────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────
Lexer::Lexer(const vector<string>& sourceLines) : lines(sourceLines) {}

// ─────────────────────────────────────────────────────────────
//  Public interface
// ─────────────────────────────────────────────────────────────
const vector<Token>& Lexer::getTokens() const { return tokens; }

// ─────────────────────────────────────────────────────────────
//  Main tokeniser
//
//  Strategy: walk each line character-by-character.
//  We handle:
//    - %% comments         (skip rest of line)
//    - "string literals"   (keep inner text as TOKEN_STRING_LITERAL)
//    - 'char literals'     (keep as TOKEN_CHAR_LITERAL)
//    - two-char operators  (<>, >=, <=, ==)
//    - single-char symbols (+, -, *, /, %, (, ), ,, :, &, $, [, ])
//    - identifier / keyword / number words (accumulated in buffer)
//
//  After collecting all raw single-line tokens we do a second pass
//  to collapse multi-word tokens:
//    SCRIPT AREA  →  TOKEN_SCRIPT_AREA
//    START SCRIPT →  TOKEN_START_SCRIPT
//    END SCRIPT   →  TOKEN_END_SCRIPT
//    START IF     →  TOKEN_START_IF
//    END IF       →  TOKEN_END_IF
//    START FOR    →  TOKEN_START_FOR
//    END FOR      →  TOKEN_END_FOR
//    START REPEAT →  TOKEN_START_REPEAT
//    END REPEAT   →  TOKEN_END_REPEAT
//    ELSE IF      →  TOKEN_ELSE_IF
//    REPEAT WHEN  →  TOKEN_REPEAT_WHEN
// ─────────────────────────────────────────────────────────────
void Lexer::tokenize() {

    for (int lineIdx = 0; lineIdx < (int)lines.size(); lineIdx++) {

        string raw  = lines[lineIdx];
        int    lineNo = lineIdx + 1;   // 1-based for error messages

        // ── strip %% comments ────────────────────────────────
        size_t commentPos = raw.find("%%");
        if (commentPos != string::npos)
            raw = raw.substr(0, commentPos);

        raw = trim(raw);
        if (raw.empty()) continue;

        // Temporary per-line token list (before multi-word collapse)
        vector<Token> lineTokens;

        auto emitLine = [&](TokenType t, const string& v) {
            lineTokens.push_back({t, v, lineNo});
        };

        string buffer;
        auto flushBuffer = [&]() {
            if (!buffer.empty()) {
                // Determine token type for accumulated word
                string w = buffer;
                buffer.clear();

                // Keywords (single-word)
                if (w == "DECLARE")      emitLine(TOKEN_DECLARE,     w);
                else if (w == "PRINT")   emitLine(TOKEN_PRINT,       w);
                else if (w == "SCAN")    emitLine(TOKEN_SCAN,        w);
                else if (w == "IF")      emitLine(TOKEN_IF,          w);
                else if (w == "ELSE")    emitLine(TOKEN_ELSE,        w);
                else if (w == "FOR")     emitLine(TOKEN_FOR,         w);
                else if (w == "REPEAT")  emitLine(TOKEN_UNKNOWN,     w); // will be collapsed
                else if (w == "WHEN")    emitLine(TOKEN_UNKNOWN,     w);
                else if (w == "SCRIPT")  emitLine(TOKEN_UNKNOWN,     w); // collapsed later
                else if (w == "AREA")    emitLine(TOKEN_UNKNOWN,     w);
                else if (w == "START")   emitLine(TOKEN_UNKNOWN,     w);
                else if (w == "END")     emitLine(TOKEN_UNKNOWN,     w);
                // Logical
                else if (w == "AND")     emitLine(TOKEN_AND,         w);
                else if (w == "OR")      emitLine(TOKEN_OR,          w);
                else if (w == "NOT")     emitLine(TOKEN_NOT,         w);
                // Data types
                else if (w == "INT")     emitLine(TOKEN_INT_TYPE,    w);
                else if (w == "FLOAT")   emitLine(TOKEN_FLOAT_TYPE,  w);
                else if (w == "BOOL")    emitLine(TOKEN_BOOL_TYPE,   w);
                else if (w == "CHAR")    emitLine(TOKEN_CHAR_TYPE,   w);
                // Bool literals (unquoted TRUE/FALSE used in expressions)
                else if (isBool(w))      emitLine(TOKEN_BOOL_LITERAL, w);
                // Number literals
                else if (isInteger(w))   emitLine(TOKEN_INT_LITERAL,  w);
                else if (isFloat(w))     emitLine(TOKEN_FLOAT_LITERAL, w);
                // Identifier
                else                     emitLine(TOKEN_IDENTIFIER,   w);
            }
        };

        // ── character scan ───────────────────────────────────
        for (int i = 0; i < (int)raw.size(); i++) {
            char c = raw[i];

            // ── whitespace: flush buffer ──────────────────
            if (isspace(c)) {
                flushBuffer();
                continue;
            }

            // ── string literal "..." ──────────────────────
            if (c == '"') {
                flushBuffer();
                string lit;
                i++;
                while (i < (int)raw.size() && raw[i] != '"')
                    lit += raw[i++];
                // i now points at closing "
                emitLine(TOKEN_STRING_LITERAL, lit);
                continue;
            }

            // ── char literal '.' ──────────────────────────
            if (c == '\'') {
                flushBuffer();
                string lit;
                i++;
                while (i < (int)raw.size() && raw[i] != '\'')
                    lit += raw[i++];
                emitLine(TOKEN_CHAR_LITERAL, lit);
                continue;
            }

            // ── two-char operators ────────────────────────
            if (i + 1 < (int)raw.size()) {
                string two = string(1, c) + raw[i+1];
                if (two == "<>") { flushBuffer(); emitLine(TOKEN_NEQ, two); i++; continue; }
                if (two == ">=") { flushBuffer(); emitLine(TOKEN_GTE, two); i++; continue; }
                if (two == "<=") { flushBuffer(); emitLine(TOKEN_LTE, two); i++; continue; }
                if (two == "==") { flushBuffer(); emitLine(TOKEN_EQ,  two); i++; continue; }
                // unary negative number: -digit (only if buffer empty, meaning nothing before it)
                if (c == '-' && isdigit(raw[i+1]) && buffer.empty()) {
                    buffer += c;
                    continue;
                }
            }

            // ── single-char symbols ───────────────────────
            switch (c) {
                case '+': flushBuffer(); emitLine(TOKEN_PLUS,     "+"); continue;
                case '-': flushBuffer(); emitLine(TOKEN_MINUS,    "-"); continue;
                case '*': flushBuffer(); emitLine(TOKEN_MULTIPLY, "*"); continue;
                case '/': flushBuffer(); emitLine(TOKEN_DIVIDE,   "/"); continue;
                case '%': flushBuffer(); emitLine(TOKEN_MODULO,   "%"); continue;
                case '>': flushBuffer(); emitLine(TOKEN_GT,       ">"); continue;
                case '<': flushBuffer(); emitLine(TOKEN_LT,       "<"); continue;
                case '=': flushBuffer(); emitLine(TOKEN_ASSIGN,   "="); continue;
                case '(': flushBuffer(); emitLine(TOKEN_LPAREN,   "("); continue;
                case ')': flushBuffer(); emitLine(TOKEN_RPAREN,   ")"); continue;
                case ',': flushBuffer(); emitLine(TOKEN_COMMA,    ","); continue;
                case ':': flushBuffer(); emitLine(TOKEN_COLON,    ":"); continue;
                case '&': flushBuffer(); emitLine(TOKEN_CONCAT,   "&"); continue;
                case '$': flushBuffer(); emitLine(TOKEN_DOLLAR,   "$"); continue;
                case '[': flushBuffer(); emitLine(TOKEN_LBRACKET, "["); continue;
                case ']': flushBuffer(); emitLine(TOKEN_RBRACKET, "]"); continue;
            }

            // ── accumulate into buffer ────────────────────
            buffer += c;
        }

        flushBuffer();

        // ── collapse multi-word tokens ────────────────────────
        //  Walk lineTokens and merge consecutive unknown-keyword pairs
        for (int k = 0; k < (int)lineTokens.size(); k++) {
            auto& t = lineTokens[k];

            if (k + 1 < (int)lineTokens.size()) {
                string pair = t.value + " " + lineTokens[k+1].value;

                if (pair == "SCRIPT AREA")  { tokens.push_back({TOKEN_SCRIPT_AREA,  pair, lineNo}); k++; continue; }
                if (pair == "START SCRIPT") { tokens.push_back({TOKEN_START_SCRIPT, pair, lineNo}); k++; continue; }
                if (pair == "END SCRIPT")   { tokens.push_back({TOKEN_END_SCRIPT,   pair, lineNo}); k++; continue; }
                if (pair == "START IF")     { tokens.push_back({TOKEN_START_IF,     pair, lineNo}); k++; continue; }
                if (pair == "END IF")       { tokens.push_back({TOKEN_END_IF,       pair, lineNo}); k++; continue; }
                if (pair == "START FOR")    { tokens.push_back({TOKEN_START_FOR,    pair, lineNo}); k++; continue; }
                if (pair == "END FOR")      { tokens.push_back({TOKEN_END_FOR,      pair, lineNo}); k++; continue; }
                if (pair == "START REPEAT") { tokens.push_back({TOKEN_START_REPEAT, pair, lineNo}); k++; continue; }
                if (pair == "END REPEAT")   { tokens.push_back({TOKEN_END_REPEAT,   pair, lineNo}); k++; continue; }
                if (pair == "ELSE IF")      { tokens.push_back({TOKEN_ELSE_IF,      pair, lineNo}); k++; continue; }
                if (pair == "REPEAT WHEN")  { tokens.push_back({TOKEN_REPEAT_WHEN,  pair, lineNo}); k++; continue; }
            }

            // Not a multi-word pair — emit as-is
            tokens.push_back(t);
        }

        // Mark end of this source line so the parser can use it as a boundary
        tokens.push_back({TOKEN_NEWLINE, "\n", lineNo});

    }  // end for each line

    tokens.push_back({TOKEN_EOF, "", (int)lines.size()});
}

// ─────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────
string Lexer::trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end   = s.find_last_not_of (" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool Lexer::isInteger(const string& s) {
    if (s.empty()) return false;
    int i = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if (i == (int)s.size()) return false;
    for (; i < (int)s.size(); i++)
        if (!isdigit(s[i])) return false;
    return true;
}

bool Lexer::isFloat(const string& s) {
    if (s.empty()) return false;
    int i = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    if (i == (int)s.size()) return false;
    bool dot = false;
    for (; i < (int)s.size(); i++) {
        if (s[i] == '.') { if (dot) return false; dot = true; }
        else if (!isdigit(s[i])) return false;
    }
    return dot;
}

bool Lexer::isBool(const string& s) {
    return s == "TRUE" || s == "FALSE";
}
