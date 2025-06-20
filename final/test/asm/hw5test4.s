.section .note.GNU-stack

@ Here is the RPI code

@ Here's function: _^main^_^main

.balign 4
.global main
.section .text

main:
         push {r4-r10, fp, lr}
         add fp, sp, #32
main$L107: 
         mov r0, #20
         bl malloc
         mov r1, #4
         str r1, [r0]
         add r1, r0, #4
         mov r2, #1
         str r2, [r1]
         add r1, r0, #8
         mov r2, #2
         str r2, [r1]
         add r1, r0, #12
         mov r2, #3
         str r2, [r1]
         add r1, r0, #16
         mov r2, #4
         str r2, [r1]
         mov r3, #0
         mov r1, #0
         mov r5, r1
main$L102: 
         mov r1, #4
         cmp r2, r1
         blt main$L103
main$L104: 
         mov r0, r5
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L103: 
         mov r1, r5
         ldr r4, [r0]
         cmp r2, r4
         bge main$L105
main$L106: 
         add r4, r2, #1
         mov r5, #4
         mul r4, r4, r5
         add r4, r0, r4
         ldr r4, [r4]
         add r1, r1, r4
         add r4, r2, #1
         mov r5, r1
         b main$L102
main$L105: 
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
