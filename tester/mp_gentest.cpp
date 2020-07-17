#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <cassert>
#include <cstdlib>
#include <cmath>
#include <chrono>

#define NTEST 100
#define DEPTHMAX 100
#define NCHECK 100

#define NARGS 4
#define NBINOP 4
#define NUOP 2
#define NLBINOP 2
#define NCMPOP 6

using namespace std;

double rand01() { return rand() / (double)RAND_MAX; }
double randm11() { return rand() / (double)RAND_MAX * 2 - 1; }

string asqrtfunc = "asqrt";

struct Args {
  vector<double> args = vector<double>(NARGS);
  Args() { for(int i=0;i<NARGS;i++) args[i] = randm11(); }
  Args(double d) { for(int i=0;i<NARGS;i++) args[i] = d; }
};

struct Number {
  Number *generate(int depth);
  virtual double eval(Args &a) { return 0; }
  virtual bool check() { return true; }
  virtual string toString() { return ""; }
  virtual int countNode() { return 0; }
};

struct Logic {
  Logic *generate(int depth, Number *num);
  virtual bool check() { return true; }
  virtual bool eval(Args &a) { return false; }
  virtual string toString() { return ""; }
  virtual int countNode() { return 0; }
};

struct Arg : public Number {
  int n;

  Arg(int a) { n = a; }
  double eval(Args &a) { return a.args.at(n); }
  string toString() { return "a" + to_string(n); }
  int countNode() { return 1; }
};

struct Const : public Number {
  double c;

  Const(double x) { c = x; }
  double eval(Args &a) { return c; }
  string toString() { return "(" + to_string(c) + ")"; }
  int countNode() { return 1; }
};

struct BinOp : public Number {
  Number *left, *right;
  int opcode;

  BinOp(int op, Number *l, Number *r) { opcode = op; left = l; right = r; }
  double eval(Args &a) {
    switch(opcode) {
    case 0: return left->eval(a) + right->eval(a);
    case 1: return left->eval(a) - right->eval(a);
    case 2: return left->eval(a) * right->eval(a);
    case 3: return left->eval(a) / right->eval(a);
    default: abort();
    }
  }
  bool check() {
    if (opcode == 3) {
      for(int i=0;i<NCHECK;i++) {
	Args *a = new Args();
	if (fabs(right->eval(*a)) < 1e-4) return false;
	if (fabs(fabs(left->eval(*a)/right->eval(*a)) - 1.0) < 1e-4) return false;
	delete a;
      }
      Args *a = new Args(0);
      if (fabs(right->eval(*a)) < 1e-4) return false;
      if (fabs(fabs(left->eval(*a)/right->eval(*a)) - 1.0) < 1e-4) return false;
      delete a;
    }
    return left->check() && right->check();
  }
  string toString() {
    switch(opcode) {
    case 0: return "(" + left->toString() + " + " + right->toString() + ")";
    case 1: return "(" + left->toString() + " - " + right->toString() + ")";
    case 2: return "(" + left->toString() + " * " + right->toString() + ")";
    case 3: return "(" + left->toString() + " / " + right->toString() + ")";
    default: abort();
    }
  }
  int countNode() { return left->countNode() + right->countNode() + 1; }
};

struct UOp : public Number {
  Number *op;
  int opcode;

  UOp(int opc, Number *n) { opcode = opc; op = n; }
  double eval(Args &a) {
    switch(opcode) {
    case (NBINOP + 0): return -op->eval(a);
    case (NBINOP + 1): return sqrt(fabs(op->eval(a)));
    default : abort();
    }
  }
  bool check() { return op->check(); }
  string toString() {
    switch(opcode) {
    case (NBINOP + 0): return "(-" + op->toString() + ")";
    case (NBINOP + 1): return asqrtfunc + "(" + op->toString() + ")";
    default: abort();
    }
  }
  int countNode() { return op->countNode() + 1; }
};

Number* Number::generate(int depth) {
  if (depth >= DEPTHMAX) return new Const(randm11());
  depth++;
  int n = (int)(rand01() * (1 + NARGS + NBINOP + NUOP));

  if (n == 0) return new Const(randm11());
  if (n < 1 + NARGS) return new Arg(n-1);
  if (n < 1 + NARGS + NBINOP) return new BinOp(n - NARGS - 1, generate(depth), generate(depth));
  if (n < 1 + NARGS + NBINOP + NUOP) return new UOp(n - NARGS - 1, generate(depth));

  abort();
}

struct LConst : public Logic {
  bool val;

  LConst(bool b) { val = b; }
  bool eval(Args &a) { return val; }
  string toString() { return val ? "true" : "false"; }
  int countNode() { return 1; }
};

struct LBinOp : public Logic {
  Logic *left, *right;
  int opcode;

  LBinOp(int op, Logic *l, Logic *r) { opcode = op; left = l; right = r; }
  bool eval(Args &a) {
    switch(opcode) {
    case (NARGS + NBINOP + NUOP + 0): return left->eval(a) || right->eval(a);
    case (NARGS + NBINOP + NUOP + 1): return left->eval(a) && right->eval(a);
    default: abort();
    }
  }
  bool check() { return left->check() && right->check(); }
  string toString() {
    switch(opcode) {
    case (NARGS + NBINOP + NUOP + 0): return "(" + left->toString() + " || " + right->toString() + ")";
    case (NARGS + NBINOP + NUOP + 1): return "(" + left->toString() + " && " + right->toString() + ")";
    default: abort();
    }
  }
  int countNode() { return left->countNode() + right->countNode() + 1; }
};

