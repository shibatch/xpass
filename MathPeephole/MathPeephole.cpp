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
#include <cstdlib>

//

static const int verbose = 0;

#define ENABLE_REDUCEFRAC
//#define DEBUG_REDUCEFRAC

#define ENABLE_REDUCEFRAC2
//#define DEBUG_REDUCEFRAC2

#define ENABLE_CMPDIV
//#define DEBUG_CMPDIV

#define ENABLE_CMPSQRT
//#define DEBUG_CMPSQRT

#define ENABLE_CMPZERO
//#define DEBUG_CMPZERO

#define ENABLE_CLEANUP
//#define DEBUG_CLEANUP

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

// Utility functions

bool checkInstOrder(BasicBlock &BB, Instruction *early, Instruction *late) {
  for (Instruction &inst : BB) {
    if (&inst == early) return true;
    if (&inst == late) return false;
  }
#ifndef NDEBUG
  errs() << "Abort : checkInstOrder\n";
#endif
  abort();
}

void eraseTree(Value *v, unordered_set<Instruction *> &eraseList) {
  if (!v) return;

  Instruction *inst = dyn_cast<Instruction>(v);
  if (!inst) return;
  if (inst->getNumUses() != 0) return;

  for(unsigned i=0;i<inst->getNumOperands();i++) {
    Value *v2 = inst->getOperand(i);
    inst->setOperand(i, UndefValue::get(inst->getType()));
    eraseTree(v2, eraseList);
  }

  eraseList.insert(inst);
}

Type *getCorrespondingIntType(Type *fpType, BasicBlock &BB) {
  if (!fpType->isVectorTy()) {
    if (fpType->isFloatTy()) {
      return Type::getInt32Ty(BB.getContext());
    } else {
      assert(fpType->isDoubleTy());
      return Type::getInt64Ty(BB.getContext());
    }
  }

  VectorType *vt = dyn_cast<VectorType>(fpType);
  Type *et = vt->getElementType();
  ElementCount ec = vt->getElementCount();
  if (et->isFloatTy()) {
    return VectorType::get(Type::getInt32Ty(BB.getContext()), ec);
  } else {
    assert(et->isDoubleTy());
    return VectorType::get(Type::getInt64Ty(BB.getContext()), ec);
  }
}

Type *getCorrespondingLogicType(Type *type, BasicBlock &BB) {
  if (!type->isVectorTy()) return Type::getInt1Ty(BB.getContext());

  VectorType *vt = dyn_cast<VectorType>(type);
  ElementCount ec = vt->getElementCount();
  return VectorType::get(Type::getInt1Ty(BB.getContext()), ec);
}

Type *getElementType(Type *type) {
  VectorType *vt = dyn_cast<VectorType>(type);
  Type *et = type;
  if (vt) et = vt->getElementType();
  return et;
}

unsigned getElementBitWidth(Type *type) {
  IntegerType *intType = dyn_cast<IntegerType>(getElementType(type));
  assert(intType);
  return intType->getBitWidth();
}

bool isFPConstant(Value *val, double d) {
  ConstantFP *c = dyn_cast<ConstantFP>(val);
  if (c) return c->isExactlyValue(d);

  if (d == 0 && dyn_cast<ConstantAggregateZero>(val)) return true;

  ConstantDataVector *dvec = dyn_cast<ConstantDataVector>(val);
  if (dvec) {
    c = dvec->getSplatValue() ? dyn_cast<ConstantFP>(dvec->getSplatValue()) : NULL;
    if (c) return c->isExactlyValue(d);
  }
  return false;
}

Value *mulSign(Value *fpval, Value *intSign, IRBuilder<> &builder) {
  Value *v = builder.CreateBitCast(fpval, intSign->getType());
  v = builder.CreateXor(v, intSign);
  return builder.CreateBitCast(v, fpval->getType());
}

bool isNonNegative(Value *v);

bool isNonPositive(Value *v) {
  ConstantFP *c = dyn_cast<ConstantFP>(v);
  if (c) return c->isNegative();

  if (dyn_cast<ConstantAggregateZero>(v)) return true;

  ConstantDataVector *dvec = dyn_cast<ConstantDataVector>(v);
  if (dvec) {
    ConstantFP *c = dvec->getSplatValue() ? dyn_cast<ConstantFP>(dvec->getSplatValue()) : NULL;
    if (c) return c->isNegative();
  }

  BinaryOperator *binOp = dyn_cast<BinaryOperator>(v);
  if (binOp && (binOp->getOpcode() == Instruction::FMul ||
		binOp->getOpcode() == Instruction::FDiv)) {
    if (isNonNegative(binOp->getOperand(0)) &&
	isNonPositive(binOp->getOperand(1))) return true;
    if (isNonPositive(binOp->getOperand(0)) &&
	isNonNegative(binOp->getOperand(1))) return true;
  }

  if (binOp && binOp->getOpcode() == Instruction::FAdd) {
    if (isNonPositive(binOp->getOperand(0)) &&
	isNonPositive(binOp->getOperand(1))) return true;
  }

  if (binOp && binOp->getOpcode() == Instruction::FSub) {
    if (isNonPositive(binOp->getOperand(0)) &&
	isNonNegative(binOp->getOperand(1))) return true;
  }

  UnaryOperator *uOp = dyn_cast<UnaryOperator>(v);
  if (uOp && uOp->getOpcode() == Instruction::FNeg) {
    if (isNonNegative(uOp->getOperand(0))) return true;
  }

  return false;
}

