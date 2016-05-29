; ModuleID = 'Checkpoint/vect_assign_partial.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@a = global <2 x float> zeroinitializer

define float @assignvec(float %f1) {
entry:
  %f = alloca float
  store float %f1, float* %f
  %t = alloca float
  %v4 = alloca <4 x float>
  %f2 = load float* %f
  %0 = fadd float %f2, 5.000000e-01
  %t3 = load float* %t
  store float %0, float* %t
  %a = load <2 x float>* @a
  %v44 = load <4 x float>* %v4
  %1 = shufflevector <4 x float> %v44, <4 x float> %v44, <2 x i32> <i32 0, i32 1>
  %2 = load <4 x float>* %v4
  %3 = extractelement <2 x float> %a, i32 0
  %4 = insertelement <4 x float> %2, float %3, i32 0
  store volatile <4 x float> %4, <4 x float>* %v4
  %5 = extractelement <2 x float> %a, i32 1
  %6 = insertelement <4 x float> %2, float %5, i32 1
  store volatile <4 x float> %6, <4 x float>* %v4
  %t5 = load float* %t
  %v46 = load <4 x float>* %v4
  %7 = extractelement <4 x float> %v46, i32 3
  %8 = load <4 x float>* %v4
  %9 = insertelement <4 x float> %8, float %t5, i32 3
  store volatile <4 x float> %9, <4 x float>* %v4
  %t7 = load float* %t
  %v48 = load <4 x float>* %v4
  %10 = extractelement <4 x float> %v48, i32 2
  %11 = load <4 x float>* %v4
  %12 = insertelement <4 x float> %11, float %t7, i32 2
  store volatile <4 x float> %12, <4 x float>* %v4
  %v49 = load <4 x float>* %v4
  %13 = extractelement <4 x float> %v49, i32 3
  %v410 = load <4 x float>* %v4
  %14 = extractelement <4 x float> %v410, i32 2
  %v411 = load <4 x float>* %v4
  %15 = extractelement <4 x float> %v411, i32 1
  %v412 = load <4 x float>* %v4
  %16 = extractelement <4 x float> %v412, i32 0
  %17 = fadd float %16, %15
  %18 = fadd float %17, %14
  %19 = fadd float %18, %13
  ret float %19
}
