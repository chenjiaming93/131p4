; ModuleID = 'Checkpoint/assign2.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@a = global <2 x float> zeroinitializer

define float @assign() {
entry:
  %b = alloca <2 x float>
  %a = load <2 x float>* @a
  %0 = shufflevector <2 x float> %a, <2 x float> %a, <2 x i32> <i32 0, i32 1>
  %1 = insertelement <2 x float> undef, float 3.000000e+00, i32 0
  %2 = insertelement <2 x float> undef, float 3.000000e+00, i32 1
  %b1 = load <2 x float>* %b
  store <2 x float> undef, <2 x float>* %b
  %b2 = load <2 x float>* %b
  %3 = extractelement <2 x float> %b2, i32 0
  ret float %3
}