bool isNonNegative(Value *v) {
  CallInst *callInst = dyn_cast<CallInst>(v);
  if (callInst) {
    Function *calledFunc = callInst->getCalledFunction();
    if (calledFunc && calledFunc->getIntrinsicID() == Intrinsic::sqrt) return true;
    if (calledFunc && calledFunc->getIntrinsicID() == Intrinsic::fabs) return true;
  }

  ConstantFP *c = dyn_cast<ConstantFP>(v);
  if (c) return !c->isNegative();

  if (dyn_cast<ConstantAggregateZero>(v)) return true;

  ConstantDataVector *dvec = dyn_cast<ConstantDataVector>(v);
  if (dvec) {
    ConstantFP *c = dvec->getSplatValue() ? dyn_cast<ConstantFP>(dvec->getSplatValue()) : NULL;
    if (c) return !c->isNegative();
  }

  BinaryOperator *binOp = dyn_cast<BinaryOperator>(v);
  if (binOp && (binOp->getOpcode() == Instruction::FMul ||
		binOp->getOpcode() == Instruction::FDiv)) {
    if (binOp->getOperand(0) == binOp->getOperand(1)) return true;
    if (isNonNegative(binOp->getOperand(0)) &&
	isNonNegative(binOp->getOperand(1))) return true;
    if (isNonPositive(binOp->getOperand(0)) &&
	isNonPositive(binOp->getOperand(1))) return true;
  }

  if (binOp && binOp->getOpcode() == Instruction::FAdd) {
    if (isNonNegative(binOp->getOperand(0)) &&
	isNonNegative(binOp->getOperand(1))) return true;
  }

  if (binOp && binOp->getOpcode() == Instruction::FSub) {
    if (isNonNegative(binOp->getOperand(0)) &&
	isNonPositive(binOp->getOperand(1))) return true;
  }

  UnaryOperator *uOp = dyn_cast<UnaryOperator>(v);
  if (uOp && uOp->getOpcode() == Instruction::FNeg) {
    if (isNonPositive(uOp->getOperand(0))) return true;
  }

  return false;
}

//

struct AR_ReduceFraction {
  vector<Value *> seq = vector<Value *>();
};

struct ReduceFraction : public RewriteRule {
  // a/b + c/d  ->  (ad + bc) / (bd)

  vector<AR_ReduceFraction> arlist = vector<AR_ReduceFraction>();
  unordered_map<Value *, int> countMap = unordered_map<Value *, int>();

  string name() { return "ReduceFraction"; }
  void clear() {
    arlist.clear();
    countMap.clear();
  }

  void match(vector<Value *> &seq) {
    // Matches FAdd+ FDiv
    if (seq.size() < 2) return;

    BinaryOperator *fdivOp = dyn_cast<BinaryOperator>(seq[seq.size()-1]);
    if (!fdivOp || fdivOp->getOpcode() != Instruction::FDiv) return;
    if (!fdivOp->getFastMathFlags().isFast()) return;

    int beginning;
    for(beginning = seq.size()-2;beginning >= 0;beginning--) {
      BinaryOperator *op = dyn_cast<BinaryOperator>(seq[beginning]);
      if (!op) break;
      if (op->getOpcode() != Instruction::FAdd &&
	  op->getOpcode() != Instruction::FSub) break;
    }

    beginning++;

    for(int i = beginning;i<seq.size();i++) {
      if (i != beginning && seq[i]->getNumUses() != 1) return;
      BinaryOperator *op = dyn_cast<BinaryOperator>(seq[i]);
      if (!op->getFastMathFlags().isFast()) return;
    }

    //

    AR_ReduceFraction ar;
    Value *atop = seq[beginning];

    for(int i=beginning;i<seq.size();i++) ar.seq.push_back(seq[i]);

#ifdef DEBUG_REDUCEFRAC
    errs() << "ReduceFraction match\n";
    for(int i=beginning;i<seq.size();i++) errs() << *(seq[i]) << "\n";
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

    if (checkInstOrder(BB, fdivOp1, fdivOp0)) {
      swap(fdivOp0, fdivOp1);
      swap(pseq0, pseq1);
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

    // Remove fdivOp0
    assert(fdivOp0->getNumUses() == 1);
    fdivOp0->replaceAllUsesWith(ConstantFP::get(fdivOp0->getType(), 0));

    eraseList.insert(fdivOp0);

    nRewrite++;

    return true;
  }
};

//

struct AR_ReduceFraction2 {
  vector<Value *> seq = vector<Value *>();
  CmpInst *cmpInst = NULL;
  Value *atop = NULL;
  Instruction *abot = NULL, *abot2 = NULL;
  bool rightSide = false;
};

struct ReduceFraction2 : public RewriteRule {
  // a/b + e > c/d + f  ->  (ad - bc) / (bd) + e > f

  vector<AR_ReduceFraction2> arlist = vector<AR_ReduceFraction2>();
  unordered_map<CmpInst *, int> countMap = unordered_map<CmpInst *, int>();

  string name() { return "ReduceFraction2"; }
  void clear() {
    arlist.clear();
    countMap.clear();
  }

  void match(vector<Value *> &seq) {
    // Matches FCmp FAdd* FDiv
    if (seq.size() < 2) return;

    BinaryOperator *fdivOp = dyn_cast<BinaryOperator>(seq[seq.size()-1]);
    if (!fdivOp || fdivOp->getOpcode() != Instruction::FDiv) return;
    if (!fdivOp->getFastMathFlags().isFast()) return;

    int beginning;
    CmpInst *cmpInst = NULL;
    for(beginning = seq.size()-2;beginning >= 0;beginning--) {
      cmpInst = dyn_cast<CmpInst>(seq[beginning]);
      if (cmpInst) break;
      if (beginning == 0) return;
    }

    if (cmpInst->isEquality()) return;

    for(int i = beginning+1;i<seq.size();i++) {
      if (seq[i]->getNumUses() != 1) return;
      BinaryOperator *op = dyn_cast<BinaryOperator>(seq[i]);
      if (!(op && op->getFastMathFlags().isFast())) return;
    }

    //

    AR_ReduceFraction2 ar;

    if (cmpInst->getOperand(0) == seq[beginning+1]) {
      ar.rightSide = false;
    } else {
      assert(cmpInst->getOperand(1) == seq[beginning+1]);
      ar.rightSide = true;
    }

    ar.cmpInst = cmpInst;

    for(int i=beginning+1;i<seq.size()-1;i++) {
      BinaryOperator *op = dyn_cast<BinaryOperator>(seq[i]);
      if (!op) return;

      if (op->getOpcode() != Instruction::FAdd &&
	  op->getOpcode() != Instruction::FSub) return;

      if (ar.atop == NULL) {
	ar.atop = ar.abot2 = ar.abot = op;
      } else {
	ar.abot2 = ar.abot;
	ar.abot = op;
      }
    }

    for(int i=beginning;i<seq.size();i++) ar.seq.push_back(seq[i]);

#ifdef DEBUG_REDUCEFRAC2
    errs() << "ReduceFraction2 match\n";
    for(int i=beginning;i<seq.size();i++) errs() << *(seq[i]) << "\n";
    errs() << "\n";
#endif

    arlist.push_back(ar);

    if (countMap.count(ar.cmpInst) == 0) countMap.insert( { ar.cmpInst, 0 } );
    countMap.at(ar.cmpInst)++;
  }

