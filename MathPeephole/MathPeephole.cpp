// USAGE : clang-10 -Xclang -load -Xclang libMathPeephole.so -ffast-math -O3 example.c
// USAGE : opt-10 -load=libMathPeephole.so -O1 -ffast-math -lint -S in.ll -o out.ll

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
using namespace llvm;

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cassert>

//

static const int verbose = 0;

#define ENABLE_REDUCEFRAC
//#define DEBUG_REDUCEFRAC

#define ENABLE_CMPDIV
//#define DEBUG_CMPDIV

#define ENABLE_CMPZERO
//#define DEBUG_CMPZERO

//

using namespace std;

namespace {
struct RewriteRule {
  int nRewrite;
  virtual string name() { return ""; }
  virtual void clearStats() { nRewrite = 0; }
  virtual void clear() {
    // Clear the saved analysis result
    nRewrite = 0;
  }
  virtual void match(vector<Value *> &seq) {
    // Check the given sequence, and save the analysis result
  }
  virtual bool execute(BasicBlock &BB, unordered_set<Instruction *> &eraseList) {
    // Rewrite the IR based on the analysis result.
    // Returns true if something is changed.
    return false;
  }
};

//

struct AR_ReduceFraction {
  vector<Value *> seq = vector<Value *>();
};

struct ReduceFraction : public RewriteRule {
  // a/b + c/d -> (a*d + b*c) / b*d

  vector<AR_ReduceFraction> arlist = vector<AR_ReduceFraction>();
  unordered_map<Value *, int> countMap = unordered_map<Value *, int>();

  string name() { return "ReduceFraction"; }
  void clear() {
    arlist.clear();
    countMap.clear();
  }

  void match(vector<Value *> &seq) {
    // Matches FAdd+ FMul* FDiv
    if (seq.size() < 2) return;

    BinaryOperator *fdivOp = dyn_cast<BinaryOperator>(seq[seq.size()-1]);
    if (!fdivOp) return;
    if (fdivOp->getOpcode() != Instruction::FDiv) return;
    if (fdivOp->getOperand(0) == fdivOp->getOperand(1)) return;

    {
      ConstantFP *c = dyn_cast<ConstantFP>(fdivOp->getOperand(1));
      if (c && c->isZero()) return;
    }

    if (!fdivOp->getFastMathFlags().isFast()) return;

    int beginning;
    for(beginning = seq.size()-2;beginning >= 0;beginning--) {
      BinaryOperator *op = dyn_cast<BinaryOperator>(seq[beginning]);
      if (!op) break;
      if (op->getOpcode() == Instruction::FAdd ||
	  op->getOpcode() == Instruction::FSub) break;
      if (op->getOpcode() != Instruction::FMul) break;
    }

    for(;beginning >= 0;beginning--) {
      BinaryOperator *op = dyn_cast<BinaryOperator>(seq[beginning]);
      if (!op) break;
      if (op->getOpcode() != Instruction::FAdd &&
	  op->getOpcode() != Instruction::FSub) break;
    }

    beginning++;

    {
      BinaryOperator *op = dyn_cast<BinaryOperator>(seq[beginning]);
      if (!op || (op->getOpcode() != Instruction::FAdd &&
		  op->getOpcode() != Instruction::FSub)) return;
    }

    for(unsigned i = beginning+1;i<seq.size();i++) {
      if (seq[i]->getNumUses() != 1) return;
    }

    //

    AR_ReduceFraction ar;
    Value *atop = NULL;

    unsigned i;
    for(i=beginning;i<seq.size()-1;i++) {
      BinaryOperator *op = dyn_cast<BinaryOperator>(seq[i]);
      assert(op != NULL);
      if (op->getOpcode() == Instruction::FMul) break;

      if (atop == NULL) atop = op;
    }

    for(i=beginning;i<seq.size();i++) ar.seq.push_back(seq[i]);

#ifdef DEBUG_REDUCEFRAC
    errs() << "ReduceFraction match\n";
    for(i=beginning;i<seq.size();i++) errs() << *(seq[i]) << "\n";
    errs() << "\n";
#endif

    arlist.push_back(ar);

    if (countMap.count(atop) == 0) countMap.insert( { atop, 0 } );
    countMap.at(atop)++;
  }

