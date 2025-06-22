.section .note.GNU-stack

@ Here is the RPI code

@ Here's function: _^main^_^main

.balign 4
.global main
.section .text

main:
         push {r4-r10, fp, lr}
         add fp, sp, #32
         sub sp, sp, #24
main$L100: 
         mov r6, #3
         mov r9, #4
         str r9, [fp, #-36]
         mov r9, #5
         str r9, [fp, #-40]
         mov r9, #6
         str r9, [fp, #-44]
         mov r9, #7
         str r9, [fp, #-48]
         mov r4, #8
         mov r9, #9
         str r9, [fp, #-52]
         mov r5, #10
         mov r3, #11
         mov r2, #0
         mov r1, #1
         mov r0, #2
         add r7, r2, r1
         add r7, r7, r0
         add r7, r7, r6
         ldr r10, [fp, #-36]
         add r7, r7, r10
         ldr r10, [fp, #-40]
         add r7, r7, r10
         ldr r10, [fp, #-44]
         add r7, r7, r10
         ldr r10, [fp, #-48]
         add r7, r7, r10
         add r7, r7, r4
         ldr r10, [fp, #-52]
         add r7, r7, r10
         add r7, r7, r5
         add r9, r7, r3
         str r9, [fp, #-56]
         ldr r9, [fp, #-56]
         mul r8, r9, r2
         ldr r9, [fp, #-56]
         mul r7, r9, r1
         add r8, r8, r7
         ldr r9, [fp, #-56]
         mul r7, r9, r0
         add r7, r8, r7
         ldr r9, [fp, #-56]
         mul r6, r9, r6
         add r7, r7, r6
         ldr r9, [fp, #-56]
         ldr r10, [fp, #-36]
         mul r6, r9, r10
         add r7, r7, r6
         ldr r9, [fp, #-56]
         ldr r10, [fp, #-40]
         mul r6, r9, r10
         add r7, r7, r6
         ldr r9, [fp, #-56]
         ldr r10, [fp, #-44]
         mul r6, r9, r10
         add r7, r7, r6
         ldr r9, [fp, #-56]
         ldr r10, [fp, #-48]
         mul r6, r9, r10
         add r6, r7, r6
         ldr r9, [fp, #-56]
         mul r4, r9, r4
         add r6, r6, r4
         ldr r9, [fp, #-56]
         ldr r10, [fp, #-52]
         mul r4, r9, r10
         add r6, r6, r4
         ldr r9, [fp, #-56]
         mul r4, r9, r5
         add r6, r6, r4
         ldr r9, [fp, #-56]
         mul r4, r9, r3
         add r4, r6, r4
         add r1, r2, r1
         ldr r10, [fp, #-40]
         sdiv r0, r0, r10
         add r0, r1, r0
         mul r1, r3, r5
         add r0, r0, r1
         bl putint
         ldr r9, [fp, #-56]
         mov r0, r9
         bl putint
         mov r0, r4
         bl putint
         ldr r9, [fp, #-48]
         ldr r10, [fp, #-44]
         mul r0, r9, r10
         ldr r9, [fp, #-36]
         add r0, r9, r0
         ldr r10, [fp, #-52]
         sub r0, r0, r10
         sub sp, fp, #32
         pop {r4-r10, fp, pc}

.global malloc
.global getint
.global putint
.global putch
.global putarray
.global getch
.global getarray
.global starttime
.global stoptime
