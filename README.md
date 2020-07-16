[![Build Status](https://travis-ci.org/shibatch/xpass.svg?branch=master)](https://travis-ci.org/shibatch/xpass)

# XPASS - Experimental LLVM passes

This is a collection of experimental optimizing passes for LLVM. It
includes two plugins. The first one is SubstSleef, which rewrites
calls to the standard math library to those to SLEEF. The second one
is MathPeephole, which performs dangerous math optimizations.

This project is based on the llvm-tutor package developed by Andrzej
Warzyński.


## Build

```
sudo apt-get install clang-10 llvm-10 llvm-10-dev llvm-10-tools libstdc++-10-dev cmake
mkdir build
cd build
cmake -DLLVM_DIR=/usr/lib/llvm-10 ..
make
```

## Running the passes

```
clang-10 -Xclang -load -Xclang libMathPeephole.so -ffast-math -O3 example.c
clang-10 -Xclang -load -Xclang libSubstSleef.so -fno-math-errno -O3 example.c
```

## SubstSleef transform pass

This pass substitutes vector calls to intrinsic math functions to the
equivalent calls to SLEEF functions. In order to make this transform
to happen, you need to specify "-O1 -fno-math-errno" flag. This pass
only substitutes vector math calls to calls to SLEEF functions. You
can compile test-substsleef.c under SubstSleef directory with the
following command.

```
clang-10 -march=skylake -Xclang -load -Xclang build/lib/libSubstSleef.so -O3 -fno-math-errno test-substsleef.c -emit-llvm -S
```

You can see many vector calls to SLEEF library in test-substsleef.ll.


## MathPeephole transform pass

This pass performs the following transforms.

* a/b + c/d  ->  (ad + bc) / (bd)
* x/y + z > a/b + c  ->  (xb - ay) / (yb) + z > c
* a/b + c > d  ->  b < 0 ^ a > b(d - c)
* w sqrt(x) + y > z  ->  (z < y) ^ (((w >= 0) ^ (z < y)) && (wwx > (z-y)(z-y)))


Please see [wiki](https://github.com/shibatch/xpass/wiki) for examples of the transforms.
