#pragma once
#include "token.h"
#include "ast.h"
#include "turing_machine.h"
#include <vector>
#include <stdexcept>
#include <string>
#include <functional>

// ============================================================
//  PARSER — berbasis TM<Token>
//
//  Tape  : TokenStream (hasil Lexer)
//  Head  : indeks token saat ini
//  State : phase parsing (stmt, expr, dsb.) — diemulasi via
//          recursive-descent dengan tiap fungsi adalah "state"
//
//  Parser mengonsumsi token satu per satu dan membangun AST.
//  Error → throw ParseError dengan info baris
// ============================================================

class ParseError : public std::runtime_error {
public:
    int line, col;
    ParseError(const std::string& msg, int l = 0, int c = 0)
        : std::runtime_error(msg), line(l), col(c) {}
};

class Parser {
public:
    ASTNodePtr parse(const TokenStream& toks) {
        tokens = toks;
        pos    = 0;

        auto program = std::make_shared<ASTNode>(NodeType::PROGRAM);
        while (!atEnd()) {
            program->add(parseStatement());
        }
        return program;
    }

private:
    TokenStream tokens;
    size_t      pos = 0;

    // ── Tape helpers ─────────────────────────────────────────
    Token& cur()  { return tokens[pos]; }
    Token& peek(int offset = 1) {
        size_t i = pos + offset;
        if (i >= tokens.size()) return tokens.back();
        return tokens[i];
    }
    bool atEnd() {
        return pos >= tokens.size() || cur().type == TokenType::END_OF_FILE;
    }
    Token advance() {
        Token t = tokens[pos];
        if (!atEnd()) pos++;
        return t;
    }
    Token expect(TokenType t, const std::string& msg = "") {
        if (cur().type != t) {
            std::string m = msg.empty()
                ? "Ekspektasi '" + Token{t,"",0,0}.typeName() + "' tapi dapat '" + cur().value + "'"
                : msg;
            throw ParseError(m, cur().line, cur().col);
        }
        return advance();
    }
    bool check(TokenType t) { return !atEnd() && cur().type == t; }
    bool match(TokenType t) { if (check(t)) { advance(); return true; } return false; }

    // ── Statements ────────────────────────────────────────────
    ASTNodePtr parseStatement() {
        // Lewati semicolon kosong
        while (match(TokenType::SEMICOLON)) {}
        if (atEnd()) return nullptr;

        Token t = cur();

        // Blok { }
        if (t.type == TokenType::LBRACE)        return parseBlock();

        // Deklarasi variabel
        if (t.type == TokenType::KW_ANGKA   ||
            t.type == TokenType::KW_NGAMBANG ||
            t.type == TokenType::KW_HURUF)       return parseVarDecl();

        // Kontrol alur
        if (t.type == TokenType::KW_KALAU)       return parseIf();
        if (t.type == TokenType::KW_ULANG_KALAU) return parseWhile();
        if (t.type == TokenType::KW_UNTUK)       return parseFor();

        // I/O
        if (t.type == TokenType::KW_MASUKIN)     return parseInput();
        if (t.type == TokenType::KW_KELUARIN)    return parseOutput();

        // berhenti / lanjut
        if (t.type == TokenType::KW_BERHENTI) {
            advance();
            expect(TokenType::SEMICOLON);
            return std::make_shared<ASTNode>(NodeType::BREAK_STMT, t);
        }
        if (t.type == TokenType::KW_LANJUT) {
            advance();
            expect(TokenType::SEMICOLON);
            return std::make_shared<ASTNode>(NodeType::CONTINUE_STMT, t);
        }

       // Assignment: IDENTIFIER = expr ;
        if (t.type == TokenType::IDENTIFIER &&
            peek().type == TokenType::OP_ASSIGN) {
            return parseAssign();
        }

        // Expression statement (fallback)
        auto node = std::make_shared<ASTNode>(NodeType::EXPR_STMT, t);
        node->add(parseExpr());
        expect(TokenType::SEMICOLON, "Kurang ';' di akhir baris");
        return node;
    }

    ASTNodePtr parseBlock() {
        Token brace = expect(TokenType::LBRACE);
        auto block  = std::make_shared<ASTNode>(NodeType::BLOCK, brace);
        while (!atEnd() && cur().type != TokenType::RBRACE) {
            auto stmt = parseStatement();
            if (stmt) block->add(stmt);
        }
        expect(TokenType::RBRACE, "Kurang '}' — blok tidak ditutup");
        return block;
    }

