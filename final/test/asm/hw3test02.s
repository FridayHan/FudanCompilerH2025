.section .note.GNU-stack

@ Here is the RPI code

@ Here's function: _^main^_^main

.balign 4
.global main
.section .text

main:
         push {r4-r10, fp, lr}
         add fp, sp, #32
main$L102: 
         mov r0, #12
         bl malloc
         mov r1, #2
         str r1, [r0]
         add r1, r0, #4
         mov r2, #1
         str r2, [r1]
         add r1, r0, #8
         mov r2, #2
         str r2, [r1]
         ldr r1, [r0]
         mov r2, #0
         cmp r2, r1
         bge main$L100
main$L101: 
         mov r1, #0
         add r1, r1, #1
         mov r2, #4
         mul r1, r1, r2
         add r0, r0, r1
         ldr r0, [r0]
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L100: 
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
