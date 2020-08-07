; ModuleID = 'check.c'
source_filename = "check.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"

; Function Attrs: norecurse nounwind readnone uwtable
define dso_local double @reduction_0(double %0, double %1, double %2, double %3) local_unnamed_addr #0 {
; CHECK_LABEL: @reduction_0
  %5 = fdiv fast double %0, %1
  %6 = fdiv fast double %2, %3
  %7 = fadd fast double %6, %5
; CHECK-COUNT-1: fdiv
  ret double %7
}

; Function Attrs: norecurse nounwind readnone uwtable
define dso_local double @reduction_1(double %0, double %1, double %2, double %3, double %4, double %5) local_unnamed_addr #0 {
; CHECK_LABEL: @reduction_1
  %7 = fdiv fast double %0, %1
  %8 = fdiv fast double %2, %3
  %9 = fadd fast double %8, %7
  %10 = fdiv fast double %4, %5
  %11 = fadd fast double %9, %10
; CHECK-COUNT-1: fdiv
  ret double %11
}

; Function Attrs: norecurse nounwind readnone uwtable
define dso_local zeroext i1 @cmpdiv_0(double %0, double %1, double %2, double %3) local_unnamed_addr #0 {
; CHECK_LABEL: @cmp_div_0
  %5 = fdiv fast double %0, %1
  %6 = fadd fast double %5, %2
  %7 = fcmp fast olt double %6, %3
; CHECK-NOT: fdiv
  ret i1 %7
}

; Function Attrs: norecurse nounwind readnone uwtable
define dso_local zeroext i1 @cmpdiv_1(double %0, double %1, double %2, double %3, double %4, double %5) local_unnamed_addr #0 {
; CHECK_LABEL: @cmp_div_1
  %7 = fdiv fast double %0, %1
  %8 = fdiv fast double %2, %3
  %9 = fadd fast double %8, %7
  %10 = fdiv fast double %4, %5
  %11 = fcmp fast olt double %9, %10
; CHECK-NOT: fdiv
  ret i1 %11
}

; Function Attrs: nounwind readnone uwtable
define dso_local zeroext i1 @cmpsqrt_0(double %0, double %1) local_unnamed_addr #1 {
; CHECK_LABEL: @cmpsqrt_0
  %3 = call fast double @llvm.sqrt.f64(double %0)
  %4 = fmul fast double %3, 1.100000e+00
  %5 = fadd fast double %4, 2.200000e+00
  %6 = fcmp fast olt double %5, %1
; CHECK-NOT: sqrt.f64
  ret i1 %6
}

; Function Attrs: nounwind readnone speculatable willreturn
declare double @llvm.sqrt.f64(double) #2

; Function Attrs: nounwind readnone uwtable
define dso_local zeroext i1 @cmpsqrt_1(double %0, double %1, double %2, double %3, double %4) local_unnamed_addr #1 {
; CHECK_LABEL: @cmpsqrt_1
  %6 = fmul fast double %4, %3
  %7 = call fast double @llvm.sqrt.f64(double %2)
  %8 = fmul fast double %7, %1
  %9 = fsub fast double %0, %8
  %10 = fcmp fast olt double %6, %9
; CHECK-NOT: sqrt.f64
  ret i1 %10
}

; Function Attrs: nounwind readnone uwtable
define dso_local zeroext i1 @cmpsqrt_2(double %0, double %1) local_unnamed_addr #1 {
; CHECK_LABEL: @cmpsqrt_2
  %3 = call fast double @llvm.sqrt.f64(double %0)
  %4 = fmul fast double %3, 1.230000e+00
  %5 = fcmp fast olt double %4, %1
; CHECK-NOT: sqrt.f64
  ret i1 %5
}

; Function Attrs: nounwind readnone uwtable
define dso_local zeroext i1 @complex_0(double %0, double %1, double %2, double %3, double %4, double %5) local_unnamed_addr #1 {
; CHECK_LABEL: @complex_0
  %7 = call fast double @llvm.sqrt.f64(double %1)
  %8 = fdiv fast double %0, %7
  %9 = fadd fast double %8, %2
  %10 = fcmp fast olt double %9, %3
; CHECK-NOT: fdiv
; CHECK-NOT: sqrt.f64
  ret i1 %10
}