    // tipe nama [, nama]* [itu expr] ;
    ASTNodePtr parseVarDecl() {
        Token typeTok = advance(); // angka / ngambang / huruf
        auto  node    = std::make_shared<ASTNode>(NodeType::VAR_DECL, typeTok);
        node->sval    = typeTok.value; // tipe

        // Daftar nama variabel (bisa lebih dari satu, dipisah koma)
        do {
            Token nameTok = expect(TokenType::IDENTIFIER, "Ekspektasi nama variabel");
            auto varNode  = std::make_shared<ASTNode>(NodeType::EXPR_VAR, nameTok);
            varNode->sval = nameTok.value;

            // Inisialisasi opsional: itu expr
            if (cur().type == TokenType::OP_ASSIGN) {
                advance();
                varNode->add(parseExpr());
            }
            node->add(varNode);
        } while (match(TokenType::COMMA));

        expect(TokenType::SEMICOLON, "Kurang ';' setelah deklarasi variabel");
        return node;
    }

    // nama itu expr ;
    ASTNodePtr parseAssign() {
        Token nameTok = advance(); // IDENTIFIER
        Token ituTok  = advance(); // =
        auto  node    = std::make_shared<ASTNode>(NodeType::ASSIGN, ituTok);
        auto  varNode = std::make_shared<ASTNode>(NodeType::EXPR_VAR, nameTok);
        varNode->sval = nameTok.value;
        node->add(varNode);
        node->add(parseExpr());
        expect(TokenType::SEMICOLON, "Kurang ';' setelah assignment");
        return node;
    }

    // kalau ( expr ) { block } [kalau tidak ( expr ) { block }]* [lain { block }]
    ASTNodePtr parseIf() {
        Token kTok = expect(TokenType::KW_KALAU);
        auto  node = std::make_shared<ASTNode>(NodeType::IF_STMT, kTok);
        node->add(parseExpr());  // kondisi
        node->add(parseBlock()); // body

        // kalau tidak
        while (check(TokenType::KW_KALAU_TIDAK)) {
            advance();
            node->add(parseExpr());   // kondisi else-if
            node->add(parseBlock());
        }
        // lain
        if (match(TokenType::KW_LAIN)) {
            node->add(parseBlock());
        }
        return node;
    }

    // ulang kalau expr { block }
    ASTNodePtr parseWhile() {
        Token tok  = expect(TokenType::KW_ULANG_KALAU);
        auto  node = std::make_shared<ASTNode>(NodeType::WHILE_STMT, tok);
        node->add(parseExpr());
        node->add(parseBlock());
        return node;
    }

    // untuk [var] itu [awal] trs [kondisi] nah [step] { block }
    ASTNodePtr parseFor() {
        Token tok  = expect(TokenType::KW_UNTUK);
        auto  node = std::make_shared<ASTNode>(NodeType::FOR_STMT, tok);

        // var (identifier)
        Token varTok = expect(TokenType::IDENTIFIER, "Ekspektasi nama variabel di 'untuk'");
        auto  varNode = std::make_shared<ASTNode>(NodeType::EXPR_VAR, varTok);
        varNode->sval = varTok.value;
        node->add(varNode);

        // itu init
        expect(TokenType::KW_ITU, "'itu' tidak ditemukan di 'untuk'");
        node->add(parseExpr());

        // trs kondisi
        expect(TokenType::KW_TRS, "'trs' tidak ditemukan di 'untuk'");
        node->add(parseExpr());

        // nah step — bisa berupa "nama itu expr" atau "nama++" dsb.
        expect(TokenType::KW_NAH, "'nah' tidak ditemukan di 'untuk'");
        // Baca step sebagai assignment atau expr
        if (cur().type == TokenType::IDENTIFIER && peek().type == TokenType::KW_ITU) {
            Token sn = advance(); advance(); // nama, itu
            auto sNode = std::make_shared<ASTNode>(NodeType::ASSIGN, sn);
            auto snv   = std::make_shared<ASTNode>(NodeType::EXPR_VAR, sn);
            snv->sval  = sn.value;
            sNode->add(snv);
            sNode->add(parseExpr());
            node->add(sNode);
        } else {
            node->add(parseExpr());
        }

        node->add(parseBlock());
        return node;
    }

    // masukin >> var [>> var] ;
    ASTNodePtr parseInput() {
        Token tok = advance(); // masukin
        auto  node = std::make_shared<ASTNode>(NodeType::IO_IN, tok);
        do {
            expect(TokenType::OP_STREAM_IN, "Gunakan '>>' setelah 'masukin'");
            Token varTok = expect(TokenType::IDENTIFIER, "Ekspektasi nama variabel");
            auto v = std::make_shared<ASTNode>(NodeType::EXPR_VAR, varTok);
            v->sval = varTok.value;
            node->add(v);
        } while (check(TokenType::OP_STREAM_IN));
        expect(TokenType::SEMICOLON, "Kurang ';' setelah masukin");
        return node;
    }

