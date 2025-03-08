 ADD x7, x1, x2
 SLLI x8, x7, 2
 SRLI x9, x8, 1
 JAL x1, func1
 ADD x12, x1, x3
 SUB x13, x4, x2
 AND x14, x5, x6
 LW x19, 4(x1)
 ADD x20, x19, x2
 SLLI x21, x20, 1
 SRLI x22, x21, 2
 SW x22, 4(x2)
 EXIT
 func1:
ADD x15, x1, x2
 SUB x16, x3, x4
 ADD x17, x5, x6
 OR x18, x1, x2
 JALR x0, 0(x1)