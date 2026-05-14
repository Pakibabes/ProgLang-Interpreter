#ifndef PARSER_H
#define PARSER_H

#include "common.h"
#include <vector>
#include <string>

using namespace std;

// ─────────────────────────────────────────────────────────────
//  Parser
//
//  Consumes the flat Token list produced by the Lexer and
//  organises them into a vector<Statement> that the Interpreter
//  can walk linearly.
//
//  Public API
//  ──────────
//  Parser p(tokens);
//  vector<Statement> stmts = p.parse();
// ─────────────────────────────────────────────────────────────
class Parser {
public:
    explicit Parser(const vector<Token>& tokens);

    // Entry point: returns the complete program as a list of
    // top-level Statements (declarations, assignments, print,
    // scan, if-blocks, for-blocks, repeat-blocks).
    vector<Statement> parse();

private:
    vector<Token> toks;
    int           pos;   // current token cursor

    // ── token navigation ─────────────────────────────────
    const Token& peek(int offset = 0) const;
    const Token& advance();
    bool         check(TokenType t, int offset = 0) const;
    bool         match(TokenType t);          // consume if matches
    const Token& expect(TokenType t, const string& errMsg);

    bool atEnd() const;

    // ── top-level structure parse ─────────────────────────
    void              parseScriptArea();
    void              parseStartScript();
    void              parseEndScript();

    // ── statement dispatchers ────────────────────────────
    // Parses one statement at the current position.
    // Returns false if we hit a block-terminator (END IF etc.)
    // that the *caller* should handle.
    Statement         parseStatement();

    // ── concrete statement parsers ────────────────────────
    Statement         parseDeclare();
    Statement         parseAssignOrExpr();   // handles  a=b=expr
    Statement         parsePrint();
    Statement         parseScan();
    Statement         parseIf();
    Statement         parseFor();
    Statement         parseRepeatWhen();

    // ── helpers ───────────────────────────────────────────
    // Collect the rest of the tokens on the current logical
    // "line" (up to the next structural keyword or EOF) as
    // a raw expression string, for the interpreter to evaluate.
    string            collectExpression(
                          const vector<TokenType>& stopAt = {});

    // Parse a comma-separated identifier list into a vector<string>
    vector<string>    parseIdentifierList();

    // Parse a block: statements until we see one of the stop tokens
    vector<Statement> parseBlock(const vector<TokenType>& stopTokens);

    // Parse variable declarator list after the data-type keyword:
    //   x, y=5, z='a'
    vector<VarDecl>   parseVarDeclList();
};

#endif // PARSER_H
