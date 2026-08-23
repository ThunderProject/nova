.intel_syntax noprefix

.text

.macro FUNC_BEGIN name
    .p2align 4
    .globl \name
    .hidden \name
    .type \name, @function
\name:
.endm

.macro FUNC_END name
    .size \name, .-\name
.endm

.macro DEFINE_LOAD_SCALAR name, loadop, loadreg, storereg, memtype
FUNC_BEGIN \name
.Lretry_\@:
    mov rax, QWORD PTR [rdi]
    test al, 1
    jnz .Lbusy_\@

    \loadop \loadreg, \memtype PTR [rsi]

    mov r8, QWORD PTR [rdi]
    cmp rax, r8
    jne .Lretry_\@
    mov \memtype PTR [rdx], \storereg
    ret
.Lbusy_\@:
    pause
    jmp .Lretry_\@
FUNC_END \name
.endm

.macro DEFINE_STORE_SCALAR name, loadop, loadreg, storereg, memtype
FUNC_BEGIN \name
    \loadop \loadreg, \memtype PTR [rdx]
    mov rax, QWORD PTR [rdi]

    inc rax
    mov QWORD PTR [rdi], rax

    mov \memtype PTR [rsi], \storereg

    inc rax
    mov QWORD PTR [rdi], rax
    ret
FUNC_END \name
.endm

DEFINE_LOAD_SCALAR  nova_seqlock_load_1,  movzx, ecx, cl,  BYTE
DEFINE_STORE_SCALAR nova_seqlock_store_1, movzx, ecx, cl,  BYTE

DEFINE_LOAD_SCALAR  nova_seqlock_load_2,  movzx, ecx, cx,  WORD
DEFINE_STORE_SCALAR nova_seqlock_store_2, movzx, ecx, cx,  WORD

DEFINE_LOAD_SCALAR  nova_seqlock_load_4,  mov,   ecx, ecx, DWORD
DEFINE_STORE_SCALAR nova_seqlock_store_4, mov,   ecx, ecx, DWORD

DEFINE_LOAD_SCALAR  nova_seqlock_load_8,  mov,   rcx, rcx, QWORD
DEFINE_STORE_SCALAR nova_seqlock_store_8, mov,   rcx, rcx, QWORD

.macro LOAD_XMMS base, count
    movdqu  xmm0, XMMWORD PTR [\base]
    .if \count >= 2
        movdqu xmm1, XMMWORD PTR [\base + 16]
    .endif

    .if \count >= 3
        movdqu xmm2, XMMWORD PTR [\base + 32]
    .endif

    .if \count >= 4
       movdqu xmm3, XMMWORD PTR [\base + 48]
    .endif

    .if \count >= 5
        movdqu xmm4, XMMWORD PTR [\base + 64]
    .endif

    .if \count >= 6
        movdqu xmm5, XMMWORD PTR [\base + 80]
    .endif

    .if \count >= 7
        movdqu xmm6, XMMWORD PTR [\base + 96]
    .endif

    .if \count >= 8
        movdqu xmm7, XMMWORD PTR [\base + 112]
    .endif
.endm

.macro STORE_XMMS base, count
    movdqu  XMMWORD PTR [\base], xmm0
    .if \count >= 2
        movdqu XMMWORD PTR [\base + 16], xmm1
    .endif

    .if \count >= 3
        movdqu XMMWORD PTR [\base + 32], xmm2
    .endif

    .if \count >= 4
        movdqu XMMWORD PTR [\base + 48], xmm3
    .endif

    .if \count >= 5
        movdqu XMMWORD PTR [\base + 64], xmm4
    .endif

    .if \count >= 6
        movdqu XMMWORD PTR [\base + 80], xmm5
    .endif

    .if \count >= 7
        movdqu XMMWORD PTR [\base + 96], xmm6
    .endif

    .if \count >= 8
        movdqu XMMWORD PTR [\base + 112], xmm7
    .endif
.endm