  bool execute(BasicBlock &BB, unordered_set<Instruction *> &eraseList) {
    if (arlist.size() == 0) return false;

    CmpInst *cmpInst = NULL;
    AR_ReduceFraction2 *par0 = NULL, *par1 = NULL;

    for(;;) {
      cmpInst = NULL;
      for(pair<CmpInst *, int> p : countMap) {
	if (p.second >= 2) {
	  cmpInst = p.first;
	  break;
	}
      }

      if (cmpInst == NULL) return false;

      BinaryOperator *fdivOp0, *fdivOp1;

      par0 = par1 = NULL;
      for(AR_ReduceFraction2 &ar : arlist) {
	if (ar.seq[0] == cmpInst) {
	  if (par0 == NULL) {
	    par0 = &ar;
	    fdivOp0 = dyn_cast<BinaryOperator>(par0->seq[par0->seq.size()-1]);
	    assert(fdivOp0 && fdivOp0->getOpcode() == Instruction::FDiv);
	  } else {
	    par1 = &ar;
	    fdivOp1 = dyn_cast<BinaryOperator>(par1->seq[par1->seq.size()-1]);
	    assert(fdivOp1 && fdivOp1->getOpcode() == Instruction::FDiv);
	    if (fdivOp0 != fdivOp1 &&
		par0->rightSide != par1->rightSide) break;
	  }
	}
      }

      assert(par0 && par1);

      if (fdivOp0 == fdivOp1 ||
	  par0->rightSide == par1->rightSide) {
	countMap.at(cmpInst) = 0;
	continue;
      }

      break;
    }

    vector<Value *> *pseq0 = &(par0->seq);
    vector<Value *> *pseq1 = &(par1->seq);

    BinaryOperator *fdivOp0 = dyn_cast<BinaryOperator>((*pseq0)[(*pseq0).size()-1]);
    BinaryOperator *fdivOp1 = dyn_cast<BinaryOperator>((*pseq1)[(*pseq1).size()-1]);

    if (!checkInstOrder(BB, fdivOp0, fdivOp1)) {
      swap(fdivOp0, fdivOp1);
      swap(pseq0, pseq1);
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

#ifdef DEBUG_REDUCEFRAC2
    errs() << "ReduceFraction2 : cmpInst   = " << *cmpInst << "\n";
    errs() << "ReduceFraction2 : fdivOp0   = " << *fdivOp0 << "\n";
    errs() << "ReduceFraction2 : negative0 = " << negative0 << "\n";
    errs() << "ReduceFraction2 : fdivOp1   = " << *fdivOp1 << "\n";
    errs() << "ReduceFraction2 : negative1 = " << negative1 << "\n";
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
      fdivOp1->setOperand(0, builder.CreateFSub(atimesd, btimesc));
    } else {
      fdivOp1->setOperand(0, builder.CreateFAdd(atimesd, btimesc));
    }
    fdivOp1->setOperand(1, btimesd);

    // Remove fdivOp0
    assert(fdivOp0->getNumUses() == 1);
    fdivOp0->replaceAllUsesWith(ConstantFP::get(fdivOp0->getType(), 0));

    eraseList.insert(fdivOp0);

    nRewrite++;

    return true;
  }
};

//

struct AR_SimplifyCmpDiv {
  vector<Value *> seq = vector<Value *>();
  Value *atop = NULL;
  Instruction *abot = NULL, *abot2 = NULL;
};

struct SimplifyCmpDiv : public RewriteRule {
  // a/b + c > d -> b < 0 ^ a > b(d - c)

  vector<AR_SimplifyCmpDiv> arlist = vector<AR_SimplifyCmpDiv>();
  string name() { return "SimplifyCmpDiv"; }
  void clear() { arlist.clear(); }

