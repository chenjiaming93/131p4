; ModuleID = 'vect_assign_partial2.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@a = global <2 x float> zeroinitializer

define float @assignvec(float %f1) {
entry:
  %f = alloca float
  store float %f1, float* %f
  %v4 = alloca <4 x float>
  %a = load <2 x float>* @a
  %v42 = load <4 x float>* %v4
  %0 = shufflevector <4 x float> %v42, <4 x float> %v42, <2 x i32> <i32 0, i32 1>
  %1 = insertelement <2 x i32> <i32 0, i32 1>, <2 x float> %a, i32 0
  %2 = insertelement <2 x i32> <i32 0, i32 1>, <2 x float> %a, i32 1
  store volatile <2 x i32> %2, <4 x float>* %v4
  %v43 = load <4 x float>* %v4
  %3 = extractelement <4 x float> %v43, i32 0
  ret float %3
}
