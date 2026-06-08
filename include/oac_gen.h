#pragma once
#include "ast.h"
#include <string>
#include <vector>
#include <sstream>

// ============================================================
//  ONE-ADDRESS CODE (OAC) — arsitektur akumulator
//
//  Satu-satunya operand eksplisit adalah alamat/literal.
//  Register implisit: ACC (accumulator)
//
//  Instruksi yang didukung:
//  ─────────────────────────────────────────────────────────
//  LOAD  <addr>     ACC = mem[addr]
//  LOADI <val>      ACC = val (literal)
//  STORE <addr>     mem[addr] = ACC
//  ADD   <addr>     ACC = ACC + mem[addr]
//  SUB   <addr>     ACC = ACC - mem[addr]
//  MUL   <addr>     ACC = ACC * mem[addr]
//  DIV   <addr>     ACC = ACC / mem[addr]
//  MOD   <addr>     ACC = ACC % mem[addr]
//  CMP   <addr>     compare ACC dengan mem[addr] → set flag
//  CMPI  <val>      compare ACC dengan literal
//  JMP   <label>    jump tak bersyarat
//  JEQ   <label>    jump jika flag EQ
//  JNE   <label>    jump jika flag NE
//  JLT   <label>    jump jika flag LT
//  JGT   <label>    jump jika flag GT
//  JLE   <label>    jump jika flag LE
//  JGE   <label>    jump jika flag GE
//  IN    <addr>     baca input → mem[addr]
//  OUT   <addr>     cetak mem[addr]
//  OUTI  <val>      cetak literal
//  OUTNL            cetak newline
//  LABEL <name>     titik lompat
//  HALT             selesai
// ============================================================

struct OACInstr {
    std::string op;
    std::string arg;
    std::string comment;

    std::string toString() const {
        std::string s = "    " + op;
        if (!arg.empty()) s += " " + arg;
        if (!comment.empty()) s += "    ; " + comment;
        return s;
    }
};

class OACGenerator {
public:
    std::vector<OACInstr> generate(ASTNodePtr root) {
        instrs.clear();
        tempCount  = 0;
        labelCount = 0;
        emit("LABEL", "main", "entry point");
        genNode(root);
        emit("HALT", "", "end of program");
        return instrs;
    }

    std::string listing() const {
        std::ostringstream ss;
        ss << "; ============================================\n";
        ss << "; One-Address Code — Bahasa Suka Suka\n";
        ss << "; ACC = implicit accumulator register\n";
        ss << "; ============================================\n\n";
        for (auto& ins : instrs) {
            if (ins.op == "LABEL")
                ss << ins.arg << ":\n";
            else
                ss << ins.toString() << "\n";
        }
        return ss.str();
    }

private:
    std::vector<OACInstr> instrs;
    int tempCount  = 0;
    int labelCount = 0;

    void emit(const std::string& op, const std::string& arg = "",
              const std::string& cmt = "") {
        instrs.push_back({op, arg, cmt});
    }

    std::string newTemp() {
        return "_t" + std::to_string(tempCount++);
    }
    std::string newLabel(const std::string& prefix = "L") {
        return prefix + std::to_string(labelCount++);
    }