  bool execute(BasicBlock &BB, unordered_set<Instruction *> &eraseList) {
    if (arlist.size() == 0) return false;

    Value *atop = NULL;
    AR_ReduceFraction *par0 = NULL, *par1 = NULL;

    for(;;) {
      atop = NULL;
      for(pair<Value *, int> p : countMap) {
	if (p.second >= 2) {
	  atop = p.first;
	  break;
	}
      }

      if (atop == NULL) return false;

      BinaryOperator *fdivOp0, *fdivOp1;

      par0 = par1 = NULL;
      for(AR_ReduceFraction &ar : arlist) {
	if (ar.seq[0] == atop) {
	  if (par0 == NULL) {
	    par0 = &ar;
	    fdivOp0 = dyn_cast<BinaryOperator>(par0->seq[par0->seq.size()-1]);
	  } else {
	    par1 = &ar;
	    fdivOp1 = dyn_cast<BinaryOperator>(par1->seq[par1->seq.size()-1]);
	    if (fdivOp0 != fdivOp1) break;
	  }
	}
      }

      assert(par0 && par1);

      if (fdivOp0 == fdivOp1) {
	countMap.at(atop) = 0;
	continue;
      }

      break;
    }

    vector<Value *> *pseq0 = &(par0->seq);
    vector<Value *> *pseq1 = &(par1->seq);

    BinaryOperator *fdivOp0 = dyn_cast<BinaryOperator>((*pseq0)[(*pseq0).size()-1]);
    BinaryOperator *fdivOp1 = dyn_cast<BinaryOperator>((*pseq1)[(*pseq1).size()-1]);
    
    for (Instruction &inst : BB) {
      if (&inst == fdivOp0) break;
      if (&inst == fdivOp1) {
	BinaryOperator *t = fdivOp0;
	fdivOp0 = fdivOp1;
	fdivOp1 = t;
	vector<Value *> *p = pseq0;
	pseq0 = pseq1;
	pseq1 = p;
	break;
      }
      // fdivOp1 comes later than fdivOp0
    }

    bool negative0 = false;
    for(unsigned i = 0;i < pseq0->size()-1;i++) {
      BinaryOperator *op = dyn_cast<BinaryOperator>((*pseq0)[i]);
      if (op && op->getOpcode() == Instruction::FSub &&
	  op->getOperand(1) == (*pseq0)[i+1]) negative0 = !negative0;
    }

    bool negative1 = false;
    for(unsigned i = 0;i < pseq1->size()-1;i++) {
      BinaryOperator *op = dyn_cast<BinaryOperator>((*pseq1)[i]);
      if (op && op->getOpcode() == Instruction::FSub &&
	  op->getOperand(1) == (*pseq1)[i+1]) negative1 = !negative1;
    }

#ifdef DEBUG_REDUCEFRAC
    errs() << "ReduceFraction : fdivOp0   = " << *fdivOp0 << "\n";
    errs() << "ReduceFraction : fdivOp1   = " << *fdivOp1 << "\n";
    errs() << "ReduceFraction : atop      = " << *atop << "\n";
    errs() << "ReduceFraction : negative0 = " << negative0 << "\n";
    errs() << "ReduceFraction : negative1 = " << negative1 << "\n";
#endif

    assert(fdivOp0 != fdivOp1);

    IRBuilder<> builder(fdivOp1);
    builder.setFastMathFlags(fdivOp1->getFastMathFlags());

    Value *aval = fdivOp1->getOperand(0);
    fdivOp1->setOperand(0, UndefValue::get(fdivOp1->getType()));

    Value *bval = fdivOp1->getOperand(1);
    fdivOp1->setOperand(1, UndefValue::get(fdivOp1->getType()));

    Value *cval = fdivOp0->getOperand(0);
    fdivOp0->setOperand(0, UndefValue::get(fdivOp0->getType()));

    Value *dval = fdivOp0->getOperand(1);
    fdivOp0->setOperand(1, UndefValue::get(fdivOp0->getType()));

    Value *atimesd = builder.CreateFMul(aval, dval);
    Value *btimesc = builder.CreateFMul(bval, cval);
    Value *btimesd = builder.CreateFMul(bval, dval);

    if (negative0 == negative1) {
      fdivOp1->setOperand(0, builder.CreateFAdd(atimesd, btimesc));
    } else {
      fdivOp1->setOperand(0, builder.CreateFSub(atimesd, btimesc));
    }
    fdivOp1->setOperand(1, btimesd);

    {
      // Remove fdivOp0

      BinaryOperator *op = dyn_cast<BinaryOperator>((*pseq0)[(*pseq0).size()-2]);
      assert(op->getOpcode() == Instruction::FAdd || 
	     op->getOpcode() == Instruction::FSub);

      if (op->getOperand(0) == fdivOp0) {
	op->setOperand(0, ConstantFP::get(op->getType(), 0));
      } else {
	assert(op->getOperand(1) == fdivOp0);
	op->setOperand(1, ConstantFP::get(op->getType(), 0));
      }

      assert(fdivOp0->getNumUses() == 0);
      eraseList.insert(fdivOp0);
    }

    nRewrite++;

    return true;
  }
};

//

struct AR_SimplifyCmpDiv {
  vector<Value *> seq = vector<Value *>();
  Value *atop = NULL, *mtop = NULL;
  Instruction *abot = NULL, *abot2 = NULL, *mbot = NULL;
};

struct SimplifyCmpDiv : public RewriteRule {
  // a/b + c > d -> b < 0 ^ a > b(d - c)

