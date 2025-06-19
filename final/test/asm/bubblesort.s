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
         mov r0, #32
         bl malloc
         mov r4, r0
         mov r0, #7
         str r0, [r4]
         add r0, r4, #4
         mov r1, #6
         str r1, [r0]
         add r0, r4, #8
         mov r1, #3
         str r1, [r0]
         add r0, r4, #12
         mov r1, #0
         str r1, [r0]
         add r0, r4, #16
         mov r1, #5
         str r1, [r0]
         add r0, r4, #20
         mov r1, #9
         str r1, [r0]
         add r0, r4, #24
         mov r1, #1
         str r1, [r0]
         add r0, r4, #28
         mov r1, #2
         str r1, [r0]
         mov r0, #8
         bl malloc
         mov r1, r0
         add r0, r1, #4
         ldr r2, =b1$bubbleSort
         str r2, [r0]
         mov r5, #0
         ldr r0, [r1, #4]
         mov r3, r0
         mov r0, r1
         mov r1, r4
         ldr r2, [r4]
         blx r3
main$L102: 
         mov r0, r5
         ldr r1, [r4]
         cmp r0, r1
         blt main$L103
main$L104: 
         mov r0, #10
         bl putch
         mov r0, #0
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
main$L103: 
         mov r2, r4
         ldr r0, [r4]
         cmp r5, r0
         bge main$L105
main$L106: 
         add r0, r5, #1
         mov r1, #4
         mul r0, r0, r1
         add r0, r2, r0
         ldr r0, [r0]
         bl putint
         mov r0, #32
         bl putch
         add r0, r5, #1
         mov r5, r0
         b main$L102
main$L105: 
         mov r0, #-1
         bl exit

@ Here's function: b1^bubbleSort

.balign 4
.global b1$bubbleSort
.section .text

b1$bubbleSort:
         push {r4-r10, fp, lr}
         add fp, sp, #32
b1$bubbleSort$L127: 
         mov r4, r1
         mov r1, r0
         mov r0, #0
         mov r3, #1
         cmp r2, r3
         ble b1$bubbleSort$L102
b1$bubbleSort$L103: 
b1$bubbleSort$L104: 
b1$bubbleSort$L107: 
         sub r3, r2, #1
         cmp r0, r3
         blt b1$bubbleSort$L108
b1$bubbleSort$L109: 
         ldr r0, [r1, #4]
         mov r3, r0
         mov r0, r1
         mov r1, r4
         sub r2, r2, #1
         blx r3
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
b1$bubbleSort$L102: 
         mov r0, #0
         sub sp, fp, #32
         pop {r4-r10, fp, pc}
b1$bubbleSort$L108: 
         mov r3, r4
         ldr r5, [r4]
         cmp r0, r5
         bge b1$bubbleSort$L110
b1$bubbleSort$L111: 
         add r5, r0, #1
         mov r6, #4
         mul r5, r5, r6
         add r3, r3, r5
         ldr r5, [r3]
         mov r3, r4
         ldr r7, [r4]
         add r6, r0, #1
         cmp r6, r7
         bge b1$bubbleSort$L112
b1$bubbleSort$L113: 
         add r6, r0, #1
         add r6, r6, #1
         mov r7, #4
         mul r6, r6, r7
         add r3, r3, r6
         ldr r3, [r3]
         cmp r5, r3
         bgt b1$bubbleSort$L124
b1$bubbleSort$L125: 
b1$bubbleSort$L126: 
         add r0, r0, #1
         b b1$bubbleSort$L107
b1$bubbleSort$L110: 
         mov r0, #-1
         bl exit
b1$bubbleSort$L112: 
         mov r0, #-1
         bl exit
b1$bubbleSort$L116: 
         mov r0, #-1
         bl exit
b1$bubbleSort$L117: 
         add r5, r0, #1
         mov r6, #4
         mul r5, r5, r6
         add r3, r3, r5
         ldr r6, [r3]
         ldr r3, [r4]
         cmp r0, r3
         bge b1$bubbleSort$L118
b1$bubbleSort$L119: 
         mov r3, r4
         ldr r7, [r4]
         add r5, r0, #1
         cmp r5, r7
         bge b1$bubbleSort$L120
b1$bubbleSort$L121: 
         add r5, r0, #1
         mov r7, #4
         mul r5, r5, r7
         add r5, r4, r5
         add r7, r0, #1
         add r7, r7, #1
         mov r8, #4
         mul r7, r7, r8
         add r3, r3, r7
         ldr r3, [r3]
         str r3, [r5]
         mov r3, r4
         ldr r7, [r4]
         add r5, r0, #1
         cmp r5, r7
         bge b1$bubbleSort$L122
b1$bubbleSort$L123: 
         add r5, r0, #1
         add r5, r5, #1
         mov r7, #4
         mul r5, r5, r7
         add r3, r3, r5
         str r6, [r3]
         b b1$bubbleSort$L126
b1$bubbleSort$L118: 
         mov r0, #-1
         bl exit
b1$bubbleSort$L120: 
         mov r0, #-1
         bl exit
b1$bubbleSort$L122: 
         mov r0, #-1
         bl exit
b1$bubbleSort$L124: 
         mov r3, r4
         ldr r5, [r4]
         cmp r0, r5
         bge b1$bubbleSort$L116
         b b1$bubbleSort$L117
.global malloc
.global getint
.global putint
.global putch
.global putarray
.global getch
.global getarray
.global starttime
.global stoptime
