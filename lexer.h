#ifndef LEXER_H
#define LEXER_H

#include "common.h"
#include <string>
#include <vector>

using namespace std;

class Lexer {
public:
    Lexer(const vector<string>& sourceLines);

    // Tokenise all lines; call once before getTokens()
    void tokenize();

    const vector<Token>& getTokens() const;

private:
    vector<string> lines;
    vector<Token>  tokens;

    static string  trim(const string& s);
    static bool    isIdentChar(char c, bool first);

    static bool    isInteger (const string& s);
    static bool    isFloat   (const string& s);
    static bool    isBool    (const string& s);

    // ── token emitters ───────────────────────────
    void emit(TokenType type, const string& value, int line);
    void classifyAndEmit(const string& word, int line);
};

#endif // LEXER_H
