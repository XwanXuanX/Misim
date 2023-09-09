.data
HW:     104, 101, 108, 108, 111, 32, 119, 111, 114, 108, 100,

.text
        ; Put address of array into R0
        xor R0, R0, R0
        add R0, R0, HW

        ; Put length of array into R1
        xor R1, R1, R1
        add R1, R1, 11

        ; Invoke kernel-level function
        syscall 1

        ; End of program
        end