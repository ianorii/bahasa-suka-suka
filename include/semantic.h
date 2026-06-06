#pragma once
#include "ast.h"
#include <stdexcept>
#include <string>

// ============================================================
//  SEMANTIC ANALYZER
//
//  - Isi Symbol Table saat menemukan VAR_DECL
//  - Cek type compatibility pada assignment & operasi
//  - Cek identifier tidak undeclared
// ============================================================

class SemanticError : public std::runtime_error {
public:
    int line;
    SemanticError(const std::string& msg, int l = 0)
        : std::runtime_error(msg), line(l) {}
};

class SemanticAnalyzer {
public:
    void analyze(ASTNodePtr root, SymbolTable& sym) {
        this->sym = &sym;
        visitNode(root);
    }

private:
    SymbolTable* sym = nullptr;

    static bool isKeyword(const std::string& s) {
        static const std::vector<std::string> kw = {
            "angka","ngambang","huruf","kalau","ulang","untuk","trs","nah",
            "itu","tidak","lain","berhenti","lanjut","masukin","keluarin","endl","dan","atau"
        };
        for (auto& k : kw) if (k == s) return true;
        return false;
    }

    bool numerik(const std::string& t) {
        return t == "angka" || t == "ngambang";
    }

    bool compatible(const std::string& a, const std::string& b) {
        if (a.empty() || b.empty()) return true;
        if (numerik(a) && numerik(b)) return true;
        return a == b;
    }

    // Resolve tipe ekspresi
    std::string typeOf(ASTNodePtr n) {
        if (!n) return "";
        switch (n->type) {
            case NodeType::EXPR_LITERAL: {
                if (n->tok.type == TokenType::LIT_INT)    return "angka";
                if (n->tok.type == TokenType::LIT_FLOAT)  return "ngambang";
                if (n->tok.type == TokenType::LIT_STRING) return "huruf";
                if (n->sval == "endl")                    return "endl";
                return "";
            }
            case NodeType::EXPR_VAR: {
                if (sym->has(n->sval)) return sym->typeOf(n->sval);
                return "";
            }
            case NodeType::EXPR_BINARY: {
                std::string lt = typeOf(n->children[0]);
                std::string rt = typeOf(n->children[1]);
                // Perbandingan → bool (angka)
                if (n->sval == "==" || n->sval == "!=" ||
                    n->sval == "<"  || n->sval == ">"  ||
                    n->sval == "<=" || n->sval == ">=" ||
                    n->sval == "&&" || n->sval == "||")
                    return "angka";
                if (numerik(lt) && numerik(rt))
                    return (lt == "ngambang" || rt == "ngambang") ? "ngambang" : "angka";
                return lt;
            }
            case NodeType::EXPR_UNARY:
                return typeOf(n->children[0]);
            default:
                return "";
        }
    }

    void visitNode(ASTNodePtr n) {
        if (!n) return;

        switch (n->type) {
            case NodeType::VAR_DECL: {
                std::string tipe = n->sval;
                for (auto& varNode : n->children) {
                    std::string name = varNode->sval;
                    if (isKeyword(name))
                        throw SemanticError("Nama variabel '" + name + "' tidak boleh menggunakan kata kunci!", n->tok.line);
                    sym->declare(name, tipe, n->tok.line);

                    // Cek tipe inisialisasi
                    if (!varNode->children.empty()) {
                        auto& initExpr = varNode->children[0];
                        std::string initType = typeOf(initExpr);
                        if (!compatible(tipe, initType) && !initType.empty())
                            throw SemanticError("Beda tipe data! '" + name + "' bertipe " + tipe +
                                " tapi nilai awal bertipe " + initType, n->tok.line);
                        if (tipe == "angka" && initType == "ngambang")
                            throw SemanticError("Aduh bikin angka tapi isinya koma koma!", n->tok.line);
                    }
                }
                break;
            }

            case NodeType::ASSIGN: {
                auto& varNode = n->children[0];
                auto& valNode = n->children[1];
                std::string name = varNode->sval;

                if (!sym->has(name))
                    throw SemanticError("Variabel '" + name + "' belum dideklarasikan!", n->tok.line);

                std::string tipe     = sym->typeOf(name);
                std::string valType  = typeOf(valNode);
                if (!compatible(tipe, valType) && !valType.empty())
                    throw SemanticError("Ga bisa assign " + valType + " ke '" + name + "' (" + tipe + ")", n->tok.line);
                visitNode(valNode);
                break;
            }

            case NodeType::EXPR_VAR: {
                if (!sym->has(n->sval))
                    throw SemanticError("Variabel '" + n->sval + "' belum dideklarasikan!", n->tok.line);
                break;
            }

            case NodeType::EXPR_BINARY: {
                visitNode(n->children[0]);
                visitNode(n->children[1]);
                std::string lt = typeOf(n->children[0]);
                std::string rt = typeOf(n->children[1]);
                if (!compatible(lt, rt) && !lt.empty() && !rt.empty())
                    throw SemanticError("Beda tipe! " + lt + " dan " + rt + " tidak kompatibel", n->tok.line);
                break;
            }

            default:
                for (auto& child : n->children) visitNode(child);
                break;
        }
    }
};
