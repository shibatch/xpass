#!/bin/sh
set -e
BUILDDIR=build
counter=0
while true; do
    $BUILDDIR/bin/mp_gentest double bool asqrt > $BUILDDIR/mp_generated_DOUBLE.c
    $BUILDDIR/bin/mp_gentest float bool asqrtf > $BUILDDIR/mp_generated_FLOAT.c
    $BUILDDIR/bin/mp_gentest double2 int2 asqrt2 > $BUILDDIR/mp_generated_DOUBLE2.c
    $BUILDDIR/bin/mp_gentest float4 int4 asqrtf4 > $BUILDDIR/mp_generated_FLOAT4.c

    for type in DOUBLE FLOAT DOUBLE2 FLOAT4; do
	clang-10 -DTEST -ffast-math -O1 $BUILDDIR/mp_generated_$type.c -emit-llvm -S -o $BUILDDIR/mp_generated_$type.ll
	opt-10 -O1 -load=$BUILDDIR/lib/libMathPeephole.so -ffast-math -lint $BUILDDIR/mp_generated_$type.ll -o $BUILDDIR/mp_generated_$type.bc
	clang-10 -ffast-math -O3 $BUILDDIR/mp_generated_$type.bc -c -o $BUILDDIR/mp_generated_t_$type.o
	clang-10 -ffast-math -O1 $BUILDDIR/mp_generated_$type.c -c -o $BUILDDIR/mp_generated_c_$type.o
	clang-10 -D$type tester/mp_tester.c $BUILDDIR/mp_generated_t_$type.o $BUILDDIR/mp_generated_c_$type.o -o $BUILDDIR/mp_tester_$type -lm
	$BUILDDIR/mp_tester_$type
	counter=$((counter+1))
	echo $counter
    done
done
