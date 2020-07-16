#!/bin/sh
set -e
BUILDDIR=build
counter=0
while true; do
    $BUILDDIR/bin/mp_gentest double bool asqrt > $BUILDDIR/mp_generated_"$$"_DOUBLE.c
    $BUILDDIR/bin/mp_gentest float bool asqrtf > $BUILDDIR/mp_generated_"$$"_FLOAT.c
    $BUILDDIR/bin/mp_gentest double2 int2 asqrt2 > $BUILDDIR/mp_generated_"$$"_DOUBLE2.c
    $BUILDDIR/bin/mp_gentest float4 int4 asqrtf4 > $BUILDDIR/mp_generated_"$$"_FLOAT4.c

    for type in DOUBLE FLOAT DOUBLE2 FLOAT4; do
	clang-10 -DTEST -ffast-math -O3 $BUILDDIR/mp_generated_"$$"_$type.c -emit-llvm -S -o $BUILDDIR/mp_generated_"$$"_$type.ll
	opt-10 -O3 -load=$BUILDDIR/lib/libMathPeephole.so -ffast-math -lint $BUILDDIR/mp_generated_"$$"_$type.ll -o $BUILDDIR/mp_generated_"$$"_$type.bc
	clang-10 -ffast-math -O3 $BUILDDIR/mp_generated_"$$"_$type.bc -c -o $BUILDDIR/mp_generated_t_"$$"_$type.o
	clang-10 -ffast-math -O3 $BUILDDIR/mp_generated_"$$"_$type.c -c -o $BUILDDIR/mp_generated_c_"$$"_$type.o
	clang-10 -D$type tester/mp_tester.c $BUILDDIR/mp_generated_t_"$$"_$type.o $BUILDDIR/mp_generated_c_"$$"_$type.o -o $BUILDDIR/mp_tester_"$$"_$type -lm
	$BUILDDIR/mp_tester_"$$"_$type
	counter=$((counter+1))
	echo "$$" $counter
    done
done
