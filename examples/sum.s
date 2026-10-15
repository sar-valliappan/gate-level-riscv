# Sums 1..5 into x3, storing the running total to memory each time around.
    addi x1, x0, 5      # x1 = loop counter, counts down from 5
    addi x3, x0, 0       # x3 = running total

loop:
    beq  x1, x0, done    # counter hit zero: exit the loop
    add  x3, x3, x1      # total += counter
    sw   x3, 0(x0)       # mem[0] = total, so you can watch it update
    addi x1, x1, -1      # counter -= 1
    jal  x31, loop       # x31: unused scratch for the return address

done:
    addi x4, x0, 1       # marker: reached the end