  void match(vector<Value *> &seq) {
    // Matches FCmp FAdd* FDiv
    if (seq.size() < 2) return;
    if (arlist.size() >= 1) return; // Only matches once for this rule

    BinaryOperator *fdivOp = dyn_cast<BinaryOperator>(seq[seq.size()-1]);
    if (!fdivOp || fdivOp->getOpcode() != Instruction::FDiv) return;
    if (!fdivOp->getFastMathFlags().isFast()) return;
    if (!fdivOp->getType()->isVectorTy() &&
	!fdivOp->getType()->isFloatTy() && !fdivOp->getType()->isDoubleTy()) return;
    if (fdivOp->getType()->isVectorTy()) {
      VectorType *vt = dyn_cast<VectorType>(fdivOp->getType());
      if (!vt->getElementType()->isFloatTy() && !vt->getElementType()->isDoubleTy()) return;
    }

    int beginning;
    CmpInst *cmpInst = NULL;
    for(beginning = seq.size()-2;beginning >= 0;beginning--) {
      cmpInst = dyn_cast<CmpInst>(seq[beginning]);
      if (cmpInst) break;
      if (beginning == 0) return;
    }

    if (cmpInst->isEquality()) return;
    if (!cmpInst->isFast()) return;

    for(int i = beginning+1;i<seq.size();i++) {
      if (seq[i]->getNumUses() != 1) return;
      BinaryOperator *op = dyn_cast<BinaryOperator>(seq[i]);
      if (!(op && op->isFast())) return;
    }

    AR_SimplifyCmpDiv ar;

    int i;
    for(i=beginning+1;i<seq.size()-1;i++) {
      BinaryOperator *op = dyn_cast<BinaryOperator>(seq[i]);
      if (!op) return;

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
  //     CMP           |  
  //     / \           |  
  //  FADD  \ <-ATOP   |  
  //    :    D         |  1. If there is no FADD, C is zero
  //  FADD    <-ABOT   |  2. If there is FADD, replace ABOT
  //   / \             |     with C', and C is ATOP
  //  C' FDIV          |  3. If ABOT is FSUB, A is
  //     / \           |     multiplied with -1
  //    A   B          |

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
#endif

    Value *bval = fdivOp->getOperand(1);
    fdivOp->setOperand(1, UndefValue::get(fdivOp->getType()));

    Value *dval;
    bool dvalOnLeft;

    if (ar.atop == cmpInst->getOperand(0) ||
	fdivOp  == cmpInst->getOperand(0)) {
      dval = cmpInst->getOperand(1);
      dvalOnLeft = false;
    } else {
      dval = cmpInst->getOperand(0);
      dvalOnLeft = true;
    }

    if (isFPConstant(dval, 0)) dval = NULL;

    Value *aval = fdivOp->getOperand(0);
    fdivOp->setOperand(0, UndefValue::get(fdivOp->getType()));

    bool negativeB = false, negativeC = false; // B and C are multiplied by negative constants
    Value *cval = NULL;
    if (ar.abot) {
      for(unsigned i=1;i<q.size()-1;i++) {
	BinaryOperator *op = dyn_cast<BinaryOperator>(q[i]);
	if (op && op->getOpcode() == Instruction::FSub &&
	    op->getOperand(1) == q[i+1]) negativeB = !negativeB;
      }

      Value *cp;
      bool negativeCP = false;
      if (ar.abot->getOperand(0) == fdivOp) {
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
    errs() << "SimplifyCmpDiv : aval = " << *aval << "\n";
    errs() << "SimplifyCmpDiv : bval = " << *bval << "\n";
    if (cval == NULL) errs() << "SimplifyCmpDiv : cval = NULL\n";
    else errs() << "SimplifyCmpDiv : cval = " << *cval << "\n";
    if (dval == NULL) errs() << "SimplifyCmpDiv : dval = NULL\n";
    else errs() << "SimplifyCmpDiv : dval = " << *dval << "\n";
    errs() << "SimplifyCmpDiv : negativeB = " << negativeB << "\n";
    errs() << "SimplifyCmpDiv : negativeC = " << negativeC << "\n";
    errs() << "SimplifyCmpDiv : dvalOnLeft = " << dvalOnLeft << "\n";
#endif

    // Now create B < 0 ^ A > B(D - C)

    builder.SetInsertPoint(cmpInst);

    Type *fpType = fdivOp->getType();
    Type *intType = getCorrespondingIntType(fpType, BB);
    Value *signMask = ConstantInt::get(intType, 1UL << (getElementBitWidth(intType) - 1));

    bool bvalIsPositive = isNonNegative(bval); // bval itself is non-negative
#ifdef DEBUG_CMPDIV
    errs() << "SimplifyCmpDiv : bvalIsPositive = " << bvalIsPositive << "\n";
#endif

    Value *signBit = NULL;

    if (!bvalIsPositive) {
      Value *bitCast1 = builder.CreateBitCast(bval, intType);
      signBit = builder.CreateAnd(bitCast1, signMask);
    }

    Value *dminusc = dval;
    if (dval == cval && !negativeC) {
      dminusc = NULL;
    } else if (cval != NULL && !negativeC) {
      if (dval) {
	dminusc = builder.CreateFSub(dval, cval);
      } else {
	dminusc = builder.CreateFNeg(cval);
      }
    } else if (cval != NULL && negativeC) {
      if (dval) {
	dminusc = builder.CreateFAdd(dval, cval);
      } else {
	dminusc = cval;
      }
    }
    if (negativeB && dminusc) dminusc = builder.CreateFNeg(dminusc);
    Value *fmulInst = dminusc ? builder.CreateFMul(bval, dminusc) : NULL;

    Value *left = aval;
    if (signBit) {
      left = builder.CreateBitCast(aval, intType);
      left = builder.CreateXor(left, signBit);
      left = builder.CreateBitCast(left, fpType);
    }

    Value *right = NULL;
    if (fmulInst) {
      right = fmulInst;
      if (signBit) {
	right = builder.CreateBitCast(fmulInst, intType);
	right = builder.CreateXor(right, signBit);
	right = builder.CreateBitCast(right, fpType);
      }
    } else {
      right = ConstantFP::get(fpType, 0);
      eraseTree(bval, eraseList);
    }

    CmpInst::Predicate pred = cmpInst->getPredicate();
    if (negativeB ^ dvalOnLeft) pred = CmpInst::getSwappedPredicate(pred);
    Value *cmpInst2 = builder.CreateFCmp(pred, left, right);

    cmpInst->replaceAllUsesWith(cmpInst2);

    assert(cmpInst->getNumUses() == 0);
    cmpInst->setOperand(0, UndefValue::get(cmpInst->getType()));
    cmpInst->setOperand(1, UndefValue::get(cmpInst->getType()));
    eraseList.insert(cmpInst);

    eraseTree(cval, eraseList);
    eraseTree(dval, eraseList);

    assert(fdivOp->getNumUses() == 0);
    eraseList.insert(fdivOp);

    nRewrite++;

    return true;
  }
};

//

struct AR_SimplifyCmpSqrt {
  vector<Value *> seq = vector<Value *>();
  Instruction *atop = NULL, *mtop = NULL;
};

struct SimplifyCmpSqrt : public RewriteRule {
  // wpos = !signbit(w), s = wpos ? 1 : -1
  // w*sqrt(x) + y >  z   ->   s*(z-y) >= 0 ? s*(w*w)*x >  s*(z-y)*(z-y) :  wpos
  // w*sqrt(x) + y <  z   ->   s*(z-y) >= 0 ? s*(w*w)*x <  s*(z-y)*(z-y) : !wpos;
  // w*sqrt(x) + y >= z   ->   s*(z-y) >= 0 ? s*(w*w)*x >= s*(z-y)*(z-y) :  wpos;
  // w*sqrt(x) + y <= z   ->   s*(z-y) >= 0 ? s*(w*w)*x <= s*(z-y)*(z-y) : !wpos;

  vector<AR_SimplifyCmpSqrt> arlist = vector<AR_SimplifyCmpSqrt>();
  string name() { return "SimplifyCmpSqrt"; }
  void clear() { arlist.clear(); }

  void match(vector<Value *> &seq) {
    // Matches FCmp FAdd* FDiv? FMul* SQRT
    if (seq.size() < 2) return;
    if (arlist.size() >= 1) return; // Only matches once for this rule

    CallInst *callInst = dyn_cast<CallInst>(seq[seq.size()-1]);
    if (!callInst || !callInst->isFast()) return;
    Function *calledFunc = callInst->getCalledFunction();
    if (!calledFunc) return;
    if (calledFunc->getIntrinsicID() != Intrinsic::sqrt) return;

    int beginning;
    CmpInst *cmpInst = NULL;
    for(beginning = seq.size()-2;beginning >= 0;beginning--) {
      cmpInst = dyn_cast<CmpInst>(seq[beginning]);
      if (cmpInst) break;
      if (beginning == 0) return;
    }

    if (cmpInst->isEquality()) return;

    for(int i = beginning;i<seq.size();i++) {
      if (i != beginning && seq[i]->getNumUses() != 1) return;
      Instruction *inst = dyn_cast<Instruction>(seq[i]);
      if (!(inst && inst->getFastMathFlags().isFast())) return;
    }

    AR_SimplifyCmpSqrt ar;

    int i;
    for(i=beginning+1;i<seq.size()-1;i++) {
      BinaryOperator *op = dyn_cast<BinaryOperator>(seq[i]);
      if (!op) return;
      if (op->getOpcode() == Instruction::FDiv &&
	  op->getOperand(0) == seq[i+1]) {
	i++;
	ar.mtop = op;
	break;
      }

      if (op->getOpcode() == Instruction::FMul) {
	ar.mtop = op;
	break;
      }

      if (op->getOpcode() != Instruction::FAdd &&
	  op->getOpcode() != Instruction::FSub) return;

      if (ar.atop == NULL) ar.atop = op;
    }

    for(;i<seq.size()-1;i++) {
      BinaryOperator *op = dyn_cast<BinaryOperator>(seq[i]);
      if (!op) return;
      if (op->getOpcode() != Instruction::FMul) return;
    }

    for(i=beginning;i<seq.size();i++) ar.seq.push_back(seq[i]);

#ifdef DEBUG_CMPSQRT
    errs() << "SimplifyCmpSqrt match\n";
    for(i=beginning;i<seq.size();i++) errs() << *(seq[i]) << "\n";
    errs() << "\n";
#endif

    arlist.push_back(ar);
  }

  virtual bool execute(BasicBlock &BB, unordered_set<Instruction *> &eraseList) {
    if (arlist.size() == 0) return false;

    AR_SimplifyCmpSqrt ar = arlist[0];
    vector<Value *> &q = ar.seq;

    CmpInst *cmpInst = dyn_cast<CmpInst>(q[0]);
    CallInst *callInst = dyn_cast<CallInst>(q[q.size()-1]);
    assert(cmpInst && callInst);

    IRBuilder<> builder(cmpInst);
    builder.setFastMathFlags(cmpInst->getFastMathFlags());

#ifdef DEBUG_CMPSQRT
    if (ar.atop != NULL)  errs() << "SimplifyCmpSqrt : atop  : " << *(ar.atop)  << "\n";
    else errs() << "SimplifyCmpSqrt : atop  : NULL\n";
    if (ar.mtop != NULL)  errs() << "SimplifyCmpSqrt : mtop  : " << *(ar.mtop)  << "\n";
    else errs() << "SimplifyCmpSqrt : mtop  : NULL\n";
#endif

    bool negative = false;
    if (ar.atop) {
      for(int i=1;i<q.size()-1;i++) {
	BinaryOperator *op = dyn_cast<BinaryOperator>(q[i]);
	if (op && op->getOpcode() == Instruction::FSub &&
	    op->getOperand(1) == q[i+1]) negative = !negative;
      }
    }

    Value *yval = ar.atop;

    Value *zval;
    bool zvalOnLeft;
    if (ar.atop  == cmpInst->getOperand(0) ||
	ar.mtop  == cmpInst->getOperand(0) ||
	callInst == cmpInst->getOperand(0)) {
      zval = cmpInst->getOperand(1);
      zvalOnLeft = false;
    } else {
      zval = cmpInst->getOperand(0);
      zvalOnLeft = true;
    }

    Value *wval = NULL, *xval = callInst->getArgOperand(0);
    bool wis1 = false, wism1 = false, wisconst = false;
    bool wispositive = false, wisnegative = false;
    callInst->setArgOperand(0, UndefValue::get(xval->getType()));

    if (ar.mtop) {
      ar.mtop->replaceAllUsesWith(ConstantFP::get(callInst->getType(), 0));

      Value *mtop = ar.mtop;
      if (ar.mtop->getOperand(0) == callInst) {
	mtop = ar.mtop->getOperand(1);
	ar.mtop->setOperand(0, UndefValue::get(ar.mtop->getType()));
	ar.mtop->setOperand(1, UndefValue::get(ar.mtop->getType()));
	eraseList.insert(ar.mtop);
	assert(callInst->getNumUses() == 0);
      } else if (ar.mtop->getOperand(1) == callInst) {
	mtop = ar.mtop->getOperand(0);
	ar.mtop->setOperand(0, UndefValue::get(ar.mtop->getType()));
	ar.mtop->setOperand(1, UndefValue::get(ar.mtop->getType()));
	eraseList.insert(ar.mtop);
	assert(callInst->getNumUses() == 0);
      } else {
	callInst->replaceAllUsesWith(ConstantFP::get(callInst->getType(), 1));
      }
      wval = mtop;

      ConstantFP *c = dyn_cast<ConstantFP>(wval);
      if (!c) {
	ConstantDataVector *d = dyn_cast<ConstantDataVector>(wval);
	if (d && dyn_cast<ConstantFP>(d->getSplatValue())) {
	  c = dyn_cast<ConstantFP>(d->getSplatValue());
	}
      }
      if (c) {
	bool wIsNegative = c->isNegative();
	if (negative) {
	  const APFloat &a = c->getValueAPF();
	  wval = ConstantFP::get(wval->getType(), APFloat::getZero(a.getSemantics()) - a);
	  wIsNegative = !wIsNegative;
	}
	if (wIsNegative) {
	  wisnegative = true;
	  wisconst = true;
	} else {
	  wispositive = true;
	  wisconst = true;
	}
      } else {
	if (negative) wval = builder.CreateFNeg(wval);
      }
    } else {
      callInst->replaceAllUsesWith(ConstantFP::get(callInst->getType(), 0));
      if (negative) {
	wval = ConstantFP::get(callInst->getType(), -1);
	wism1 = wisconst = wisnegative = true;
      } else {
	wval = ConstantFP::get(callInst->getType(), 1);
	wis1 = wisconst = wispositive = true;
      }
    }

    wispositive = isNonNegative(wval);
    wisnegative = isNonPositive(wval);

#ifdef DEBUG_CMPSQRT
    errs() << "SimplifyCmpSqrt : wval = " << *wval << "\n";
    errs() << "SimplifyCmpSqrt : wis1 = " << wis1 << ", wism1 = " << wism1 << "\n";
    errs() << "SimplifyCmpSqrt : wisconst = " << wisconst << "\n";
    errs() << "SimplifyCmpSqrt : wispositive = " << wispositive << "\n";
    errs() << "SimplifyCmpSqrt : wisnegative = " << wisnegative << "\n";
    errs() << "SimplifyCmpSqrt : xval = " << *xval << "\n";
    if (yval == NULL) errs() << "SimplifyCmpSqrt : yval = NULL\n";
    else errs() << "SimplifyCmpSqrt : yval = " << *yval << "\n";
    errs() << "SimplifyCmpSqrt : zval = " << *zval << "\n";
    errs() << "SimplifyCmpSqrt : negative = " << negative << "\n";
    errs() << "SimplifyCmpSqrt : zvalOnLeft = " << zvalOnLeft << "\n";
#endif

    CmpInst::Predicate pred = cmpInst->getPredicate();
    if (zvalOnLeft) pred = CmpInst::getSwappedPredicate(pred);

    Type *fpType = cmpInst->getOperand(0)->getType();
    bool isVector = fpType->isVectorTy();
    Type *intType = getCorrespondingIntType(fpType, BB);
    Value *signMask = ConstantInt::get(intType, 1UL << (getElementBitWidth(intType) - 1));

    Value *signbitW = NULL;
    Value *wsign = NULL;
    if (!(wispositive || wisnegative)) {
      wsign = builder.CreateBitCast(wval, intType);
      signbitW = builder.CreateAnd(wsign, signMask);
      wsign = builder.CreateLShr(wsign, getElementBitWidth(intType) - 1);
    }

    Value *zminusy = yval ? builder.CreateFSub(zval, yval) : zval;
    Value *sTimes_ZMinusY;
    if (wispositive) {
      sTimes_ZMinusY = zminusy;
    } else if (wisnegative) {
      sTimes_ZMinusY = builder.CreateFNeg(zminusy);
    } else {
      sTimes_ZMinusY = mulSign(zminusy, signbitW, builder);
    }

    Value *cmpSTimes_ZMinusY = NULL;
    if (isNonNegative(sTimes_ZMinusY)) {
      cmpSTimes_ZMinusY = ConstantInt::get(getCorrespondingLogicType(fpType, BB), 1);
    } else if (isNonPositive(sTimes_ZMinusY)) {
      cmpSTimes_ZMinusY = ConstantInt::get(getCorrespondingLogicType(fpType, BB), 0);
    } else {
      cmpSTimes_ZMinusY = builder.CreateFCmpOGE(sTimes_ZMinusY, ConstantFP::get(fpType, 0));
    }

    Value *sTimes_ZMinusYSqu = NULL;

    if (sTimes_ZMinusY == zminusy) {
      CallInst *callInst3 = dyn_cast<CallInst>(zminusy);
      if (callInst3) {
	Function *calledFunc = callInst3->getCalledFunction();
	if (calledFunc && calledFunc->getIntrinsicID() == Intrinsic::sqrt) {
	  sTimes_ZMinusYSqu = callInst3->getArgOperand(0);
	  if (callInst3->getNumUses() <= 1) {
	    callInst3->setArgOperand(0, UndefValue::get(sTimes_ZMinusYSqu->getType()));
	    callInst3->replaceAllUsesWith(ConstantFP::get(callInst3->getType(), 0));
	    assert(callInst3->getNumUses() == 0);
	    eraseList.insert(callInst3);
	  }
	}
      }
    }

    if (sTimes_ZMinusYSqu == NULL) {
      sTimes_ZMinusYSqu = builder.CreateFMul(sTimes_ZMinusY, zminusy);
    }

    Value *wtimesw = NULL;
    if (wis1 || wism1) {
      wtimesw = NULL;
    } else if (wisconst) {
      ConstantFP *c = dyn_cast<ConstantFP>(wval);
      if (c) {
	const APFloat &a = c->getValueAPF();
	wtimesw = ConstantFP::get(wval->getType(), a*a);
      }

      ConstantDataVector *d = dyn_cast<ConstantDataVector>(wval);
      if (d && dyn_cast<ConstantFP>(d->getSplatValue())) {
	c = dyn_cast<ConstantFP>(d->getSplatValue());
	const APFloat &a = c->getValueAPF();
	wtimesw = ConstantFP::get(wval->getType(), a*a);
      }

      assert(wtimesw);
    } else {
      wtimesw = builder.CreateFMul(wval, wval);
    }
    Value *wwx = (wis1 || wism1) ? xval : builder.CreateFMul(wtimesw, xval);
    Value *swwx;
    if (wispositive) {
      swwx = wwx;
    } else if (wisnegative) {
      swwx = builder.CreateFNeg(wwx);
    } else {
      swwx = mulSign(wwx, signbitW, builder);
    }
    Value *cmpMain = builder.CreateFCmp(pred, swwx, sTimes_ZMinusYSqu);

    Value *andInst1 = builder.CreateAnd(cmpSTimes_ZMinusY, cmpMain);

    Value *andInst2 = NULL;

    switch(pred) {
    case CmpInst::Predicate::FCMP_OGT:
    case CmpInst::Predicate::FCMP_UGT:
    case CmpInst::Predicate::FCMP_OGE:
    case CmpInst::Predicate::FCMP_UGE:
      if (wispositive) {
	andInst2 = builder.CreateNot(cmpSTimes_ZMinusY);
      } else if (wisnegative) {
      } else {
	wsign = builder.CreateIntCast(wsign, getCorrespondingLogicType(fpType, BB), true);
	wsign = builder.CreateNot(wsign);
	andInst2 = builder.CreateNot(cmpSTimes_ZMinusY);
	andInst2 = builder.CreateAnd(andInst2, wsign);
      }
      break;
    case CmpInst::Predicate::FCMP_OLT:
    case CmpInst::Predicate::FCMP_ULT:
    case CmpInst::Predicate::FCMP_OLE:
    case CmpInst::Predicate::FCMP_ULE:
      if (wispositive) {
      } else if (wisnegative) {
	andInst2 = builder.CreateNot(cmpSTimes_ZMinusY);
      } else {
	wsign = builder.CreateIntCast(wsign, getCorrespondingLogicType(fpType, BB), true);
	andInst2 = builder.CreateNot(cmpSTimes_ZMinusY);
	andInst2 = builder.CreateAnd(andInst2, wsign);
      }
      break;
    default:
#ifndef NDEBUG
      errs() << "Abort : SimplifyCmpSqrt::execute\n";
#endif
      abort();
    }

    Value *orInst = andInst2 ? builder.CreateOr(andInst1, andInst2) : andInst1;

    cmpInst->replaceAllUsesWith(orInst);

    cmpInst->setOperand(0, UndefValue::get(cmpInst->getType()));
    cmpInst->setOperand(1, UndefValue::get(cmpInst->getType()));
    eraseList.insert(cmpInst);

    eraseList.insert(callInst);

    nRewrite++;

    return true;
  }
};

//

struct AR_CleanupMulDiv {
  vector<Value *> seq = vector<Value *>();
};

struct AR_CleanupMulOne {
  BinaryOperator *op = NULL;
  bool ope0IsOne = false;
};

struct AR_CleanupAddZero {
  BinaryOperator *op = NULL;
  bool ope0IsZero = false;
  bool isFSub = false;
};

struct Cleanup : public RewriteRule {
  // (a / b) * c -> (a * c) / b

  vector<AR_CleanupMulDiv>  arlistMulDiv  = vector<AR_CleanupMulDiv>();
  vector<AR_CleanupMulOne>  arlistMulOne  = vector<AR_CleanupMulOne>();
  vector<AR_CleanupAddZero> arlistAddZero = vector<AR_CleanupAddZero>();
  string name() { return "Cleanup"; }

  void clear() {
    arlistMulDiv.clear();
    arlistMulOne.clear();
    arlistAddZero.clear();
  }

  void matchMulDiv(vector<Value *> &seq) {
    // Matches FMul+ FDiv
    if (arlistMulDiv.size() >= 1) return;
    if (seq.size() < 2) return;

    BinaryOperator *fdivOp =
      dyn_cast<BinaryOperator>(seq[seq.size()-1]);

    if (!(fdivOp && (fdivOp->getOpcode() == Instruction::FDiv))) return;
    if (!fdivOp->getFastMathFlags().isFast()) return;

    int beginning;
    for(beginning = seq.size()-2;beginning >= 0;beginning--) {
      BinaryOperator *op = dyn_cast<BinaryOperator>(seq[beginning]);
      if (!op) break;
      if (op->getOpcode() != Instruction::FMul) break;
    }

    beginning++;
    if (beginning == seq.size()-1) return;

    for(int i = beginning;i<seq.size();i++) {
      if (i != beginning && seq[i]->getNumUses() != 1) return;
      BinaryOperator *op = dyn_cast<BinaryOperator>(seq[i]);
      if (!op->getFastMathFlags().isFast()) return;
    }

    //

    AR_CleanupMulDiv ar;
    for(int i=beginning;i<seq.size();i++) ar.seq.push_back(seq[i]);

#ifdef DEBUG_CLEANUP
    errs() << "Cleanup match\n";
    for(int i=beginning;i<seq.size();i++) errs() << *(seq[i]) << "\n";
    errs() << "\n";
#endif

    arlistMulDiv.push_back(ar);
  }

  void matchMulOne(vector<Value *> &seq) {
    BinaryOperator *op =
      dyn_cast<BinaryOperator>(seq[seq.size()-1]);

    if (!op) return;

    if (op->getOpcode() == Instruction::FMul) {
      if (isFPConstant(op->getOperand(0), 1.0)) {
#ifdef DEBUG_CLEANUP
	errs() << "Clearnup (mulone) match\n";
#endif
	AR_CleanupMulOne ar;
	ar.op = op;
	ar.ope0IsOne = true;
	arlistMulOne.push_back(ar);
	return;
      }
    }

    if (op->getOpcode() == Instruction::FMul ||
	op->getOpcode() == Instruction::FDiv) {
      if (isFPConstant(op->getOperand(1), 1.0)) {
#ifdef DEBUG_CLEANUP
	errs() << "Clearnup (mulone) match\n";
#endif
	AR_CleanupMulOne ar;
	ar.op = op;
	ar.ope0IsOne = false;
	arlistMulOne.push_back(ar);
	return;
      }
    }
  }

  void matchAddZero(vector<Value *> &seq) {
    BinaryOperator *addOp =
      dyn_cast<BinaryOperator>(seq[seq.size()-1]);

    if (!(addOp && (addOp->getOpcode() == Instruction::FAdd ||
		    addOp->getOpcode() == Instruction::FSub))) return;

    ConstantFP *c;
    if (isFPConstant(addOp->getOperand(0), 0)) {
#ifdef DEBUG_CLEANUP
      errs() << "Clearnup (addzero) match\n";
#endif
      AR_CleanupAddZero ar;
      ar.op = addOp;
      ar.ope0IsZero = true;
      ar.isFSub = addOp->getOpcode() == Instruction::FSub;
      arlistAddZero.push_back(ar);
      return;
    }

    if (isFPConstant(addOp->getOperand(1), 0)) {
#ifdef DEBUG_CLEANUP
      errs() << "Clearnup (addzero) match\n";
#endif
      AR_CleanupAddZero ar;
      ar.op = addOp;
      ar.ope0IsZero = false;
      ar.isFSub = addOp->getOpcode() == Instruction::FSub;
      arlistAddZero.push_back(ar);
      return;
    }
  }

  void match(vector<Value *> &seq) {
    matchMulDiv(seq);
    matchMulOne(seq);
    matchAddZero(seq);
  }

  bool executeMulDiv(BasicBlock &BB, unordered_set<Instruction *> &eraseList) {
    if (arlistMulDiv.size() == 0) return false;

    AR_CleanupMulDiv ar = arlistMulDiv[0];

    BinaryOperator *fdivOp = dyn_cast<BinaryOperator>(ar.seq[ar.seq.size()-1]);
    BinaryOperator *mtop = dyn_cast<BinaryOperator>(ar.seq[0]);
    BinaryOperator *mbot = dyn_cast<BinaryOperator>(ar.seq[ar.seq.size()-2]);

    mtop->replaceAllUsesWith(fdivOp);

    if (mbot->getOperand(0) == fdivOp) {
      mbot->setOperand(0, ConstantFP::get(mbot->getType(), 1));

      AR_CleanupMulOne arm1;
      arm1.op = mbot;
      arm1.ope0IsOne = true;
      arlistMulOne.push_back(arm1);
    } else {
      assert(mbot->getOperand(1) == fdivOp);
      mbot->setOperand(1, ConstantFP::get(mbot->getType(), 1));

      AR_CleanupMulOne arm1;
      arm1.op = mbot;
      arm1.ope0IsOne = false;
      arlistMulOne.push_back(arm1);
    }

    fdivOp->moveAfter(mtop);

    IRBuilder<> builder(fdivOp);
    builder.setFastMathFlags(fdivOp->getFastMathFlags());

    fdivOp->setOperand(0, builder.CreateFMul(mtop, fdivOp->getOperand(0)));

    nRewrite++;

    return true;
  }

  bool executeMulOne(BasicBlock &BB, unordered_set<Instruction *> &eraseList) {
    bool changed = false;

    for(AR_CleanupMulOne &ar : arlistMulOne) {
      BinaryOperator *op = ar.op;
      op->replaceAllUsesWith(op->getOperand(ar.ope0IsOne ? 1 : 0));
      op->setOperand(ar.ope0IsOne ? 1 : 0, UndefValue::get(op->getType()));
      eraseList.insert(op);
      nRewrite++;
      changed = true;
    }

    return changed;
  }

  bool executeAddZero(BasicBlock &BB, unordered_set<Instruction *> &eraseList) {
    bool changed = false;

    for(AR_CleanupAddZero &ar : arlistAddZero) {
      BinaryOperator *op = ar.op;
      if (!(ar.isFSub && ar.ope0IsZero)) {
	op->replaceAllUsesWith(op->getOperand(ar.ope0IsZero ? 1 : 0));
	op->setOperand(ar.ope0IsZero ? 1 : 0, UndefValue::get(op->getType()));
	eraseList.insert(op);
      } else {
	IRBuilder<> builder(op);
	op->replaceAllUsesWith(builder.CreateFNeg(op->getOperand(1)));
	op->setOperand(1, UndefValue::get(op->getType()));
	eraseList.insert(op);
      }
      nRewrite++;
      changed = true;
    }

    return changed;
  }

  bool execute(BasicBlock &BB, unordered_set<Instruction *> &eraseList) {
    bool changed = false;
    changed = executeMulDiv(BB, eraseList) || changed;
    changed = executeMulOne(BB, eraseList) || changed;
    changed = executeAddZero(BB, eraseList) || changed;
    return changed;
  }
};

//

struct AR_SimplifyCmpZero {
  CmpInst *cmpInst;
  bool lzero;
  Constant *constant;
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

    Constant *lconst = dyn_cast<Constant>(cmpInst->getOperand(0));
    bool lzero = lconst && isFPConstant(lconst, 0);

    Constant *rconst = dyn_cast<Constant>(cmpInst->getOperand(1));
    bool rzero = rconst && isFPConstant(rconst, 0);
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

struct MathPeephole : public FunctionPass {
  static char ID;
  vector<shared_ptr<RewriteRule>> rules;

  MathPeephole() : FunctionPass(ID) {
#ifdef ENABLE_CLEANUP
    rules.push_back(shared_ptr<RewriteRule>(new Cleanup()));
#endif

#ifdef ENABLE_CMPZERO
    rules.push_back(shared_ptr<RewriteRule>(new SimplifyCmpZero()));
#endif

#ifdef ENABLE_REDUCEFRAC2
    rules.push_back(shared_ptr<RewriteRule>(new ReduceFraction2()));
#endif

#ifdef ENABLE_REDUCEFRAC
    rules.push_back(shared_ptr<RewriteRule>(new ReduceFraction()));
#endif

#ifdef ENABLE_CMPDIV
    rules.push_back(shared_ptr<RewriteRule>(new SimplifyCmpDiv()));
#endif

#ifdef ENABLE_CMPSQRT
    rules.push_back(shared_ptr<RewriteRule>(new SimplifyCmpSqrt()));
#endif
  }

  void traverse(BasicBlock &BB, vector<Value *> &seq, unordered_set<Instruction *> &eraseList, unordered_set<Value *> &visited) {
    Value *val = seq[seq.size()-1];

    // Visit each instruction only once
    if (visited.count(val) != 0) return; 
    visited.insert(val);

    Instruction *inst = dyn_cast<Instruction>(val);
    if (inst && eraseList.count(inst) != 0) return;

    if (inst) {
      for(unsigned i=0;i<inst->getNumOperands();i++) {
	Instruction *inst2 = dyn_cast<Instruction>(inst->getOperand(i));
	if (inst2 && inst2->getParent() != &BB) continue;
	if (inst2 && checkInstOrder(BB, inst, inst2)) continue;
	seq.push_back(inst->getOperand(i));
	traverse(BB, seq, eraseList, visited);
	seq.pop_back();
      }
    }

    for(shared_ptr<RewriteRule> rule : rules) rule->match(seq);
  }

  bool runOnFunction(Function &F) override {
    if (F.isDeclaration()) return false;

    if (!F.getAttributes().getFnAttributes().hasAttribute("unsafe-fp-math") ||
	F.getAttributes().getFnAttributes().getAttribute("unsafe-fp-math").getValueAsString() != "true") {
      if (verbose >= 3) errs() << "Skipping " << F.getName() << "\n";
      return false;
    }

    if (verbose >= 3) errs() << F << "\n";

    bool changed = false;
    unordered_set<Instruction *> eraseList;

    for(shared_ptr<RewriteRule> rule : rules) rule->clearStats();

    for (auto &BB : F) {
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
	  if (!inst2) continue;
	  if (inst2->getParent() != &BB) continue;
	  if (checkInstOrder(BB, inst2, inst)) continue;
	  isRoot = false;
	  break;
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

	  unordered_set<Value *> visited = unordered_set<Value *>();

	  traverse(BB, seq, eraseList, visited);

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
      assert(inst->getNumUses() == 0);
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
    PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder &Builder,
       legacy::PassManagerBase &PM) { PM.add(new MathPeephole()); });

//

static void loadPass(const PassManagerBuilder &Builder, legacy::PassManagerBase &PM) {
  PM.add(new MathPeephole());
}

static RegisterStandardPasses clangtoolLoader_Ox(PassManagerBuilder::EP_OptimizerLast, loadPass);
