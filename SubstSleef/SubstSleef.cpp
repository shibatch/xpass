// USAGE : clang-10 -Xclang -load -Xclang libSubstSleef.so -fno-math-errno -O3 example.c
// USAGE : opt-10 -load-pass-plugin=libSubstSleef.so -passes="subst-sleef" -S in.ll -o out.ll

#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
using namespace llvm;

#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
using namespace std;

namespace {
static unordered_map<string, string> substMap({
  { "llvm.sin.v4f32" , "Sleef_sinf4_u10" },
  { "llvm.sin.v8f32" , "Sleef_sinf8_u10" },
  { "llvm.sin.v16f32", "Sleef_sinf16_u10" },
  { "llvm.sin.v2f64" , "Sleef_sind2_u10" },
  { "llvm.sin.v4f64" , "Sleef_sind4_u10" },
  { "llvm.sin.v8f64" , "Sleef_sind8_u10" },

  { "llvm.cos.v4f32" , "Sleef_cosf4_u10" },
  { "llvm.cos.v8f32" , "Sleef_cosf8_u10" },
  { "llvm.cos.v16f32", "Sleef_cosf16_u10" },
  { "llvm.cos.v2f64" , "Sleef_cosd2_u10" },
  { "llvm.cos.v4f64" , "Sleef_cosd4_u10" },
  { "llvm.cos.v8f64" , "Sleef_cosd8_u10" },

  { "llvm.exp.v4f32" , "Sleef_expf4_u10" },
  { "llvm.exp.v8f32" , "Sleef_expf8_u10" },
  { "llvm.exp.v16f32", "Sleef_expf16_u10" },
  { "llvm.exp.v2f64" , "Sleef_expd2_u10" },
  { "llvm.exp.v4f64" , "Sleef_expd4_u10" },
  { "llvm.exp.v8f64" , "Sleef_expd8_u10" },

  { "llvm.exp2.v4f32" , "Sleef_exp2f4_u10" },
  { "llvm.exp2.v8f32" , "Sleef_exp2f8_u10" },
  { "llvm.exp2.v16f32", "Sleef_exp2f16_u10" },
  { "llvm.exp2.v2f64" , "Sleef_exp2d2_u10" },
  { "llvm.exp2.v4f64" , "Sleef_exp2d4_u10" },
  { "llvm.exp2.v8f64" , "Sleef_exp2d8_u10" },

  { "llvm.log.v4f32" , "Sleef_logf4_u10" },
  { "llvm.log.v8f32" , "Sleef_logf8_u10" },
  { "llvm.log.v16f32", "Sleef_logf16_u10" },
  { "llvm.log.v2f64" , "Sleef_logd2_u10" },
  { "llvm.log.v4f64" , "Sleef_logd4_u10" },
  { "llvm.log.v8f64" , "Sleef_logd8_u10" },

  { "llvm.log2.v4f32" , "Sleef_log2f4_u10" },
  { "llvm.log2.v8f32" , "Sleef_log2f8_u10" },
  { "llvm.log2.v16f32", "Sleef_log2f16_u10" },
  { "llvm.log2.v2f64" , "Sleef_log2d2_u10" },
  { "llvm.log2.v4f64" , "Sleef_log2d4_u10" },
  { "llvm.log2.v8f64" , "Sleef_log2d8_u10" },

  { "llvm.log10.v4f32" , "Sleef_log10f4_u10" },
  { "llvm.log10.v8f32" , "Sleef_log10f8_u10" },
  { "llvm.log10.v16f32", "Sleef_log10f16_u10" },
  { "llvm.log10.v2f64" , "Sleef_log10d2_u10" },
  { "llvm.log10.v4f64" , "Sleef_log10d4_u10" },
  { "llvm.log10.v8f64" , "Sleef_log10d8_u10" },

  { "llvm.pow.v4f32" , "Sleef_powf4_u10" },
  { "llvm.pow.v8f32" , "Sleef_powf8_u10" },
  { "llvm.pow.v16f32", "Sleef_powf16_u10" },
  { "llvm.pow.v2f64" , "Sleef_powd2_u10" },
  { "llvm.pow.v4f64" , "Sleef_powd4_u10" },
  { "llvm.pow.v8f64" , "Sleef_powd8_u10" },
});

static unordered_map<string, string> substMapFast({
  { "llvm.sin.v4f32" , "Sleef_sinf4_u35" },
  { "llvm.sin.v8f32" , "Sleef_sinf8_u35" },
  { "llvm.sin.v16f32", "Sleef_sinf16_u35" },
  { "llvm.sin.v2f64" , "Sleef_sind2_u35" },
  { "llvm.sin.v4f64" , "Sleef_sind4_u35" },
  { "llvm.sin.v8f64" , "Sleef_sind8_u35" },

  { "llvm.cos.v4f32" , "Sleef_cosf4_u35" },
  { "llvm.cos.v8f32" , "Sleef_cosf8_u35" },
  { "llvm.cos.v16f32", "Sleef_cosf16_u35" },
  { "llvm.cos.v2f64" , "Sleef_cosd2_u35" },
  { "llvm.cos.v4f64" , "Sleef_cosd4_u35" },
  { "llvm.cos.v8f64" , "Sleef_cosd8_u35" },

  { "llvm.exp.v4f32" , "Sleef_expf4_u35" },
  { "llvm.exp.v8f32" , "Sleef_expf8_u35" },
  { "llvm.exp.v16f32", "Sleef_expf16_u35" },
  { "llvm.exp.v2f64" , "Sleef_expd2_u35" },
  { "llvm.exp.v4f64" , "Sleef_expd4_u35" },
  { "llvm.exp.v8f64" , "Sleef_expd8_u35" },

  { "llvm.exp2.v4f32" , "Sleef_exp2f4_u35" },
  { "llvm.exp2.v8f32" , "Sleef_exp2f8_u35" },
  { "llvm.exp2.v16f32", "Sleef_exp2f16_u35" },
  { "llvm.exp2.v2f64" , "Sleef_exp2d2_u35" },
  { "llvm.exp2.v4f64" , "Sleef_exp2d4_u35" },
  { "llvm.exp2.v8f64" , "Sleef_exp2d8_u35" },

  { "llvm.log.v4f32" , "Sleef_logf4_u35" },
  { "llvm.log.v8f32" , "Sleef_logf8_u35" },
  { "llvm.log.v16f32", "Sleef_logf16_u35" },
  { "llvm.log.v2f64" , "Sleef_logd2_u35" },
  { "llvm.log.v4f64" , "Sleef_logd4_u35" },
  { "llvm.log.v8f64" , "Sleef_logd8_u35" },

  { "llvm.log2.v4f32" , "Sleef_log2f4_u35" },
  { "llvm.log2.v8f32" , "Sleef_log2f8_u35" },
  { "llvm.log2.v16f32", "Sleef_log2f16_u35" },
  { "llvm.log2.v2f64" , "Sleef_log2d2_u35" },
  { "llvm.log2.v4f64" , "Sleef_log2d4_u35" },
  { "llvm.log2.v8f64" , "Sleef_log2d8_u35" },

  { "llvm.log10.v4f32" , "Sleef_log10f4_u35" },
  { "llvm.log10.v8f32" , "Sleef_log10f8_u35" },
  { "llvm.log10.v16f32", "Sleef_log10f16_u35" },
  { "llvm.log10.v2f64" , "Sleef_log10d2_u35" },
  { "llvm.log10.v4f64" , "Sleef_log10d4_u35" },
  { "llvm.log10.v8f64" , "Sleef_log10d8_u35" },

  { "llvm.pow.v4f32" , "Sleef_powf4_u10" },
  { "llvm.pow.v8f32" , "Sleef_powf8_u10" },
  { "llvm.pow.v16f32", "Sleef_powf16_u10" },
  { "llvm.pow.v2f64" , "Sleef_powd2_u10" },
  { "llvm.pow.v4f64" , "Sleef_powd4_u10" },
  { "llvm.pow.v8f64" , "Sleef_powd8_u10" },
});

struct SubstSleef : public FunctionPass {
  static char ID;
  