.macro DEFINE_LOAD_XMM name, count
FUNC_BEGIN \name
.Lretry_\@:
    mov     rax, QWORD PTR [rdi]

    test    al, 1
    jnz     .Lbusy_\@

    LOAD_XMMS rsi, \count
    mov     r8, QWORD PTR [rdi]
    cmp     rax, r8
    jne     .Lretry_\@
    STORE_XMMS rdx, \count
    ret
.Lbusy_\@:
    pause
    jmp     .Lretry_\@
FUNC_END \name
.endm

.macro DEFINE_STORE_XMM name, count
FUNC_BEGIN \name
    LOAD_XMMS rdx, \count
    mov     rax, QWORD PTR [rdi]
    inc     rax
    mov     QWORD PTR [rdi], rax
    STORE_XMMS rsi, \count
    inc     rax
    mov     QWORD PTR [rdi], rax
    ret
FUNC_END \name
.endm

DEFINE_LOAD_XMM  nova_seqlock_load_16,  1
DEFINE_STORE_XMM nova_seqlock_store_16, 1

DEFINE_LOAD_XMM  nova_seqlock_load_32,  2
DEFINE_STORE_XMM nova_seqlock_store_32, 2

DEFINE_LOAD_XMM  nova_seqlock_load_48,  3
DEFINE_STORE_XMM nova_seqlock_store_48, 3

DEFINE_LOAD_XMM  nova_seqlock_load_64,  4
DEFINE_STORE_XMM nova_seqlock_store_64, 4

DEFINE_LOAD_XMM  nova_seqlock_load_128,  8
DEFINE_STORE_XMM nova_seqlock_store_128, 8

FUNC_BEGIN nova_seqlock_load_24
.Lload24_retry:
    mov     rax, QWORD PTR [rdi]
    test    al, 1
    jnz     .Lload24_busy
    movdqu  xmm0, XMMWORD PTR [rsi]
    mov     rcx, QWORD PTR [rsi + 16]
    mov     r8, QWORD PTR [rdi]
    cmp     rax, r8
    jne     .Lload24_retry
    movdqu  XMMWORD PTR [rdx], xmm0
    mov     QWORD PTR [rdx + 16], rcx
    ret
.Lload24_busy:
    pause
    jmp     .Lload24_retry
FUNC_END nova_seqlock_load_24

FUNC_BEGIN nova_seqlock_store_24
    movdqu  xmm0, XMMWORD PTR [rdx]
    mov     rcx, QWORD PTR [rdx + 16]
    mov     rax, QWORD PTR [rdi]
    inc     rax
    mov     QWORD PTR [rdi], rax
    movdqu  XMMWORD PTR [rsi], xmm0
    mov     QWORD PTR [rsi + 16], rcx
    inc     rax
    mov     QWORD PTR [rdi], rax
    ret
FUNC_END nova_seqlock_store_24

FUNC_BEGIN nova_seqlock_load_generic
    mov     r8, rdi
    mov     r9, rsi
    mov     r10, rdx
    mov     r11, rcx

.Lload_generic_retry:
    mov     rax, QWORD PTR [r8]
    test    al, 1
    jnz     .Lload_generic_busy
    mov     rsi, r9
    mov     rdi, r10
    mov     rcx, r11
    rep movsb
    mov     rdx, QWORD PTR [r8]
    cmp     rax, rdx
    jne     .Lload_generic_retry
    ret
.Lload_generic_busy:
    pause
    jmp     .Lload_generic_retry
FUNC_END nova_seqlock_load_generic

FUNC_BEGIN nova_seqlock_store_generic
    mov     r8, rdi
    mov     r9, rsi
    mov     r10, rdx
    mov     r11, rcx
    mov     rax, QWORD PTR [r8]
    inc     rax
    mov     QWORD PTR [r8], rax
    mov     rsi, r10
    mov     rdi, r9
    mov     rcx, r11
    rep movsb

    inc     rax
    mov     QWORD PTR [r8], rax

    ret
FUNC_END nova_seqlock_store_generic

.section .note.GNU-stack,"",@progbits