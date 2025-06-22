.section .note.GNU-stack

@ Here is the RPI code

@ Here's function: _^main^_^main

.balign 4
.global main
.section .text

main:
         push {r4-r10, fp, lr}
         add fp, sp, #32
         sub sp, sp, #4
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
         mov r7, #0
         mov r0, #8
         bl malloc
         mov r5, r0
         mov r1, #2
         str r1, [r5, #0]
         add r0, r5, #4
         ldr r1, =c1$m1
         str r1, [r0]
         mov r0, #8
         bl malloc
         mov r4, r0
         mov r1, #2
         str r1, [r4, #0]
         add r0, r4, #4
         ldr r1, =c1$m1
         str r1, [r0]
         ldr r9, [fp, #-36]
         ldr r0, [r9]
         mov r6, r0
main$L102: 
         cmp r7, r6
         blt main$L103
main$L104: 
         ldr r9, [fp, #-36]
         mov r1, r9
         mov r0, r6
         bl putarray
         mov r0, r6
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L103: 
         mov r0, #2
         sdiv r0, r7, r0
         mov r1, #2
         mul r0, r0, r1
         cmp r0, r7
         beq main$L111
main$L112: 
         ldr r9, [fp, #-36]
         mov r0, r9
         ldr r9, [fp, #-36]
         ldr r1, [r9]
         cmp r7, r1
         bge main$L109
main$L110: 
         ldr r2, [r4, #4]
         add r1, r7, #1
         mov r3, #4
         mul r1, r1, r3
         add r8, r0, r1
         mov r1, r6
         mov r0, r4
         blx r2
         str r0, [r8]
main$L113: 
         add r0, r7, #1
         mov r7, r0
         b main$L102
main$L107: 
         mov r0, #-1
         bl exit
main$L108: 
         ldr r2, [r5, #4]
         add r1, r7, #1
         mov r3, #4
         mul r1, r1, r3
         add r8, r0, r1
         mov r1, r7
         mov r0, r5
         blx r2
         str r0, [r8]
         b main$L113
main$L109: 
         mov r0, #-1
         bl exit
main$L111: 
         ldr r9, [fp, #-36]
         mov r0, r9
         ldr r9, [fp, #-36]
         ldr r1, [r9]
         cmp r7, r1
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