    // ── Kode generasi ekspresi → hasilnya di ACC ──────────────
    // Mengembalikan nama temp jika perlu disimpan
    std::string genExpr(ASTNodePtr n) {
        if (!n) return "";

        switch (n->type) {
            case NodeType::EXPR_LITERAL: {
                if (n->sval == "endl") return "__endl__";
                emit("LOADI", quoteLiteral(n->tok), "literal " + n->sval);
                return "";
            }
            case NodeType::EXPR_VAR: {
                emit("LOAD", n->sval, "load " + n->sval);
                return "";
            }
            case NodeType::EXPR_UNARY: {
                genExpr(n->children[0]);
                std::string t = newTemp();
                emit("STORE", t, "save for negate");
                emit("LOADI", "0");
                emit("SUB",   t,  "negate");
                return "";
            }
            case NodeType::EXPR_BINARY: {
                // Evaluasi kiri → simpan ke temp
                genExpr(n->children[0]);
                std::string lt = newTemp();
                emit("STORE", lt, "save left operand");

                // Evaluasi kanan → ACC
                genExpr(n->children[1]);
                std::string rt = newTemp();
                emit("STORE", rt, "save right operand");

                // Operasi dengan kanan di ACC sudah di rt
                emit("LOAD", lt, "restore left to ACC");

                std::string op = n->sval;
                if      (op == "+")  emit("ADD", rt);
                else if (op == "-")  emit("SUB", rt);
                else if (op == "*")  emit("MUL", rt);
                else if (op == "/")  emit("DIV", rt);
                else if (op == "%")  emit("MOD", rt);
                // Perbandingan & logika — hasilkan 1/0 di ACC
                else if (op == "==" || op == "!=" ||
                         op == "<"  || op == ">"  ||
                         op == "<=" || op == ">=") {
                    emit("CMP", rt, "compare");
                    std::string trueL  = newLabel("_cmp_t");
                    std::string doneL  = newLabel("_cmp_d");
                    std::string jmpOp  = cmpJump(op);
                    emit(jmpOp,  trueL);
                    emit("LOADI", "0", "false");
                    emit("JMP",   doneL);
                    emit("LABEL", trueL);
                    emit("LOADI", "1", "true");
                    emit("LABEL", doneL);
                }
                else if (op == "&&") {
                    // lt && rt — kedua sudah di temp
                    // short-circuit: if lt==0 → false
                    std::string falseL = newLabel("_and_f");
                    std::string doneL  = newLabel("_and_d");
                    emit("LOAD",  lt);
                    emit("CMPI",  "0");
                    emit("JEQ",   falseL, "short-circuit AND");
                    emit("LOAD",  rt);
                    emit("CMPI",  "0");
                    emit("JEQ",   falseL);
                    emit("LOADI", "1");
                    emit("JMP",   doneL);
                    emit("LABEL", falseL);
                    emit("LOADI", "0");
                    emit("LABEL", doneL);
                }
                else if (op == "||") {
                    std::string trueL  = newLabel("_or_t");
                    std::string doneL  = newLabel("_or_d");
                    emit("LOAD",  lt);
                    emit("CMPI",  "0");
                    emit("JNE",   trueL, "short-circuit OR");
                    emit("LOAD",  rt);
                    emit("CMPI",  "0");
                    emit("JNE",   trueL);
                    emit("LOADI", "0");
                    emit("JMP",   doneL);
                    emit("LABEL", trueL);
                    emit("LOADI", "1");
                    emit("LABEL", doneL);
                }
                return "";
            }
            default: return "";
        }
    }

