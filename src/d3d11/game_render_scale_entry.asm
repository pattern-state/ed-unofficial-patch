EXTERN g_gameRenderScaleOriginal:QWORD
EXTERN gameRenderScaleEntryObserved:PROC
PUBLIC gameRenderScaleEntry
.code
gameRenderScaleEntry PROC FRAME
    ; Save incoming flags before SUB changes them. ALLOCSTACK describes PUSHFQ's stack effect.
    push rbp
    .pushreg rbp
    pushfq
    .allocstack 8
    sub rsp, 2C8h
    .allocstack 2C8h
    mov rbp, rsp
    .setframe rbp, 0
    .endprolog
    mov [rsp+40h], rax
    mov [rsp+48h], rcx
    mov [rsp+50h], rdx
    mov [rsp+58h], r8
    mov [rsp+60h], r9
    mov [rsp+68h], r10
    mov [rsp+70h], r11
    fxsave64 [rsp+0C0h]
    call gameRenderScaleEntryObserved
    fxrstor64 [rsp+0C0h]
    mov rax, [rsp+40h]
    mov rcx, [rsp+48h]
    mov rdx, [rsp+50h]
    mov r8, [rsp+58h]
    mov r9, [rsp+60h]
    mov r10, [rsp+68h]
    mov r11, [rsp+70h]
    ; POPFQ runs before the conventional LEA/JMP epilogue; neither changes flags.
    push qword ptr [rsp+2C8h]
    popfq
    lea rsp, [rbp+2D0h]
    pop rbp
    jmp qword ptr [g_gameRenderScaleOriginal]
gameRenderScaleEntry ENDP
END
