EXTERN g_scaleExpectedFx:BYTE
EXTERN g_scaleActualFx:BYTE
EXTERN g_scaleActualRegs:QWORD
EXTERN g_scaleTestTarget:QWORD
EXTERN g_scaleActualNonvol:QWORD
EXTERN g_scaleCallbackAlignment:QWORD
PUBLIC gameRenderScaleTestInvoke
PUBLIC gameRenderScaleTestOriginal
PUBLIC gameRenderScaleTestClobber
.data
ALIGN 4
testMxcsr DWORD 03F80h
clobberMxcsr DWORD 01F80h
.code
gameRenderScaleTestInvoke PROC FRAME
    push rbx
    .pushreg rbx
    push rbp
    .pushreg rbp
    push rsi
    .pushreg rsi
    push rdi
    .pushreg rdi
    push r12
    .pushreg r12
    push r13
    .pushreg r13
    push r14
    .pushreg r14
    push r15
    .pushreg r15
    sub rsp, 248h
    .allocstack 248h
    .endprolog
    fxsave64 [rsp+40h]
    mov [g_scaleTestTarget], rcx
    fninit
    fldpi
    ldmxcsr [testMxcsr]
    pcmpeqd xmm0, xmm0
    movaps xmm1, xmm0
    movaps xmm2, xmm0
    movaps xmm3, xmm0
    movaps xmm4, xmm0
    movaps xmm5, xmm0
    fxsave64 [g_scaleExpectedFx]
    mov rbx, 12345000h
    mov rbp, 12345001h
    mov rsi, 12345002h
    mov rdi, 12345003h
    mov r12, 12345004h
    mov r13, 12345005h
    mov r14, 12345006h
    mov r15, 12345007h
    mov rcx, 12345678h
    mov rdx, 22334455h
    mov r8, 33445566h
    mov r9, 44556677h
    mov r10, 55667788h
    mov r11, 66778899h
    mov rax, 1122334455667788h
    mov [rsp+20h], rax
    push 0A47h
    popfq
    call qword ptr [g_scaleTestTarget]
    mov [g_scaleActualRegs+64], rax
    mov [g_scaleActualNonvol+0], rbx
    mov [g_scaleActualNonvol+8], rbp
    mov [g_scaleActualNonvol+16], rsi
    mov [g_scaleActualNonvol+24], rdi
    mov [g_scaleActualNonvol+32], r12
    mov [g_scaleActualNonvol+40], r13
    mov [g_scaleActualNonvol+48], r14
    mov [g_scaleActualNonvol+56], r15
    fxrstor64 [rsp+40h]
    add rsp, 248h
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    ret
gameRenderScaleTestInvoke ENDP

gameRenderScaleTestOriginal PROC
    mov [g_scaleActualRegs+0], rcx
    mov [g_scaleActualRegs+8], rdx
    mov [g_scaleActualRegs+16], r8
    mov [g_scaleActualRegs+24], r9
    mov [g_scaleActualRegs+32], r10
    mov [g_scaleActualRegs+40], r11
    pushfq
    pop rax
    mov [g_scaleActualRegs+48], rax
    mov rax, rsp
    and eax, 0Fh
    mov [g_scaleActualRegs+72], rax
    mov rax, [rsp+28h]
    mov [g_scaleActualRegs+56], rax
    fxsave64 [g_scaleActualFx]
    mov rax, 0AABBCCDDEEFF0011h
    ret
gameRenderScaleTestOriginal ENDP

gameRenderScaleTestClobber PROC
    mov rax, rsp
    and eax, 0Fh
    mov [g_scaleCallbackAlignment], rax
    ldmxcsr [clobberMxcsr]
    fninit
    pxor xmm0, xmm0
    pxor xmm1, xmm1
    pxor xmm2, xmm2
    pxor xmm3, xmm3
    pxor xmm4, xmm4
    pxor xmm5, xmm5
    xor eax, eax
    xor ecx, ecx
    xor edx, edx
    xor r8d, r8d
    xor r9d, r9d
    xor r10d, r10d
    xor r11d, r11d
    ret
gameRenderScaleTestClobber ENDP
END
