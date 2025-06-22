.section .note.GNU-stack

@ Here is the RPI code

@ Here's function: _^main^_^main

.balign 4
.global main
.section .text

main:
         push {r4-r10, fp, lr}
         add fp, sp, #32
         sub sp, sp, #20
main$L114: 
         mov r0, #32
         bl malloc
         mov r9, r0
         str r9, [fp, #-36]
         mov r0, #7
         ldr r10, [fp, #-36]
         str r0, [r10]
         ldr r9, [fp, #-36]
         add r0, r9, #4
         mov r1, #1
         str r1, [r0]
         ldr r9, [fp, #-36]
         add r0, r9, #8
         mov r1, #2
         str r1, [r0]
         ldr r9, [fp, #-36]
         add r0, r9, #12
         mov r1, #3
         str r1, [r0]
         ldr r9, [fp, #-36]
         add r0, r9, #16
         mov r1, #4
         str r1, [r0]
         ldr r9, [fp, #-36]
         add r0, r9, #20
         mov r1, #5
         str r1, [r0]
         ldr r9, [fp, #-36]
         add r0, r9, #24
         mov r1, #6
         str r1, [r0]
         ldr r9, [fp, #-36]
         add r0, r9, #28
         mov r1, #7
         str r1, [r0]
         mov r4, #0
         mov r0, #8
         bl malloc
         mov r9, r0
         str r9, [fp, #-48]
         mov r1, #2
         ldr r9, [fp, #-48]
         str r1, [r9, #0]
         ldr r9, [fp, #-48]
         add r0, r9, #4
         ldr r1, =c1$m1
         str r1, [r0]
         mov r0, #8
         bl malloc
         mov r8, r0
         mov r1, #2
         str r1, [r8, #0]
         add r0, r8, #4
         ldr r1, =c1$m1
         str r1, [r0]
         ldr r9, [fp, #-36]
         ldr r0, [r9]
         mov r9, r0
         str r9, [fp, #-44]
         mov r9, r4
         str r9, [fp, #-40]
main$L102: 
         ldr r9, [fp, #-40]
         ldr r10, [fp, #-44]
         cmp r9, r10
         blt main$L103
main$L104: 
         ldr r9, [fp, #-36]
         mov r1, r9
         ldr r9, [fp, #-44]
         mov r0, r9
         bl putarray
         ldr r9, [fp, #-44]
         mov r0, r9
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L103: 
         mov r0, #2
         ldr r9, [fp, #-40]
         sdiv r0, r9, r0
         mov r1, #2
         mul r0, r0, r1
         ldr r10, [fp, #-40]
         cmp r0, r10
         beq main$L111
main$L112: 
         ldr r9, [fp, #-36]
         mov r4, r9
         ldr r9, [fp, #-36]
         ldr r0, [r9]
         ldr r9, [fp, #-40]
         cmp r9, r0
         bge main$L109
main$L110: 
         ldr r0, [r8, #4]
         mov r9, r0
         str r9, [fp, #-52]
         mov r0, r8
         ldr r9, [fp, #-40]
         mov r1, r9
         mov r2, r5
         mov r3, r5
         mov r6, #0
         sub r6, r6, #2
         mov r7, #2
         mul r6, r6, r7
         mov r6, #0
         sub r6, r6, #4
         ldr r9, [fp, #-40]
         add r6, r9, #1
         mov r7, #4
         mul r6, r6, r7
         add r4, r4, r6
         ldr r9, [fp, #-52]
         blx r9
         str r0, [r4]
main$L113: 
         ldr r9, [fp, #-40]
         add r0, r9, #1
         mov r9, r0
         str r9, [fp, #-40]
         b main$L102
main$L107: 
         mov r0, #-1
         bl exit
main$L108: 
         ldr r9, [fp, #-48]
         ldr r2, [r9, #4]
         ldr r9, [fp, #-40]
         add r0, r9, #1
         mov r1, #4
         mul r0, r0, r1
         add r4, r3, r0
         ldr r9, [fp, #-40]
         mov r1, r9
         ldr r9, [fp, #-48]
         mov r0, r9
         blx r2
         str r0, [r4]
         b main$L113
main$L109: 
         mov r0, #-1
         bl exit
main$L111: 
         ldr r9, [fp, #-36]
         mov r3, r9
         ldr r9, [fp, #-36]
         ldr r0, [r9]
         ldr r9, [fp, #-40]
         cmp r9, r0
         bge main$L107
         b main$L108

@ Here's function: c1^m1

.balign 4
.global c1$m1
.section .text

c1$m1:
         push {r4-r10, fp, lr}
         add fp, sp, #32
c1$m1$L100: 
         ldr r0, [r0, #0]
         sub sp, fp, #32
         pop {r4-r10, fp, pc}

@ Here's function: c2^m1

.balign 4
.global c2$m1
.section .text

c2$m1:
         push {r4-r10, fp, lr}
         add fp, sp, #32
c2$m1$L100: 
         ldr r0, [r0, #0]
         add r0, r0, r1
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
