#pragma once
#include "ast.h"
#include <string>
#include <sstream>

// ============================================================
//  C++ CODE GENERATOR
//  Menghasilkan C++ source dari AST.
//  Output bisa langsung dikompilasi dengan g++.
// ============================================================

class CppGenerator {
public:
    std::string generate(ASTNodePtr root) {
        std::ostringstream ss;
        ss << "#include <iostream>\n"
           << "#include <string>\n"
           << "using namespace std;\n\n"
           << "int main() {\n";
        indent = 1;
        genNode(root, ss);
        ss << "    return 0;\n"
           << "}\n";
        return ss.str();
    }

private:
    int indent = 0;

    std::string ind() {
        std::string s;
        for (int i = 0; i < indent; i++) s += "    ";
        return s;
    }

    std::string typeMap(const std::string& t) {
        if (t == "angka")    return "long long";
        if (t == "ngambang") return "float";
        if (t == "huruf")    return "string";
        return t;
    }

    void genNode(ASTNodePtr n, std::ostringstream& ss) {
        if (!n) return;

        switch (n->type) {
            case NodeType::PROGRAM:
                for (auto& c : n->children) genNode(c, ss);
                break;

            case NodeType::BLOCK:
                ss << " {\n";
                indent++;
                for (auto& c : n->children) genNode(c, ss);
                indent--;
                ss << ind() << "}";
                break;

            case NodeType::VAR_DECL: {
                std::string tipe = typeMap(n->sval);
                for (size_t i = 0; i < n->children.size(); i++) {
                    auto& v = n->children[i];
                    ss << ind() << tipe << " " << v->sval;
                    if (!v->children.empty()) {
                        ss << " = ";
                        genExpr(v->children[0], ss);
                    }
                    ss << ";\n";
                }
                break;
            }

            case NodeType::ASSIGN:
                ss << ind() << n->children[0]->sval << " = ";
                genExpr(n->children[1], ss);
                ss << ";\n";
                break;

            case NodeType::EXPR_STMT:
                ss << ind();
                genExpr(n->children[0], ss);
                ss << ";\n";
                break;

            case NodeType::IO_IN:
                ss << ind() << "cin";
                for (auto& v : n->children)
                    ss << " >> " << v->sval;
                ss << ";\n";
                break;

            case NodeType::IO_OUT:
                ss << ind() << "cout";
                for (auto& e : n->children) {
                    ss << " << ";
                    if (e->type == NodeType::EXPR_LITERAL && e->sval == "endl") {
                        ss << "endl";
                    } else {
                        genExpr(e, ss);
                    }
                }
                ss << ";\n";
                break;

            case NodeType::IF_STMT: {
                size_t nc = n->children.size();
                size_t i  = 0;
                bool first = true;
                while (i < nc) {
                    bool isElse = (nc - i == 1);
                    if (isElse) {
                        ss << " else";
                        // body harus block
                        if (n->children[i]->type == NodeType::BLOCK) {
                            genNode(n->children[i], ss);
                        } else {
                            ss << " {\n"; indent++;
                            genNode(n->children[i], ss);
                            indent--; ss << ind() << "}";
                        }
                        break;
                    }
                    if (first) { ss << ind() << "if ("; first = false; }
                    else       { ss << " else if ("; }
                    genExpr(n->children[i], ss);
                    ss << ")";
                    genNode(n->children[i+1], ss);
                    i += 2;
                }
                ss << "\n";
                break;
            }

            case NodeType::WHILE_STMT:
                ss << ind() << "while (";
                genExpr(n->children[0], ss);
                ss << ")";
                genNode(n->children[1], ss);
                ss << "\n";
                break;

            case NodeType::FOR_STMT:
                ss << ind() << "for (int ";
                ss << n->children[0]->sval << " = ";
                genExpr(n->children[1], ss);
                ss << "; ";
                genExpr(n->children[2], ss);
                ss << "; ";
                // step
                if (n->children[3]->type == NodeType::ASSIGN) {
                    ss << n->children[3]->children[0]->sval << " = ";
                    genExpr(n->children[3]->children[1], ss);
                } else if (n->children[3]->type == NodeType::INC_STMT) {
                    ss << n->children[3]->sval << "++";
                } else if (n->children[3]->type == NodeType::PLUS_ASSIGN_STMT) {
                    ss << n->children[3]->sval << " += ";
                    genExpr(n->children[3]->children[0], ss);
                } else if (n->children[3]->type == NodeType::DEC_STMT) {
                    ss << n->children[3]->sval << "--";
                } else if (n->children[3]->type == NodeType::MINUS_ASSIGN_STMT) {
                    ss << n->children[3]->sval << " -= ";
                    genExpr(n->children[3]->children[0], ss);
                } else {
                    genExpr(n->children[3], ss);
                }
                ss << ")";
                genNode(n->children[4], ss);
                ss << "\n";
                break;

            case NodeType::BREAK_STMT:
                ss << ind() << "break;\n";
                break;

            case NodeType::CONTINUE_STMT:
                ss << ind() << "continue;\n";
                break;

            default:
                for (auto& c : n->children) genNode(c, ss);
                break;
        }
    }

    void genExpr(ASTNodePtr n, std::ostringstream& ss) {
        if (!n) return;
        switch (n->type) {
            case NodeType::EXPR_LITERAL:
                if (n->sval == "endl") ss << "endl";
                else ss << n->sval;
                break;
            case NodeType::EXPR_VAR:
                ss << n->sval;
                break;
            case NodeType::EXPR_UNARY:
                ss << "-(";
                genExpr(n->children[0], ss);
                ss << ")";
                break;
            case NodeType::EXPR_BINARY:
                ss << "(";
                genExpr(n->children[0], ss);
                ss << " " << n->sval << " ";
                genExpr(n->children[1], ss);
                ss << ")";
                break;
            default:
                genNode(n, ss);
                break;
        }
    }
};
