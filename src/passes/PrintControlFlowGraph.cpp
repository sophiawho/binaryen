//
// Prints the statements in .dot format. You can use http://www.graphviz.org/ to
// view .dot files.
//

#include <iomanip>
#include <memory>
#include <string>
#include <map>

#include "ir/module-utils.h"
#include "ir/utils.h"
#include "pass.h"
#include "wasm.h"

using namespace std;

namespace wasm {

struct PrintControlFlowGraph : public Pass {

  int nodeCounter = 1;
  bool functionPrinted = false;
  string funcName = "";
  map<string, int> labelMap;

  bool modifiesBinaryenIR() override { return false; }

  void printGraphEdges(Expression *lhs, Expression *rhs, bool isDotted) {
    // Node Counter
    if (lhs->nodeCounter == -1) {
      lhs->nodeCounter = nodeCounter++;
      cout << "\t" << lhs->nodeCounter << " [label = ";
      printExpression(lhs);
      cout << "];\n";
    } 
    if (rhs->nodeCounter == -1) {
      rhs->nodeCounter = nodeCounter++;
      cout << "\t" << rhs->nodeCounter << " [label = ";
      printExpression(rhs);
      cout << "];\n";
    }
    // If LHS or RHS -> nodeCounter == -1
    // Assign a count based on nodeCounter and increment nodeCounter
    // stdout: $nodeCounter [label = printExpression(e)];\n
    // Then instead of printExpression, print $nodeCounter
    cout << "\t";
    if (!functionPrinted) {
      cout << "\"" << funcName << "\"" << " -> ";
      functionPrinted = true;
    }
    cout << lhs->nodeCounter << " -> " << rhs->nodeCounter;
    if (isDotted) cout << " [style=dotted]";
    cout << ";\n";
  }

  void traverseExpression(Expression* e) {
    switch (e->_id) {
          case Expression::Id::BlockId: { // 1
            int size = static_cast<Block*>(e)->list.size();
            for (int i=0; i<size-1; i++) {
              Expression *lhs = static_cast<Block*>(e)->list[i];
              Expression *rhs = static_cast<Block*>(e)->list[i+1];
              printGraphEdges(lhs, rhs, false);
            }
            cout << "\t// end of block \n";
            for (int i=0; i<size; i++) {
              Expression *curr = static_cast<Block*>(e)->list[i];
              traverseExpression(curr);
            }
            break;
          }
          case Expression::Id::LoopId: {
            Loop* loop = static_cast<Loop*>(e);
            traverseExpression(loop->body);
            break;
          }
          case Expression::Id::SwitchId: {
            // TODO
            break;
          }
          case Expression::Id::CallId: { // call printf
            Call* call = static_cast<Call*>(e);
            int size = call->operands.size();
            for (int i=0; i<size; i++) {
              Expression *rhs = call->operands[i];
              printGraphEdges(e, rhs, true);
              traverseExpression(rhs);
            }
            break;
          }
          case Expression::Id::LocalGetId: {
            break;
          }
          case Expression::Id::LocalSetId: {
            // These instructions get or set the values of variables, respectively. 
            // The 𝗅𝗈𝖼𝖺𝗅.𝗍𝖾𝖾 instruction is like 𝗅𝗈𝖼𝖺𝗅.𝗌𝖾𝗍 but also returns its argument.
            LocalSet* ls = static_cast<LocalSet*>(e);
            printGraphEdges(e, ls->value, true);
            traverseExpression(ls->value);
            break;
          }
          case Expression::Id::LoadId: {
            Load *load = static_cast<Load*>(e);
            printGraphEdges(e, load->ptr, true);
            traverseExpression(load->ptr);
            break;
          }
          case Expression::Id::StoreId: {
            Store *s = static_cast<Store*>(e);
            printGraphEdges(e, s->ptr, true);
            printGraphEdges(e, s->value, true);
            traverseExpression(s->ptr);
            traverseExpression(s->value);
            break;
          }
          case Expression::Id::ConstId: { // 14
            break;
          }
          case Expression::Id::BinaryId: { // 16
            Expression *left = static_cast<Binary*>(e)->left;
            Expression *right = static_cast<Binary*>(e)->right;
            printGraphEdges(e, left, true);
            printGraphEdges(e, right, true);
            traverseExpression(left);
            traverseExpression(right);
            break;
            }
          case Expression::Id::DropId: {
            printGraphEdges(e, static_cast<Drop*>(e)->value, true);
            traverseExpression(static_cast<Drop*>(e)->value);
            break;
          }
          default: 
            break;
        }
  }