  vector<AR_SimplifyCmpDiv> arlist = vector<AR_SimplifyCmpDiv>();
  string name() { return "SimplifyCmpDiv"; }
  void clear() { arlist.clear(); }

  void match(vector<Value *> &seq) {
    // Matches FCmp FAdd* FMul* FDiv
    if (seq.size() < 2) return;
    if (arlist.size() >= 1) return; // Only matches once for this rule

    BinaryOperator *fdivOp = dyn_cast<BinaryOperator>(seq[seq.size()-1]);
    if (!fdivOp || fdivOp->getOpcode() != Instruction::FDiv) return;
    if (!fdivOp->getFastMathFlags().isFast()) return;
    {
      ConstantFP *c = dyn_cast<ConstantFP>(fdivOp->getOperand(1));
      if (c && c->isZero()) return;
    }

    int beginning;
    CmpInst *cmpInst = NULL;
    for(beginning = seq.size()-2;beginning >= 0;beginning--) {
      cmpInst = dyn_cast<CmpInst>(seq[beginning]);
      if (cmpInst) break;
      if (beginning == 0) return;
    }

    if (cmpInst->isEquality()) return;

    for(unsigned i = beginning;i<seq.size();i++) {
      if (seq[i]->getNumUses() != 1) return;
    }

    AR_SimplifyCmpDiv ar;

    unsigned i;
    for(i=beginning+1;i<seq.size()-1;i++) {
      BinaryOperator *op = dyn_cast<BinaryOperator>(seq[i]);
      if (!op) return;
      if (op->getOpcode() == Instruction::FMul) {
	ar.mtop = ar.mbot = op;
	break;
      }

      if (op->getOpcode() != Instruction::FAdd &&
	  op->getOpcode() != Instruction::FSub) return;

      if (ar.atop == NULL) {
	ar.atop = ar.abot2 = ar.abot = op;
      } else {
	ar.abot2 = ar.abot;
	ar.abot = op;
      }
    }

    for(;i<seq.size()-1;i++) {
      BinaryOperator *op = dyn_cast<BinaryOperator>(seq[i]);
      if (!op) return;
      if (op->getOpcode() != Instruction::FMul) return;
      ar.mbot = op;
    }

    for(i=beginning;i<seq.size();i++) ar.seq.push_back(seq[i]);

#ifdef DEBUG_CMPDIV
    errs() << "SimplifyCmpDiv match\n";
    for(i=beginning;i<seq.size();i++) errs() << *(seq[i]) << "\n";
    errs() << "\n";
#endif

    arlist.push_back(ar);
  }

