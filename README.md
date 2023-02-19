[![Build Status](https://travis-ci.org/shibatch/xpass.svg?branch=master)](https://travis-ci.org/shibatch/xpass)

# XPASS - An experimental LLVM pass

This is an experimental optimizing pass for LLVM, named
MathPeephole. This pass performs the following transforms.

* a/b + c/d  ->  (ad + bc) / (bd)
* x/y + z > a/b + c  ->  (xb - ay) / (yb) + z > c
* a/b + c > d  ->  b < 0 ? (a - b * (d - c) < 0) : (-(a - b * (d - c)) < 0)
* w sqrt(x) + y > z  ->  w >= 0 ? ((z < y) | (wwx > (z-y)(z-y))) : ((z <= y) & (wwx < (z-y)(z-y)))


This project is based on the llvm-tutor package developed by Andrzej
Warzyński.

## Running the passes

```
clang-14 -S -emit-llvm -O1 -ffast-math example.c -o example.ll
opt -load-pass-plugin=libMathPeephole.so -passes=math-peephole example.ll -S -o example.opt.ll
clang-14 -O1 -ffast-math example.opt.ll -o example
```

Please see [wiki](https://github.com/shibatch/xpass/wiki) for examples of the transforms.
