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
         mov r5, r0
         mov r0, #4
         str r0, [r5]
         mov r0, r5
         bl getarray
         mov r4, r0
         ldr r6, [r5]
         add r0, r6, #1
         mov r1, #4
         mul r0, r0, r1
         bl malloc
         mov r1, r0
         str r6, [r1]
         mov r0, #4
         add r2, r6, #1
         mov r3, #4
         mul r3, r2, r3
main$L100: 
         cmp r0, r3
         blt main$L101
main$L102: 
         mov r5, r1
         mov r1, r5
         mov r0, r4
         bl putarray
         mov r0, r5
         ldr r1, [r5]
         mov r2, #0
         cmp r2, r1
         bge main$L103
main$L104: 
         mov r1, #0
         add r1, r1, #1
         mov r2, #4
         mul r1, r1, r2
         add r0, r0, r1
         ldr r0, [r0]
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L101: 
         add r2, r1, r0
         add r6, r5, r0
         ldr r6, [r6]
         mov r7, #0
         sub r6, r7, r6
         str r6, [r2]
         add r0, r0, #4
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