  // A/B + C > D   ==>    B < 0 ^ A > B(D - C)
  //
  //     CMP           |  1. If there is no FMUL, A is A''
  //     / \           |  2. If there is FMUL, replace FDIV
  //  FADD  \ <-ATOP   |     with A'', and A is MTOP
  //    :    D         |  3. If there is no FADD, C is zero
  //  FADD    <-ABOT   |  4. If there is FADD, replace ABOT
  //   / \             |     with C', and C is ATOP
  //  C' FMUL <-MTOP   |  5. If ABOT is FSUB, A is
  //      :            |     multiplied with -1
  //     FMUL <-MBOT2  |
  //      |            |
  //     FMUL <-MBOT   |
  //     / \           |
  //  FDIV  A'         |
  //  /	\              |
  // A'' B             |

  bool execute(BasicBlock &BB, unordered_set<Instruction *> &eraseList) {
    if (arlist.size() == 0) return false;

    AR_SimplifyCmpDiv ar = arlist[0];
    vector<Value *> &q = ar.seq;

    CmpInst *cmpInst = dyn_cast<CmpInst>(q[0]);
    BinaryOperator *fdivOp = dyn_cast<BinaryOperator>(q[q.size()-1]);

    IRBuilder<> builder(cmpInst);
    builder.setFastMathFlags(cmpInst->getFastMathFlags());

#ifdef DEBUG_CMPDIV
    if (ar.atop != NULL)  errs() << "atop  : " << *(ar.atop)  << "\n"; else errs() << "atop  : NULL\n";
    if (ar.abot != NULL)  errs() << "abot  : " << *(ar.abot)  << "\n"; else errs() << "abot  : NULL\n";
    if (ar.abot2 != NULL) errs() << "abot2 : " << *(ar.abot2) << "\n"; else errs() << "abot2 : NULL\n";
    if (ar.mtop != NULL)  errs() << "mtop  : " << *(ar.mtop)  << "\n"; else errs() << "mtop  : NULL\n";
    if (ar.mbot != NULL)  errs() << "mbot  : " << *(ar.mbot)  << "\n"; else errs() << "mbot  : NULL\n";
#endif

    Value *bval = fdivOp->getOperand(1);
    fdivOp->setOperand(1, UndefValue::get(fdivOp->getType()));

    Value *dval;
    bool dvalOnLeft;

    if (ar.atop == cmpInst->getOperand(0) ||
	ar.mtop == cmpInst->getOperand(0) ||
	fdivOp  == cmpInst->getOperand(0)) {
      dval = cmpInst->getOperand(1);
      dvalOnLeft = false;
    } else {
      dval = cmpInst->getOperand(0);
      dvalOnLeft = true;
    }

    Value *aval = NULL;
    if (ar.mbot) {
      if (ar.mbot->getOperand(0) == fdivOp) {
	ar.mbot->setOperand(0, fdivOp->getOperand(0));
      } else {
	ar.mbot->setOperand(1, fdivOp->getOperand(0));
      }
      aval = ar.mtop;
    } else {
      aval = fdivOp->getOperand(0);
      fdivOp->setOperand(0, UndefValue::get(fdivOp->getType()));
    }

    bool negativeB = false, negativeC = false;
    Value *cval = NULL;
    if (ar.abot) {
      for(unsigned i=1;i<q.size()-1;i++) {
	BinaryOperator *op = dyn_cast<BinaryOperator>(q[i]);
	if (op && op->getOpcode() == Instruction::FSub &&
	    op->getOperand(1) == q[i+1]) negativeB = !negativeB;
      }

      Value *cp;
      bool negativeCP = false;
      if (ar.abot->getOperand(0) == ar.mtop || ar.abot->getOperand(0) == fdivOp) {
	cp = ar.abot->getOperand(1);
	if (ar.abot->getOpcode() == Instruction::FSub) negativeCP = true;
      } else {
	cp = ar.abot->getOperand(0);
      }

      if (ar.abot == ar.atop) {
	if (negativeCP) negativeC = true;
	cval = cp;
      } else {
	if (negativeCP) {
	  builder.SetInsertPoint(ar.abot2);
	  cp = builder.CreateFNeg(cp);
	}
	if (ar.abot2->getOperand(0) == ar.abot) {
	  ar.abot2->setOperand(0, cp);
	} else {
	  ar.abot2->setOperand(1, cp);
	}
	cval = ar.atop;
      }
      ar.abot->setOperand(0, UndefValue::get(ar.abot->getType()));
      ar.abot->setOperand(1, UndefValue::get(ar.abot->getType()));
      eraseList.insert(ar.abot);
    }

#ifdef DEBUG_CMPDIV
    errs() << "aval : " << *aval << "\n";
    errs() << "bval : " << *bval << "\n";
    if (cval == NULL) errs() << "cval : NULL\n"; else errs() << "cval : " << *cval << "\n";
    errs() << "dval : " << *dval << "\n";
    errs() << "negativeB : " << negativeB << "\n";
    errs() << "negativeC : " << negativeC << "\n";
    errs() << "dvalOnLeft : " << dvalOnLeft << "\n";
#endif

    // Now create B < 0 ^ A > B(D - C)

    builder.SetInsertPoint(cmpInst);

    Value *cmpInst2 = builder.CreateFCmpOLT(bval, ConstantFP::get(bval->getType(), 0));

    Value *dminusc = dval;
    if (cval != NULL && !negativeC) {
      dminusc = builder.CreateFSub(dval, cval);
    } else if (cval != NULL && negativeC) {
      dminusc = builder.CreateFAdd(dval, cval);
    }

    if (negativeB) dminusc = builder.CreateFNeg(dminusc);

    Value *fmulInst = builder.CreateFMul(bval, dminusc);

    CmpInst::Predicate pred  = cmpInst->getPredicate();
    if (negativeB ^ dvalOnLeft) pred = (CmpInst::getInversePredicate(pred));
    Value *cmpInst3 = builder.CreateFCmp(pred, aval, fmulInst);
    Value *xorInst  = builder.CreateXor(cmpInst2, cmpInst3);

    cmpInst->replaceAllUsesWith(xorInst);

    cmpInst->setOperand(0, UndefValue::get(cmpInst->getType()));
    cmpInst->setOperand(1, UndefValue::get(cmpInst->getType()));
    eraseList.insert(cmpInst);

    if (fdivOp->getNumUses() <= 1) {
      eraseList.insert(fdivOp);
    } else {
#ifdef DEBUG_CMPDIV
      errs() << "fdivOp has " << fdivOp->getNumUses() << "uses\n";
#endif
    }

    nRewrite++;

    return true;
  }
};

//

struct AR_SimplifyCmpZero {
  CmpInst *cmpInst;
  bool lzero;
  ConstantFP *constant;
  BinaryOperator *op;
};

struct SimplifyCmpZero : public RewriteRule {
  // a - b > 0 -> a > b

