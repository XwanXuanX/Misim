.data
        ; Declare the array
Arr:    100, 100, 100, 200

.text
        ; Load array start address into R0
		xor R0, R0, R0
		add R0, R0, Arr

        ; R1 as accumulator
		xor R1, R1, R1

		; R2 as counter
		xor R2, R2, R2
		add R2, R2, 4

        ; Start of for loop
lab: 	add R4, R2, 0
		jz  stop            ; If counter reach 0, stop

		ldr R4, R0          ; Load the content at R0 into R4
		add R1, R1, R4      ; Accumulate in R1
		add R0, R0, 1       ; Increment address

		xor R3, R3, R3
		add R3, R3, 1       ; Take the 2's comp by hand
		not R3, R3
		add R3, R3, 1
		add R2, R2, R3      ; Subtract 1 from counter

		jmp lab

stop:	end                 ; End of program