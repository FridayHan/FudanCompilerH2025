.section .note.GNU-stack

@ Here is the RPI code

@ Here's function: _^main^_^main

.balign 4
.global main
.section .text

main:
         push {r4-r10, fp, lr}
         add fp, sp, #32
main$L116: 
         mov r0, #44
         bl malloc
         mov r5, r0
         mov r0, #10
         str r0, [r5]
         add r0, r5, #4
         mov r1, #1
         str r1, [r0]
         add r0, r5, #8
         mov r1, #2
         str r1, [r0]
         add r0, r5, #12
         mov r1, #3
         str r1, [r0]
         add r0, r5, #16
         mov r1, #4
         str r1, [r0]
         add r0, r5, #20
         mov r1, #5
         str r1, [r0]
         add r0, r5, #24
         mov r1, #6
         str r1, [r0]
         add r0, r5, #28
         mov r1, #7
         str r1, [r0]
         add r0, r5, #32
         mov r1, #8
         str r1, [r0]
         add r0, r5, #36
         mov r1, #9
         str r1, [r0]
         add r0, r5, #40
         mov r1, #10
         str r1, [r0]
         mov r0, #16
         bl malloc
         mov r1, #3
         str r1, [r0]
         add r1, r0, #4
         mov r2, #3
         str r2, [r1]
         add r1, r0, #8
         mov r2, #4
         str r2, [r1]
         add r1, r0, #12
         mov r2, #5
         str r2, [r1]
         mov r1, r5
         ldr r1, [r5]
         ldr r1, [r0]
         mov r2, #0
         cmp r2, r1
         bge main$L100
main$L101: 
         mov r2, #0
         add r2, r2, #1
         mov r3, #4
         mul r2, r2, r3
         add r2, r0, r2
         ldr r2, [r2]
         add r2, r2, #1
         mov r3, #4
         mul r2, r2, r3
         add r1, r1, r2
         ldr r1, [r1]
         mov r2, #1
         cmp r1, r2
         blt main$L106
main$L107: 
main$L108: 
         mov r1, #0
         mov r3, r1
         mov r2, #9
         mov r1, #10
         cmp r2, r1
         bgt main$L111
main$L112: 
         mov r1, r3
         mov r2, r0
         ldr r0, [r0]
         cmp r1, r0
         bge main$L114
main$L115: 
         add r0, r1, #1
         mov r1, #4
         mul r0, r0, r1
         add r0, r2, r0
         ldr r0, [r0]
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L100: 
         mov r0, #-1
         bl exit
main$L102: 
         mov r0, #-1
         bl exit
main$L103: 
         ldr r1, [r0]
         mov r2, #0
         cmp r2, r1
         bge main$L100
         b main$L101
main$L106: 
         mov r0, r5
         b main$L108
main$L111: 
         mov r1, #1
         mov r3, r1
         b main$L112
main$L114: 
         mov r0, #-1
         bl exit

.global malloc
.global getint
.global putint
.global putch
.global putarray
.global getch
.global getarray
.global starttime
.global stoptime