  SubstSleef() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override {
    if (F.isDeclaration()) return false;
    bool changed = false;

    for (auto &BB : F) {
      for (auto inst = BB.begin(); inst != BB.end(); inst++) {
	auto *callInst = dyn_cast<CallInst>(inst);
	if (callInst) {
	  unordered_map<string, string> *map = inst->getFastMathFlags().approxFunc() ? &substMapFast : &substMap;
	  Function *cf = callInst->getCalledFunction();
	  if (cf && map->count(cf->getName())) {
	    FunctionCallee c = BB.getModule()->getOrInsertFunction(map->at(cf->getName()),
								   callInst->getCalledFunction()->getFunctionType(),
								   callInst->getCalledFunction()->getAttributes());
	    callInst->setCalledFunction(c);
	    changed = true;
	  }
	}
      }
    }

    return changed;
  }
}; // struct SubstSleef
}  // namespace

char SubstSleef::ID = 0;
static RegisterPass<SubstSleef> X("Xsubst-sleef", "Substitute calls to math intrinsic functions with those to SLEEF",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);

static RegisterStandardPasses Y(
    PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder &Builder,
       legacy::PassManagerBase &PM) { PM.add(new SubstSleef()); });

//

static void loadPass(const PassManagerBuilder &Builder, legacy::PassManagerBase &PM) {
  PM.add(new SubstSleef());
}

static RegisterStandardPasses clangtoolLoader_Ox(PassManagerBuilder::EP_OptimizerLast, loadPass);
