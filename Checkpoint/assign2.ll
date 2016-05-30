; ModuleID = 'Checkpoint/assign2.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-redhat-linux-gnu"

@a = global <2 x float> zeroinitializer

define float @assign() {
entry:
  %v4 = alloca <4 x float>
  %v41 = load <4 x float>* %v4
  %0 = extractelement <4 x float> %v41, i32 1
  %1 = load <4 x float>* %v4
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  store volatile <4 x float> %2, <4 x float>* %v4
  %a = load <2 x float>* @a
  %3 = extractelement <2 x float> %a, i32 0
  %v42 = load <4 x float>* %v4
  %4 = extractelement <4 x float> %v42, i32 1
  %5 = load <2 x float>* @a
  %6 = load <4 x float>* %v4
  %7 = extractelement <2 x float> %5, i32 0
  %8 = extractelement <4 x float> %6, i32 0
  %FAtoFA = fadd float %7, %8
  %9 = load <4 x float>* %v4
  %10 = insertelement <4 x float> %9, float %FAtoFA, i32 1
  store volatile <4 x float> %10, <4 x float>* %v4
  %v43 = load <4 x float>* %v4
  %11 = extractelement <4 x float> %v43, i32 1
  ret float %11
}
