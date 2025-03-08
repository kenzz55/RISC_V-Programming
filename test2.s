ADD x7, x1, x0
SUB x8, x7, x1
ADDI x8, x0, 9
ORI x9, x8, 45
LOOP:
ADD x7, x7, x8
SUB x9, x9, x8
BGE x9, x8, loop
ADDI x9, x0, 5
ADDI x7, x0, 7
BGE x7, x9, LAB
ADD x10, x1, x2
LAB:
SUB x11, x3, x4