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

  void printExpression(Expression* e) {
    switch (e->_id) {
          case Expression::Id::BlockId: {
            // Possibly get block name
            std::cout << "Block expression: " << static_cast<Block*>(e)->name << "\n";
            int size = static_cast<Block*>(e)->list.size();
            std::cout << size << "\n";
            for (int i=0; i<size; i++) {
              Expression *currExp = static_cast<Block*>(e)->list[i];
              printExpression(currExp);
            }
            break;
          }
          case Expression::Id::ConstId: {
            std::cout << "Constant expression\n";
            Literal l = getSingleLiteralFromConstExpression(e);
            // Print out literal
            if (l.type == Type::i32) {
              std::cout << "Literal value: " << l.geti32() << "\n";
            } else if (l.type == Type::i64) {
              std::cout << "Literal value: " << l.geti64() << "\n";
            }
            break;
          }
          case Expression::Id::BinaryId: {
            std::cout << "Binary expression\n";
            Expression *left = static_cast<Binary*>(e)->left;
            Expression *right = static_cast<Binary*>(e)->right;
            std::cout << int(left->_id) << "\n";
            std::cout << int(right->_id) << "\n";
            // Print out LocalGet 
            // class LocalGet : public SpecificExpression<Expression::LocalGetId> {
            // Index leftIndex = static_cast<LocalGet*>(left)->index;
            std::cout << "Left index: " << static_cast<LocalGet*>(left)->index << "\n";
            std::cout << "Right index: " << static_cast<LocalGet*>(right)->index << "\n";
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
        std::cout << " Function name: " << curr->name << "\n";
        // Visit expression
        if (!curr->body) {
            std::cout << "Function does not have a body\n";
            continue;
        }
        printExpression(curr->body);
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
