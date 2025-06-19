.section .note.GNU-stack

@ Here is the RPI code

@ Here's function: _^main^_^main

.balign 4
.global main
.section .text

main:
         push {r4-r10, fp, lr}
         add fp, sp, #32
main$L105: 
         mov r0, #20
         bl malloc
         mov r4, r0
         mov r0, #4
         str r0, [r4]
         mov r0, r4
         bl getarray
         ldr r5, [r4]
         add r0, r5, #1
         mov r1, #4
         mul r0, r0, r1
         bl malloc
         mov r1, r0
         str r5, [r1]
         mov r0, #4
         add r2, r5, #1
         mov r3, #4
         mul r2, r2, r3
         mov r6, r0
main$L100: 
         cmp r6, r2
         blt main$L101
main$L102: 
         mov r4, r1
         mov r1, r4
         mov r0, #4
         bl putarray
         mov r2, r4
         ldr r0, [r4]
         mov r1, #0
         cmp r1, r0
         bge main$L103
main$L104: 
         mov r0, #0
         add r0, r0, #1
         mov r1, #4
         mul r0, r0, r1
         add r0, r2, r0
         ldr r0, [r0]
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L101: 
         add r3, r1, r6
         add r0, r4, r6
         ldr r0, [r0]
         mov r5, #0
         sub r0, r5, r0
         str r0, [r3]
         add r0, r6, #4
         mov r6, r0
         b main$L100
main$L103: 
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