struct Compare : public Logic {
  Number *left, *right;
  int opcode;

  Compare(int op, Number *l, Number *r) { opcode = op; left = l; right = r; }
  bool eval(Args &a) {
    switch(opcode) {
    case (NARGS + NBINOP + NUOP + NLBINOP + 0): return left->eval(a) <  right->eval(a);
    case (NARGS + NBINOP + NUOP + NLBINOP + 1): return left->eval(a) >  right->eval(a);
    case (NARGS + NBINOP + NUOP + NLBINOP + 2): return left->eval(a) <= right->eval(a);
    case (NARGS + NBINOP + NUOP + NLBINOP + 3): return left->eval(a) >= right->eval(a);
    case (NARGS + NBINOP + NUOP + NLBINOP + 4): return left->eval(a) == right->eval(a);
    case (NARGS + NBINOP + NUOP + NLBINOP + 5): return left->eval(a) != right->eval(a);
    default: abort();
    }
  }
  bool check() {
    for(int i=0;i<NCHECK;i++) {
      Args *a = new Args();
      if (fabs(left->eval(*a)-right->eval(*a)) < 1e-4) return false;
      delete a;
    }
    Args *a = new Args(0);
    if (fabs(left->eval(*a)-right->eval(*a)) < 1e-4) return false;
    delete a;
    return left->check() && right->check();
  }
  string toString() {
    switch(opcode) {
    case (NARGS + NBINOP + NUOP + NLBINOP + 0): return left->toString() + " < " + right->toString();
    case (NARGS + NBINOP + NUOP + NLBINOP + 1): return left->toString() + " > " + right->toString();
    case (NARGS + NBINOP + NUOP + NLBINOP + 2): return left->toString() + " <= " + right->toString();
    case (NARGS + NBINOP + NUOP + NLBINOP + 3): return left->toString() + " >= " + right->toString();
    case (NARGS + NBINOP + NUOP + NLBINOP + 4): return left->toString() + " == " + right->toString();
    case (NARGS + NBINOP + NUOP + NLBINOP + 5): return left->toString() + " != " + right->toString();
    default: abort();
    }
  }
  int countNode() { return left->countNode() + right->countNode() + 1; }
};

Logic* Logic::generate(int depth, Number *num) {
  if (depth >= DEPTHMAX) return new LConst(rand() & 1 ? true : false);
  depth++;
  int n = (int)(rand01() * (1 + NLBINOP + NCMPOP));

  if (n == 0) return new LConst(rand() & 1 ? true : false);
  if (n < 1 + NLBINOP) return new LBinOp(n - 1 + NARGS + NBINOP + NUOP, generate(depth, num), generate(depth, num));
  if (n < 1 + NLBINOP + NCMPOP) {
    Number *left, *right;
    left  = rand01() > 0.1 ? num->generate(depth) : new Const(0);
    right = rand01() > 0.1 ? num->generate(depth) : new Const(0);
    return new Compare(n - 1 + NARGS + NBINOP + NUOP, left, right);
  }

  abort();
}

int main(int argc, char **argv) {
  auto duration = std::chrono::system_clock::now().time_since_epoch();
  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  srand(millis);
  cout << "// Seed : " << millis << "\n";

  string fptype = "double";
  string booltype = "bool";

  if (argc > 1) fptype = argv[1];
  if (argc > 2) booltype = argv[2];
  if (argc > 3) asqrtfunc = argv[3];

  Number *x = new Number();
  Logic *y = new Logic();

  //

  cout << "#include <stdbool.h>\n";
  cout << "#include <math.h>\n\n";

  cout << "typedef double double2 __attribute__((ext_vector_type(2)));\n";
  cout << "typedef long int2 __attribute__((ext_vector_type(2)));\n";
  cout << "typedef float float4 __attribute__((ext_vector_type(4)));\n";
  cout << "typedef int int4 __attribute__((ext_vector_type(4)));\n\n";

  cout << "static __attribute__((always_inline)) double asqrt(double x) {\n";
  cout << "  return sqrt(fabs(x));\n";
  cout << "}\n\n";

  cout << "static __attribute__((always_inline)) float asqrtf(float x) {\n";
  cout << "  return sqrtf(fabsf(x));\n";
  cout << "}\n\n";

  cout << "static __attribute__((always_inline)) double2 asqrt2(double2 x) {\n";
  cout << "  return (double2) { sqrt(fabs(x[0])), sqrt(fabs(x[1])) };\n";
  cout << "}\n\n";

  cout << "static __attribute__((always_inline)) float4 asqrtf4(float4 x) {\n";
  cout << "  return (float4) { sqrtf(fabsf(x[0])), sqrtf(fabsf(x[1])), sqrtf(fabsf(x[2])), sqrtf(fabsf(x[3])) };\n";
  cout << "}\n\n";

  cout << "#ifdef TEST\n";

  for(int i=0;i<NTEST;i++) cout << "#define f" << i << "c f" << i << "t\n";
  cout << "#endif\n\n";

  for(int i=0;i<NTEST;i++) {
    cout << booltype << " f" << i << "c(" << fptype << " a0, " << fptype <<
      " a1, " << fptype << " a2, " << fptype << " a3) {\n";
    cout << "  return ";

    Logic *l;
    for(;;) {
      l = y->generate(0, x);
      if (!l->check()) continue;
      if (l->countNode() > 20) break;
    }
    cout << l->toString() << ";\n";
    
    cout << "}\n\n";
  }
}