    // keluarin << expr [<< expr] ;
    ASTNodePtr parseOutput() {
        Token tok = advance(); // keluarin
        auto  node = std::make_shared<ASTNode>(NodeType::IO_OUT, tok);
        do {
            expect(TokenType::OP_STREAM_OUT, "Gunakan '<<' setelah 'keluarin'");
            if (check(TokenType::KW_ENDL)) {
                auto e = std::make_shared<ASTNode>(NodeType::EXPR_LITERAL, cur());
                e->sval = "endl";
                advance();
                node->add(e);
            } else {
                node->add(parseExpr());
            }
        } while (check(TokenType::OP_STREAM_OUT));
        expect(TokenType::SEMICOLON, "Kurang ';' setelah keluarin");
        return node;
    }

    // ── Expressions (precedence climbing) ─────────────────────
    ASTNodePtr parseExpr()      { return parseOr(); }

    ASTNodePtr parseOr() {
        auto left = parseAnd();
        while (check(TokenType::KW_ATAU)) {
            Token op = advance();
            auto right = parseAnd();
            auto node  = std::make_shared<ASTNode>(NodeType::EXPR_BINARY, op);
            node->sval = "||";
            node->add(left); node->add(right);
            left = node;
        }
        return left;
    }
    ASTNodePtr parseAnd() {
        auto left = parseEquality();
        while (check(TokenType::KW_DAN)) {
            Token op = advance();
            auto right = parseEquality();
            auto node  = std::make_shared<ASTNode>(NodeType::EXPR_BINARY, op);
            node->sval = "&&";
            node->add(left); node->add(right);
            left = node;
        }
        return left;
    }
    ASTNodePtr parseEquality() {
        auto left = parseRelational();
        while (check(TokenType::OP_EQ) || check(TokenType::OP_NEQ)) {
            Token op  = advance();
            auto right = parseRelational();
            auto node  = std::make_shared<ASTNode>(NodeType::EXPR_BINARY, op);
            node->sval = op.value;
            node->add(left); node->add(right);
            left = node;
        }
        return left;
    }
    ASTNodePtr parseRelational() {
        auto left = parseAddSub();
        while (check(TokenType::OP_LT)  || check(TokenType::OP_GT) ||
               check(TokenType::OP_LTE) || check(TokenType::OP_GTE)) {
            Token op  = advance();
            auto right = parseAddSub();
            auto node  = std::make_shared<ASTNode>(NodeType::EXPR_BINARY, op);
            node->sval = op.value;
            node->add(left); node->add(right);
            left = node;
        }
        return left;
    }
    ASTNodePtr parseAddSub() {
        auto left = parseMulDiv();
        while (check(TokenType::OP_PLUS) || check(TokenType::OP_MINUS)) {
            Token op  = advance();
            auto right = parseMulDiv();
            auto node  = std::make_shared<ASTNode>(NodeType::EXPR_BINARY, op);
            node->sval = op.value;
            node->add(left); node->add(right);
            left = node;
        }
        return left;
    }
    ASTNodePtr parseMulDiv() {
        auto left = parseUnary();
        while (check(TokenType::OP_MUL) || check(TokenType::OP_DIV) || check(TokenType::OP_MOD)) {
            Token op  = advance();
            auto right = parseUnary();
            auto node  = std::make_shared<ASTNode>(NodeType::EXPR_BINARY, op);
            node->sval = op.value;
            node->add(left); node->add(right);
            left = node;
        }
        return left;
    }
    ASTNodePtr parseUnary() {
        if (check(TokenType::OP_MINUS)) {
            Token op = advance();
            auto node  = std::make_shared<ASTNode>(NodeType::EXPR_UNARY, op);
            node->sval = "-";
            node->add(parsePrimary());
            return node;
        }
        return parsePrimary();
    }
    ASTNodePtr parsePrimary() {
        Token t = cur();
        if (t.type == TokenType::LIT_INT) {
            advance();
            auto n = std::make_shared<ASTNode>(NodeType::EXPR_LITERAL, t);
            n->sval = t.value;
            return n;
        }
        if (t.type == TokenType::LIT_FLOAT) {
            advance();
            auto n = std::make_shared<ASTNode>(NodeType::EXPR_LITERAL, t);
            n->sval = t.value;
            return n;
        }
        if (t.type == TokenType::LIT_STRING) {
            advance();
            auto n = std::make_shared<ASTNode>(NodeType::EXPR_LITERAL, t);
            n->sval = t.value;
            return n;
        }
        if (t.type == TokenType::IDENTIFIER) {
            advance();
            auto n = std::make_shared<ASTNode>(NodeType::EXPR_VAR, t);
            n->sval = t.value;
            return n;
        }
        if (t.type == TokenType::LPAREN) {
            advance();
            auto inner = parseExpr();
            expect(TokenType::RPAREN, "Kurang ')'");
            return inner;
        }
        throw ParseError("Token tak terduga: '" + t.value + "'", t.line, t.col);
    }
};