  vector<AR_SimplifyCmpZero> arlist = vector<AR_SimplifyCmpZero>();
  string name() { return "SimplifyCmpZero"; }
  void clear() { arlist.clear(); }

  void match(vector<Value *> &seq) {
    // Matches FCmp
    if (arlist.size() >= 1) return; // Only matches once for this rule

    CmpInst *cmpInst = dyn_cast<CmpInst>(seq[seq.size()-1]);
    if (!cmpInst) return;
    if (cmpInst->isEquality()) return;

    ConstantFP *lconst = dyn_cast<ConstantFP>(cmpInst->getOperand(0));
    bool lzero = lconst && lconst->isZero();

    ConstantFP *rconst = dyn_cast<ConstantFP>(cmpInst->getOperand(1));
    bool rzero = rconst && rconst->isZero();
    if (!lzero && !rzero) return;

    BinaryOperator *op =
      dyn_cast<BinaryOperator>(cmpInst->getOperand(lzero ? 1 : 0));

    if (!(op && (op->getOpcode() == Instruction::FAdd ||
		 op->getOpcode() == Instruction::FSub))) return;

    if (op->getNumUses() != 1) return;
    if (!op->getFastMathFlags().isFast()) return;

    //

    AR_SimplifyCmpZero ar;
    ar.cmpInst = cmpInst;
    ar.lzero = lzero;
    ar.constant = lzero ? lconst : rconst;
    ar.op = op;

#ifdef DEBUG_CMPZERO
    errs() << "SimplifyCmpZero match\n";
    errs() << "cmpInst = " << *cmpInst << "\n";
    errs() << "constant = " << *(ar.constant) << "\n";
    errs() << "op = " << *op << "\n";
    errs() << "lzero = " << lzero << "\n";
#endif

    arlist.push_back(ar);
  }