    // ── Kode generasi statement ────────────────────────────────
    void genNode(ASTNodePtr n) {
        if (!n) return;

        switch (n->type) {
            case NodeType::PROGRAM:
            case NodeType::BLOCK:
                for (auto& c : n->children) genNode(c);
                break;

            case NodeType::VAR_DECL:
                for (auto& varNode : n->children) {
                    // Deklarasi saja (tanpa init) → inisialisasi ke 0/"" secara implisit
                    if (varNode->children.empty()) {
                        emit("LOADI", "0", "default init " + varNode->sval);
                        emit("STORE", varNode->sval);
                    } else {
                        genExpr(varNode->children[0]);
                        emit("STORE", varNode->sval, "init " + varNode->sval);
                    }
                }
                break;

            case NodeType::ASSIGN:
                genExpr(n->children[1]);
                emit("STORE", n->children[0]->sval, "assign to " + n->children[0]->sval);
                break;

            case NodeType::INC_STMT:
                emit("LOAD",  n->sval, "load for ++");
                emit("PUSHI", "1",     "increment by 1");
                emit("ADD",   "",      "i++");
                emit("STORE", n->sval, "store i++");
                break;

            case NodeType::PLUS_ASSIGN_STMT:
                emit("LOAD",  n->sval, "load for +=");
                genExpr(n->children[0]);
                emit("ADD",   "",      "+=");
                emit("STORE", n->sval, "store +=");
                break;

            

            case NodeType::EXPR_STMT:
                genExpr(n->children[0]);
                break;

            case NodeType::IO_IN:
                for (auto& v : n->children) {
                    emit("IN", v->sval, "read into " + v->sval);
                }
                break;

            case NodeType::IO_OUT:
                for (auto& e : n->children) {
                    if (e->type == NodeType::EXPR_LITERAL && e->sval == "endl") {
                        emit("OUTNL", "", "newline");
                    } else if (e->type == NodeType::EXPR_LITERAL) {
                        emit("OUTI", quoteLiteral(e->tok), "print literal");
                    } else if (e->type == NodeType::EXPR_VAR) {
                        emit("OUT", e->sval, "print " + e->sval);
                    } else {
                        genExpr(e);
                        std::string t = newTemp();
                        emit("STORE", t);
                        emit("OUT", t, "print expr result");
                    }
                }
                break;

            case NodeType::IF_STMT: {
                // children: cond, body, [cond, body, ...], [else_body]
                // Tentukan apakah anak terakhir adalah else (tanpa pasangan kondisi)
                size_t n_children = n->children.size();
                std::string doneL = newLabel("_if_done");
                std::vector<std::string> nextLabels;

                size_t i = 0;
                while (i < n_children) {
                    // Cek: apakah ini else (sisa satu anak)?
                    bool isElse = (n_children - i == 1);
                    if (isElse) {
                        genNode(n->children[i]);
                        break;
                    }
                    // if / else-if: dua anak (cond, body)
                    std::string failL = newLabel("_if_fail");
                    genExpr(n->children[i]);
                    emit("CMPI", "0", "test condition");
                    emit("JEQ",  failL, "jump if false");
                    genNode(n->children[i+1]);
                    emit("JMP",  doneL, "skip remaining branches");
                    emit("LABEL", failL);
                    i += 2;
                }
                emit("LABEL", doneL);
                break;
            }

            case NodeType::WHILE_STMT: {
                std::string loopL = newLabel("_while");
                std::string doneL = newLabel("_wdone");
                breakLabel    = doneL;
                continueLabel = loopL;

                emit("LABEL", loopL, "while loop");
                genExpr(n->children[0]);
                emit("CMPI", "0", "test condition");
                emit("JEQ",  doneL, "exit if false");
                genNode(n->children[1]);
                emit("JMP",  loopL, "repeat");
                emit("LABEL", doneL);
                breakLabel = continueLabel = "";
                break;
            }

            case NodeType::FOR_STMT: {
                // children: var, init, cond, step, body
                std::string loopL = newLabel("_for");
                std::string stepL = newLabel("_for_step");
                std::string doneL = newLabel("_for_done");
                breakLabel    = doneL;
                continueLabel = stepL;

                // Init
                genExpr(n->children[1]);
                emit("STORE", n->children[0]->sval, "for init");

                emit("LABEL", loopL, "for loop");
                genExpr(n->children[2]);
                emit("CMPI", "0", "for cond");
                emit("JEQ",  doneL);
                genNode(n->children[4]);  // body

                emit("LABEL", stepL);
                genNode(n->children[3]);  // step
                emit("JMP",  loopL);
                emit("LABEL", doneL);
                breakLabel = continueLabel = "";
                break;
            }

            case NodeType::BREAK_STMT:
                if (!breakLabel.empty())
                    emit("JMP", breakLabel, "break");
                break;

            case NodeType::CONTINUE_STMT:
                if (!continueLabel.empty())
                    emit("JMP", continueLabel, "continue");
                break;

            default:
                for (auto& c : n->children) genNode(c);
                break;
        }
    }

    std::string cmpJump(const std::string& op) {
        if (op == "==") return "JEQ";
        if (op == "!=") return "JNE";
        if (op == "<")  return "JLT";
        if (op == ">")  return "JGT";
        if (op == "<=") return "JLE";
        if (op == ">=") return "JGE";
        return "JMP";
    }

    std::string quoteLiteral(const Token& tok) {
        if (tok.type == TokenType::LIT_STRING) return tok.value;
        return tok.value;
    }

    std::string breakLabel;
    std::string continueLabel;
};
