.section .note.GNU-stack

@ Here is the RPI code

@ Here's function: _^main^_^main

.balign 4
.global main
.section .text

main:
         push {r4-r10, fp, lr}
         add fp, sp, #32
main$L129: 
         mov r0, #4
         bl malloc
         mov r5, r0
         mov r0, #0
         str r0, [r5]
         mov r4, #0
         bl starttime
         mov r2, r4
main$L104: 
         mov r3, r5
         ldr r0, [r5]
         cmp r2, r0
         bge main$L100
main$L101: 
         add r0, r2, #1
         mov r1, #4
         mul r0, r0, r1
         add r0, r3, r0
         str r2, [r0]
         add r4, r2, #1
         mov r1, #0
         mov r0, #0
         cmp r1, r0
         bne main$L106
main$L105: 
         mov r0, #8
         cmp r4, r0
         bge main$L109
main$L110: 
main$L111: 
         mov r2, r4
         b main$L104
main$L100: 
         mov r0, #-1
         bl exit
main$L106: 
         mov r1, r5
         mov r0, #8
         bl putarray
         mov r0, r5
main$L114: 
         mov r5, r0
         mov r1, #1
         mov r0, #1
         cmp r1, r0
         beq main$L115
main$L116: 
         bl stoptime
         mov r2, r5
         ldr r0, [r5]
         mov r1, #1000
         cmp r1, r0
         bge main$L127
main$L128: 
         mov r0, #1000
         add r0, r0, #1
         mov r1, #4
         mul r0, r0, r1
         add r0, r2, r0
         ldr r0, [r0]
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L109: 
         b main$L106
main$L115: 
         bl getint
         mov r1, #0
         cmp r4, r1
         bne main$L119
main$L120: 
main$L121: 
         mov r1, #0
         cmp r4, r1
         bne main$L125
main$L124: 
         mov r5, r0
         b main$L116
main$L119: 
         mov r5, r0
         b main$L116
main$L125: 
main$L126: 
         b main$L114
main$L127: 
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