  bool execute(BasicBlock &BB, unordered_set<Instruction *> &eraseList) {
    if (arlist.size() == 0) return false;

    AR_SimplifyCmpZero ar = arlist[0];

    if (ar.op->getOpcode() == Instruction::FSub) {
      ar.cmpInst->setOperand(ar.lzero ? 0 : 1, ar.op->getOperand(1));
      ar.cmpInst->setOperand(ar.lzero ? 1 : 0, ar.op->getOperand(0));
    } else {
      assert(ar.op->getOpcode() == Instruction::FAdd);

      IRBuilder<> builder(ar.cmpInst);
      builder.setFastMathFlags(ar.cmpInst->getFastMathFlags());

      Value *v = builder.CreateFNeg(ar.op->getOperand(1));
      ar.cmpInst->setOperand(ar.lzero ? 0 : 1, v);
      ar.cmpInst->setOperand(ar.lzero ? 1 : 0, ar.op->getOperand(0));
    }

    ar.op->setOperand(0, UndefValue::get(ar.op->getType()));
    ar.op->setOperand(1, UndefValue::get(ar.op->getType()));
    assert(ar.op->getNumUses() == 0);
    eraseList.insert(ar.op);

    nRewrite++;

    return true;
  }
};

//

static bool initialized = false;
static vector<shared_ptr<RewriteRule>> rules;

struct MathPeephole : public FunctionPass {
  static char ID;

  //
  
  MathPeephole() : FunctionPass(ID) {
    if (!initialized) {
      initialized = true;

#ifdef ENABLE_CMPZERO
      rules.push_back(shared_ptr<RewriteRule>(new SimplifyCmpZero()));
#endif

#ifdef ENABLE_REDUCEFRAC
      rules.push_back(shared_ptr<RewriteRule>(new ReduceFraction()));
#endif

#ifdef ENABLE_CMPDIV
      rules.push_back(shared_ptr<RewriteRule>(new SimplifyCmpDiv()));
#endif
    }
  }

  void traverse(BasicBlock &BB, vector<Value *> &seq, unordered_set<Instruction *> &eraseList) {
    Value *val = seq[seq.size()-1];

    Instruction *inst = dyn_cast<Instruction>(val);
    if (inst && eraseList.count(inst) != 0) return;

    for(shared_ptr<RewriteRule> rule : rules) rule->match(seq);

    if (inst) {
      for(unsigned i=0;i<inst->getNumOperands();i++) {
	Instruction *inst2 = dyn_cast<Instruction>(inst->getOperand(i));
	if (inst2 && inst2->getParent() != &BB) continue;
	seq.push_back(inst->getOperand(i));
	traverse(BB, seq, eraseList);
	seq.pop_back();
      }
    }
  }

