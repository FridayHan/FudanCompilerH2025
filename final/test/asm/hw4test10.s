.section .note.GNU-stack

@ Here is the RPI code

@ Here's function: _^main^_^main

.balign 4
.global main
.section .text

main:
         push {r4-r10, fp, lr}
         add fp, sp, #32
main$L119: 
         mov r2, #19
         mov r0, #0
         mov r3, #1
         mov r1, #0
         mov r4, r1
         mov r1, #1
         cmp r2, r1
         bgt main$L102
main$L103: 
         add r1, r3, r4
         mov r3, #0
         cmp r1, r3
         bne main$L116
main$L109: 
         mov r1, #2
         cmp r2, r1
         bgt main$L116
main$L115: 
         mov r3, r0
         mov r1, #3
         cmp r2, r1
         bgt main$L112
main$L117: 
         mov r0, r3
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L102: 
         mov r1, #1
         mov r4, r1
         b main$L103
main$L112: 
         mov r3, r0
         mov r0, #0
         cmp r2, r0
         bne main$L116
         b main$L117
main$L116: 
         mov r0, #1
         mov r3, r0
         b main$L117

.global malloc
.global getint
.global putint
.global putch
.global putarray
.global getch
.global getarray
.global starttime
.global stoptime
