//
// Prints the control flow graph in .dot format. You can use http://www.graphviz.org/ to
// view .dot files.
//

#include <iomanip>
#include <memory>

#include "ir/module-utils.h"
#include "ir/utils.h"
#include "pass.h"
#include "wasm.h"

namespace wasm {

struct PrintControlFlowGraph : public Pass {
  bool modifiesBinaryenIR() override { return false; }

  void printIndent(int size) {
    for (int i=0; i<size; i++) std::cout << "  ";
  }

  void printExpression(Expression* e, int indent) {
    printIndent(indent);
    switch (e->_id) {
          case Expression::Id::BlockId: { // 1
            // Possibly get block name
            int size = static_cast<Block*>(e)->list.size();
            std::cout << "Block expression: " << static_cast<Block*>(e)->name << " with block size: " << size << "\n";
            for (int i=0; i<size; i++) {
              Expression *currExp = static_cast<Block*>(e)->list[i];
              printExpression(currExp, indent + 1);
            }
            break;
          }
          case Expression::Id::LoopId: {
            std::cout << "Loop ID\n";
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
          case Expression::Id::SwitchId: {
            std::cout << "Switch ID\n";
            break;
          }
          case Expression::Id::CallId: { // call printf
            Call* call = static_cast<Call*>(e);
            std::cout << "Call function: " << call->target << "\n";
            int size = call->operands.size();
            for (int i=0; i<size; i++) {
              Expression *currExp = call->operands[i];
              printExpression(currExp, indent+1);
            }
            break;
          }
          case Expression::Id::LocalGetId: {
            std::cout << "Local get ID with index: " << static_cast<LocalGet*>(e)->index << "\n";
            break;
          }
          case Expression::Id::LocalSetId: {
            // These instructions get or set the values of variables, respectively. 
            // The 𝗅𝗈𝖼𝖺𝗅.𝗍𝖾𝖾 instruction is like 𝗅𝗈𝖼𝖺𝗅.𝗌𝖾𝗍 but also returns its argument.
            LocalSet* ls = static_cast<LocalSet*>(e);
            if (!ls->isTee()) {
              std::cout << "Local set ID\n";
            } else {
              std::cout << "Local tee ID\n";
            }
            printExpression(ls->value, indent+1);
            break;
          }
          case Expression::Id::LoadId: {
            Load *load = static_cast<Load*>(e);
            std::cout << "Load ID with offset: " << load->offset.addr << "\n";
            printExpression(load->ptr, indent+1);
            break;
          }
          case Expression::Id::StoreId: {
            Store *s = static_cast<Store*>(e);
            std::cout << "Store expression with offset: " << s->offset.addr << "\n";
            printExpression(s->ptr, indent+1);
            printExpression(s->value, indent+1);
            // (i32.store offset=4
            //   (i32.const 0)
            //   (tee_local $0
            //     (i32.sub
            //     (i32.load offset=4
            //       (i32.const 0)
            //     )
            //     (i32.const 16)
            //     )
            //   )
            // )
            break;
          }
          case Expression::Id::ConstId: { // 14
            Literal l = getSingleLiteralFromConstExpression(e);
            // Print out literal
            if (l.type == Type::i32) {
              std::cout << "Constant exprssion with literal value: " << l.geti32() << "\n";
            } else if (l.type == Type::i64) {
              std::cout << "Constant exprssion with literal value: " << l.geti64() << "\n";
            }
            break;
          }
          case Expression::Id::BinaryId: { // 16
            BinaryOp op = static_cast<Binary*>(e)->op;
            std::cout << "Binary expression with binop: " << op << "\n"; // 0 = add, 1 = sub
            Expression *left = static_cast<Binary*>(e)->left;
            Expression *right = static_cast<Binary*>(e)->right;
            printExpression(left, indent+1);
            printExpression(right, indent+1);
            break;
            }
          case Expression::Id::DropId: {
            std::cout << "Drop Expression\n";
            printExpression(static_cast<Drop*>(e)->value, indent+1);
            // Calling another function
            break;
          }
          default: 
            std::cout << "No case for expression type " << int(e->_id) << "\n";
        }
  }

  void run(PassRunner* runner, Module* module) override {
    std::ostream& o = std::cout;
    //  o << "Sophia\n";
    //  Sophia
    //     "0" [style="filled", fillcolor="white"];
    //     "0" [style="filled", fillcolor="gray"];
    //     }

    // .wat follows
    // (module
    // (table 0 anyfunc)
    // (memory $0 1)
    // (export "memory" (memory $0))
    // (export "main" (func $main))
    // (func $main (; 0 ;) (result i32)
    // (i32.const 3)
    // )
    // )

    // o << "digraph call {\n"
    //      "  rankdir = LR;\n"
    //      "  subgraph cluster_key {\n"
    //      "    node [shape=box, fontname=courier, fontsize=10];\n"
    //      "    edge [fontname=courier, fontsize=10];\n"
    //      "    label = \"Key\";\n"
    //      "    \"Import\" [style=\"filled\", fillcolor=\"turquoise\"];\n"
    //      "    \"Export\" [style=\"filled\", fillcolor=\"gray\"];\n"
    //      "    \"Indirect Target\" [style=\"filled, rounded\", "
    //      "fillcolor=\"white\"];\n"
    //      "    \"A\" -> \"B\" [style=\"filled, rounded\", label = \"Direct "
    //      "Call\"];\n"
    //      "  }\n\n"
    //      "  node [shape=box, fontname=courier, fontsize=10];\n";

    // Defined functions
    ModuleUtils::iterDefinedFunctions(*module, [&](Function* curr) {
      std::cout << "  \"" << curr->name
                << "\" [style=\"filled\", fillcolor=\"white\"];\n";
    });

    // Imported functions
    ModuleUtils::iterImportedFunctions(*module, [&](Function* curr) {
      o << "  \"" << curr->name
        << "\" [style=\"filled\", fillcolor=\"turquoise\"];\n";
    });

    // Iterate through each function
    for (auto& curr : module->functions) {
        std::cout << "Function name: " << curr->name << "\n";
        // Visit expression
        if (!curr->body) {
            std::cout << "Function does not have a body\n";
            continue;
        }
        printExpression(curr->body, 1);
    }

    // Exports
    for (auto& curr : module->exports) {
      if (curr->kind == ExternalKind::Function) {
        Function* func = module->getFunction(curr->value);
        o << "  \"" << func->name
          << "\" [style=\"filled\", fillcolor=\"gray\"];\n";
      }
    }

    struct CallPrinter : public PostWalker<CallPrinter> {
      Module* module;
      Function* currFunction;
      std::set<Name> visitedTargets; // Used to avoid printing duplicate edges.
      std::vector<Function*> allIndirectTargets;
      CallPrinter(Module* module) : module(module) {
        // Walk function bodies.
        ModuleUtils::iterDefinedFunctions(*module, [&](Function* curr) {
          currFunction = curr;
          visitedTargets.clear();
          walk(curr->body);
        });
      }
      // Direct Calls
      void visitCall(Call* curr) {
        auto* target = module->getFunction(curr->target);
        // Helper code to avoid printing duplicate edges
        if (visitedTargets.count(target->name) > 0) {
          return;
        }
        visitedTargets.insert(target->name);
        std::cout << "  \"" << currFunction->name << "\" -> \"" << target->name
                  << "\"; // call\n";
      }
    };
    CallPrinter printer(module);

    // Indirect Targets
    for (auto& segment : module->table.segments) {
      for (auto& curr : segment.data) {
        auto* func = module->getFunction(curr);
        o << "  \"" << func->name << "\" [style=\"filled, rounded\"];\n";
      }
    }

    o << "}\n";
  }
};

Pass* createPrintControlFlowGraph() { return new PrintControlFlowGraph(); }

} // namespace wasm