  bool runOnFunction(Function &F) override {
    if (F.isDeclaration()) return false;

    if (verbose >= 3) errs() << F << "\n";

    bool changed = false;
    unordered_set<Instruction *> eraseList;

    for(shared_ptr<RewriteRule> rule : rules) rule->clearStats();

    for (auto &BB : F) {
#ifndef NDEBUG
      bool divByZero = false;
      for (Instruction &instr : BB) {
	BinaryOperator *fdivOp = dyn_cast<BinaryOperator>(&instr);
	if (fdivOp && fdivOp->getOpcode() == Instruction::FDiv) {
	  ConstantFP *c = dyn_cast<ConstantFP>(fdivOp->getOperand(1));
	  if (c && c->isZero()) {
	    divByZero = true;
	    break;
	  }
	}
      }
      if (divByZero) continue;
#endif

      if (verbose >= 3) {
	for (Instruction &instr : BB) {
	  if (eraseList.count(&instr) == 0) errs() << "Before : " << instr << "\n";
	}
	errs() << "\n";
      }

      for (Instruction &instr : BB) {
	Instruction *inst = &instr;
	if (eraseList.count(inst) != 0) continue;
	bool isRoot = true;
	for(User *u : inst->users()) {
	  Instruction *inst2 = dyn_cast<Instruction>(u);
	  if (inst2 && inst2->getParent() == &BB) {
	    isRoot = false;
	    break;
	  }
	}
	if (!isRoot) continue;
	if (inst->getNumOperands() == 0) continue;

	Instruction *root = inst;

	if (verbose >= 3) errs() << "Root : " << *root << "\n";

	bool willRescanRoot;
	do {
	  willRescanRoot = false;
	  for(shared_ptr<RewriteRule> rule : rules) rule->clear();

	  vector<Value *> seq = vector<Value *>();
	  seq.push_back(root);

	  traverse(BB, seq, eraseList);

	  for(shared_ptr<RewriteRule> rule : rules) {
	    bool rewritten = false;

	    rewritten = rule->execute(BB, eraseList);
	    if (rewritten) {
	      changed = willRescanRoot = true;
	      if (verbose >= 3) {
		for (Instruction &xinstr : BB) {
		  if (eraseList.count(&xinstr) == 0) errs() << "Rewritten : " << xinstr << "\n";
		}
	      }
	      break;
	    }
	  }
	  if (verbose >= 3 && willRescanRoot) errs() << "\nRescanning\n";
	} while(willRescanRoot);
      }

      if (verbose >= 3) {
	errs() << "\n";
	for (Instruction &instr : BB)
	  if (eraseList.count(&instr) == 0) errs() << "After : " << instr << "\n";
	errs() << "\n";
      }
    }

    for(Instruction *inst : eraseList) {
      if (verbose >= 3) errs() << "Erasing " << *inst << " with " << inst->getNumUses() << " uses\n";
      inst->replaceAllUsesWith(UndefValue::get(inst->getType()));
      inst->eraseFromParent();
    }

    if (verbose >= 3) errs() << "Finish (" << changed << ")\n";

    if (verbose >= 1) {
      for(shared_ptr<RewriteRule> rule : rules) {
	if (rule->nRewrite == 0) continue;
	errs() << F.getName() << " : " << rule->name() << " is executed " << rule->nRewrite << " time(s).\n";
      }
    }

    return changed;
  }
}; // struct MathPeephole
}  // namespace

char MathPeephole::ID = 0;
static RegisterPass<MathPeephole> X("Xmath-peephole", "Mathematical peephole optimization",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);

static RegisterStandardPasses Y(
    PassManagerBuilder::EP_VectorizerStart,
    [](const PassManagerBuilder &Builder,
       legacy::PassManagerBase &PM) { PM.add(new MathPeephole()); });

//

static void loadPass(const PassManagerBuilder &Builder, legacy::PassManagerBase &PM) {
  PM.add(new MathPeephole());
}

static RegisterStandardPasses clangtoolLoader_Ox(PassManagerBuilder::EP_VectorizerStart, loadPass);