  void printExpression(Expression* e) {
    cout << "\"";
    switch (e->_id) {
          case Expression::Id::BlockId: { // 1
            break;
          }
          case Expression::Id::IfId: {
            // If* ifExp = static_cast<If*>(e);
            cout << "if_expression";
          }
          case Expression::Id::LoopId: {
            Loop* loop = static_cast<Loop*>(e);
            cout << "\n\nDEBUG: loop [name]: " << loop->name;
            // (loop $label$2 (result i32)
            // (br_if $label$2
            // (i32.lt_s
            //   (tee_local $0
            //   (i32.add
            //     (get_local $0)
            //     (i32.const 1)
            //   )
            //   )
            //   (tee_local $1
            //   (i32.add
            //     (get_local $1)
            //     (i32.const -1)
            //   )
            //   )
            // )
            // )
            // (br $label$0)
            break;
          }
          case Expression::Id::BreakId: {
            Break* breakExp = static_cast<Break*>(e);
            cout << "break to label: " << breakExp->name << "$end$";
            // label id
          }
          case Expression::Id::SwitchId: {
            cout << "switch";
            break;
          }
          case Expression::Id::CallId: { // call printf
            Call* call = static_cast<Call*>(e);
            cout << "call_" << call->target;
            break;
          }
          case Expression::Id::LocalGetId: {
            cout << "local_get_idx_" << static_cast<LocalGet*>(e)->index;
            break;
          }
          case Expression::Id::LocalSetId: {
            // These instructions get or set the values of variables, respectively. 
            // The 𝗅𝗈𝖼𝖺𝗅.𝗍𝖾𝖾 instruction is like 𝗅𝗈𝖼𝖺𝗅.𝗌𝖾𝗍 but also returns its argument.
            LocalSet* ls = static_cast<LocalSet*>(e);
            if (!ls->isTee()) {
              cout << "local_set";
            } else {
              cout << "local_tee";
            }
            break;
          }
          case Expression::Id::LoadId: {
            Load *load = static_cast<Load*>(e);
            cout << "load_with_offset_" << load->offset.addr;
            break;
          }
          case Expression::Id::StoreId: {
            Store *s = static_cast<Store*>(e);
            cout << "store_with_offset_" << s->offset.addr;
            break;
          }
          case Expression::Id::ConstId: { // 14
            Literal l = getSingleLiteralFromConstExpression(e);
            // Print out literal
            if (l.type == Type::i32) {
              cout << "constant_val_" << l.geti32();
            } else if (l.type == Type::i64) {
              cout << "constant_val_" << l.geti64();
            }
            break;
          }
          case Expression::Id::BinaryId: { // 16
            BinaryOp op = static_cast<Binary*>(e)->op;
            cout << "binop_" << op; // 0 = add, 1 = sub
            break;
            }
          case Expression::Id::DropId: {
            cout << "drop";
            break;
          }
          default: 
            cout << "no_case_for_exp_type_" << int(e->_id);
        }
    cout << "\"";
  }

  void run(PassRunner* runner, Module* module) override {
    ostream& o = cout;
    o << "digraph G {\n";

    // Iterate through each function
    for (auto& curr : module->functions) {
        cout << "\n\t// begin function\n";
        funcName.clear();
        funcName += "function_";
        funcName += curr->name.str;
        cout << "\t\"" << funcName << "\" [shape=Mdiamond];\n";
        // Visit expression
        if (!curr->body) {
            continue;
        }
        functionPrinted = false;
        // std::cout << "\t\"" << funcName << "\" -> ";
        traverseExpression(curr->body);
    }

    o << "}\n";
  }
};

Pass* createPrintControlFlowGraph() { return new PrintControlFlowGraph(); }

} // namespace wasm