; Function Attrs: nounwind readnone uwtable
define dso_local zeroext i1 @complex_1(double %0, double %1, double %2, double %3, double %4, double %5) local_unnamed_addr #1 {
; CHECK_LABEL: @complex_1
  %7 = fmul fast double %1, 2.300000e+00
  %8 = fdiv fast double %0, %7
  %9 = call fast double @llvm.sqrt.f64(double %8)
  %10 = fmul fast double %9, 1.200000e+00
  %11 = fcmp fast olt double %10, %2
; CHECK-NOT: fdiv
; CHECK-NOT: sqrt.f64
  ret i1 %11
}

; Function Attrs: nounwind readnone uwtable
define dso_local zeroext i1 @complex_2(double %0, double %1) local_unnamed_addr #1 {
; CHECK_LABEL: @complex_2
  %3 = call fast double @llvm.sqrt.f64(double %0)
  %4 = fdiv fast double 1.000000e+00, %3
  %5 = call fast double @llvm.sqrt.f64(double %1)
  %6 = fdiv fast double 1.000000e+00, %5
  %7 = fcmp fast olt double %4, %6
; CHECK-NOT: fdiv
; CHECK-NOT: sqrt.f64
  ret i1 %7
}

; Function Attrs: nounwind readnone uwtable
define dso_local zeroext i1 @complex_3(double %0, double %1, double %2, double %3, double %4, double %5) local_unnamed_addr #1 {
; CHECK_LABEL: @complex_3
  %7 = fdiv fast double %0, %1
  %8 = fadd fast double %2, 1.100000e+00
  %9 = call fast double @llvm.sqrt.f64(double %8)
  %10 = fadd fast double %9, %7
  %11 = fadd fast double %4, 1.200000e+00
  %12 = fdiv fast double %3, %11
  %13 = fadd fast double %5, 1.300000e+00
  %14 = fdiv fast double %13, %0
  %15 = fadd fast double %14, %12
  %16 = fcmp fast olt double %10, %15
; CHECK-NOT: fdiv
; CHECK-NOT: sqrt.f64
  ret i1 %16
}

; Function Attrs: nounwind readnone speculatable willreturn
declare <4 x float> @llvm.sqrt.v4f32(<4 x float>) #2

; Function Attrs: nounwind readnone uwtable
define dso_local <4 x i32> @complexv_1(<4 x float> %0, <4 x float> %1, <4 x float> %2, <4 x float> %3, <4 x float> %4, <4 x float> %5) local_unnamed_addr #3 {
; CHECK_LABEL: @complexv_1
  %7 = fmul fast <4 x float> %1, <float 0x4002666660000000, float 0x4002666660000000, float 0x4002666660000000, float 0x4002666660000000>
  %8 = fdiv fast <4 x float> %0, %7
  %9 = call fast <4 x float> @llvm.sqrt.v4f32(<4 x float> %8)
  %10 = fmul fast <4 x float> %9, <float 0x3FF3333340000000, float 0x3FF3333340000000, float 0x3FF3333340000000, float 0x3FF3333340000000>
  %11 = fcmp fast olt <4 x float> %10, %2
  %12 = sext <4 x i1> %11 to <4 x i32>
; CHECK-NOT: fdiv
; CHECK-NOT: llvm.sqrt
  ret <4 x i32> %12
}

; Function Attrs: nounwind readnone uwtable
define dso_local <4 x i32> @complexv_2(<4 x float> %0, <4 x float> %1) local_unnamed_addr #3 {
; CHECK_LABEL: @complexv_2
  %3 = call fast <4 x float> @llvm.sqrt.v4f32(<4 x float> %0)
  %4 = fdiv fast <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>, %3
  %5 = call fast <4 x float> @llvm.sqrt.v4f32(<4 x float> %1)
  %6 = fdiv fast <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>, %5
  %7 = fcmp fast olt <4 x float> %4, %6
  %8 = sext <4 x i1> %7 to <4 x i32>
; CHECK-NOT: fdiv
; CHECK-NOT: llvm.sqrt
  ret <4 x i32> %8
; CHECK: ret
}

attributes #0 = { norecurse nounwind readnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="true" "no-jump-tables"="false" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="true" "use-soft-float"="false" }
attributes #1 = { nounwind readnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="true" "no-jump-tables"="false" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="true" "use-soft-float"="false" }
attributes #2 = { nounwind readnone speculatable willreturn }
attributes #3 = { nounwind readnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="128" "no-infs-fp-math"="true" "no-jump-tables"="false" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="true" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0-4ubuntu1 "}
