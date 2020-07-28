; ModuleID = 'check.c'
source_filename = "check.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: norecurse nounwind readnone uwtable
;
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
; CHECK: declare double @llvm.sqrt.f64

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
; CHECK: ret
  ret i1 %11
}

attributes #0 = { norecurse nounwind readnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="true" "no-jump-tables"="false" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="true" "use-soft-float"="false" }
attributes #1 = { nounwind readnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="true" "no-jump-tables"="false" "no-nans-fp-math"="true" "no-signed-zeros-fp-math"="true" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="true" "use-soft-float"="false" }
attributes #2 = { nounwind readnone speculatable willreturn }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0-4ubuntu1 "}
