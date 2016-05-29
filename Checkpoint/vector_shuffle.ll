; ModuleID = 'vector_shuffle.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@v2 = global <2 x float> zeroinitializer

define float @foo(float %f1) {
entry:
  %f = alloca float
  store float %f1, float* %f
  %t = alloca float
  %f2 = load float* %f
  %v2 = load <2 x float>* @v2
  %0 = shufflevector <2 x float> %v2, <2 x float> %v2, <2 x i32> <i32 1, i32 0>
  %1 = extractelement <2 x float> %0, i32 0
  %2 = fadd float %1, %f2
  %t3 = load float* %t
  store float %2, float* %t
  %t4 = load float* %t
  ret float %t4
}
