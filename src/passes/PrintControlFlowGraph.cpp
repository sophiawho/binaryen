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

  void run(PassRunner* runner, Module* module) override {
    std::ostream& o = std::cout;
     o << "Sophia\n";
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
