//
// Visits the expressions in the IR to conduct dataflow analysis.
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

struct VisitorPattern : public Pass {

  // Helper for printing out graph nodes
  int nodeCounter = 1;
  bool hasPrefix = false;
  string prefix = "";
  string funcName = "";
  map<string, int> labelMap;
  set<int> printedBreakNodes;

  // Helper for reaching defs 
  map<int, Expression*> statementMap;
  
  bool modifiesBinaryenIR() override { return false; }

  void newNodeRDef(Expression *e) {
      if (e->_id == Expression::Id::LocalSetId) {
        LocalSet* ls = static_cast<LocalSet*>(e);
        // Check if there are statements to kill
        int localVar = ls->index;
        std::vector<int>::iterator iter = e->localrdefs.begin();
        while (iter != e->localrdefs.end()) {
            Expression *curExp = statementMap.at(*iter);
            int curVar = static_cast<LocalSet*>(curExp)->index;
            if (curVar == localVar) {
                iter = e->localrdefs.erase(iter);
            } else {
                ++iter;
            }
        }
        // Add to localrdefs
        e->localrdefs.push_back(e->nodeCounter);
    }
    // Print out reaching definitions
    if (e->localrdefs.empty()) return;
    cout << e->nodeCounter << ": <";
    for (std::vector<int>::const_iterator i = e->localrdefs.begin(); i != e->localrdefs.end(); ++i) {
        if (i+1 == e->localrdefs.end()) {
            std::cout << *i;
        } else {
            std::cout << *i << ',';
        }
    }
    cout << ">\n";
  }

  void analyseRDefs(Expression *e) {
      newNodeRDef(e);
      switch (e->_id) {
        case Expression::Id::BlockId: {
            int size = static_cast<Block*>(e)->list.size();
            for (int i=0; i<size; i++) {
                Expression *curr = static_cast<Block*>(e)->list[i];
                analyseRDefs(curr);
            }
            break;
        }
        case Expression::Id::LoopId: {
            Loop* loop = static_cast<Loop*>(e);
            analyseRDefs(loop->body);
            break;
          }
          case Expression::Id::BreakId: {
            Break* breakExp = static_cast<Break*>(e);
            if (breakExp->condition == NULL) return;
            analyseRDefs(breakExp->condition);
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
              analyseRDefs(rhs);
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
            analyseRDefs(ls->value);
            break;
          }
          case Expression::Id::LoadId: {
            Load *load = static_cast<Load*>(e);
            analyseRDefs(load->ptr);
            break;
          }
          case Expression::Id::StoreId: {
            Store *s = static_cast<Store*>(e);
            analyseRDefs(s->ptr);
            analyseRDefs(s->value);
            break;
          }
          case Expression::Id::ConstId: { // 14
            break;
          }
          case Expression::Id::BinaryId: { // 16
            Expression *left = static_cast<Binary*>(e)->left;   
            Expression *right = static_cast<Binary*>(e)->right;
            analyseRDefs(left);
            analyseRDefs(right);
            break;
            }
          case Expression::Id::DropId: {
            analyseRDefs(static_cast<Drop*>(e)->value);
            break;
          }
          default: 
            break;
        }
      }

  void visitModule(Module* module) {
    cout << "\n\n================Reaching Definition Analysis=======================\n";
    for (auto& curr : module->functions) {
        funcName.clear();
        funcName += "\"function_";
        funcName += curr->name.str;
        cout << "\nFunction " << funcName << "\":\n";
        // Visit expression
        if (!curr->body) {
            continue;
        }
        analyseRDefs(curr->body);
    }
  }

  // Code from PrintControlFlowGraph.cpp

  // If first time encountering node, print out label
  void newNode(Expression *e){
    e->nodeCounter = nodeCounter++;
    cout << "\t" << e->nodeCounter << " [label = ";
    printExpression(e);
    cout << "];\n";
    statementMap.insert(pair<int, wasm::Expression*>(e->nodeCounter, e));
  }

  void printGraphEdges(Expression *lhs, Expression *rhs, int style) {
    // If new node, print out unique node count and label
    if (lhs->nodeCounter == -1) {
      newNode(lhs);
    } 
    if (rhs->nodeCounter == -1) {
      newNode(rhs);
    }
    // Print out graph edge
    cout << "\t";
    if (hasPrefix) {
      cout << prefix << " -> ";
      hasPrefix = false;
      prefix.clear();
    }
    cout << lhs->nodeCounter << " -> " << rhs->nodeCounter;
    switch (style) {
      case 1:
        cout << " [style=dotted]"; // for child of stack expressions
        break;
      case 2:
        cout << " [color = green, label = \"condition\"]"; // for branching conditions
        break;
      default:
        break;
    }
    cout << ";\n";
    checkBreakExpression(lhs);
    checkBreakExpression(rhs);
  }

