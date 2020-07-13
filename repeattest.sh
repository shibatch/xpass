#!/bin/sh
set -e
BUILDDIR=build
while true; do
    $BUILDDIR/bin/mp_gentest > $BUILDDIR/mp_generated.c
    clang-10 -DTEST -ffast-math -O1 $BUILDDIR/mp_generated.c -emit-llvm -S -o $BUILDDIR/mp_generated.ll
    opt-10 -O1 -load=$BUILDDIR/lib/libMathPeephole.so -ffast-math -lint $BUILDDIR/mp_generated.ll -o $BUILDDIR/mp_generated.bc
    clang-10 -ffast-math -O3 $BUILDDIR/mp_generated.bc -c -o $BUILDDIR/mp_generated_t.o
    clang-10 -ffast-math -O1 $BUILDDIR/mp_generated.c -c -o $BUILDDIR/mp_generated_c.o
    clang-10 tester/mp_tester.c $BUILDDIR/mp_generated_t.o $BUILDDIR/mp_generated_c.o -o $BUILDDIR/mp_tester -lm
    $BUILDDIR/mp_tester
done