  void checkBreakExpression(Expression *e) {
    if (e->_id != Expression::Id::BreakId) return;
    Break* breakExp = static_cast<Break*>(e);
    string key = funcName;
    key.append(breakExp->name.str);
    int labelNode = labelMap.at(key);
    // If break expression points to itself or has been previously printed
    if (labelNode == e->nodeCounter || printedBreakNodes.count(e->nodeCounter) != 0) return;
    printedBreakNodes.insert(e->nodeCounter);
    cout << "\t" << e->nodeCounter << " -> " << labelNode << "; // debug break\n";
  }

  void traverseExpression(Expression* e) {
    switch (e->_id) {
          case Expression::Id::BlockId: { // 1
            hasPrefix = true;
            if (prefix.length() == 0) { // Prefix is either a funcName or expressionNode->nodeCounter
              prefix.append(to_string(e->nodeCounter));
            }
            int size = static_cast<Block*>(e)->list.size();
            for (int i=0; i<size-1; i++) {
              Expression *lhs = static_cast<Block*>(e)->list[i];
              Expression *rhs = static_cast<Block*>(e)->list[i+1];
              printGraphEdges(lhs, rhs, 0);
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
            loop->body->nodeCounter = e->nodeCounter; // Assign loop body same unique node counter as loop
            traverseExpression(loop->body);
            break;
          }
          case Expression::Id::BreakId: {
            Break* breakExp = static_cast<Break*>(e);
            if (breakExp->condition == NULL) return;
            printGraphEdges(breakExp, breakExp->condition, 2);
            traverseExpression(breakExp->condition);
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
              printGraphEdges(e, rhs, 1);
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
            printGraphEdges(e, ls->value, 1);
            traverseExpression(ls->value);
            break;
          }
          case Expression::Id::LoadId: {
            Load *load = static_cast<Load*>(e);
            printGraphEdges(e, load->ptr, 1);
            traverseExpression(load->ptr);
            break;
          }
          case Expression::Id::StoreId: {
            Store *s = static_cast<Store*>(e);
            printGraphEdges(e, s->ptr, 1);
            printGraphEdges(e, s->value, 1);
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
            printGraphEdges(e, left, 1);
            printGraphEdges(e, right, 1);
            traverseExpression(left);
            traverseExpression(right);
            break;
            }
          case Expression::Id::DropId: {
            printGraphEdges(e, static_cast<Drop*>(e)->value, 1);
            traverseExpression(static_cast<Drop*>(e)->value);
            break;
          }
          default: 
            break;
        }
  }

  void printExpression(Expression* e) {
    cout << "\"" << e->nodeCounter << ": ";
    switch (e->_id) {
          case Expression::Id::BlockId: { // 1
            Block* block = static_cast<Block*>(e);
            cout << "block_" << block->name;
            string key = funcName;
            key.append(block->name.str);
            labelMap.insert(pair<string, int>(key, e->nodeCounter));
            break;
          }
          case Expression::Id::IfId: {
            // If* ifExp = static_cast<If*>(e);
            cout << "if_expression";
          }
          case Expression::Id::LoopId: {
            Loop* loop = static_cast<Loop*>(e);
            cout << "loop_" << loop->name;
            string key = funcName;
            key.append(loop->name.str);
            labelMap.insert(pair<string, int>(key, e->nodeCounter));
            break;
          }
          case Expression::Id::BreakId: {
            Break* breakExp = static_cast<Break*>(e);
            // Branch instructions come in several flavors: 
            // 𝖻𝗋 performs an unconditional branch, 𝖻𝗋_𝗂𝖿 performs a conditional branch
            if (breakExp->condition != NULL) {
              cout << "conditional_";
            } else {
              cout << "unconditional_";
            }
            cout << "break_to_" << breakExp->name;
            break;
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
            cout << "_idx_" << static_cast<LocalSet*>(e)->index; // TODO MOVE TO PRINT CTRL FLOW GRAPH
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
        funcName += "\"function_";
        funcName += curr->name.str;
        funcName += "\"";
        prefix = funcName;
        cout << "\t" << funcName << " [shape=Mdiamond];\n";
        // Visit expression
        if (!curr->body) {
            continue;
        }
        traverseExpression(curr->body);
    }

    o << "}\n";

    visitModule(module);
  }
};

Pass* runVisitorPattern() { return new VisitorPattern(); }

} // namespace wasm
