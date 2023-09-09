.extra
Field:	100

.text
		; Move field starting address to R0
		xor R0, R0, R0
		add R0, R0, Field

		; Move field length to R1
		xor R1, R1, R1
		add R1, R1, 100

		; Invoke kernel-level function to take input
		syscall 2

		; Print out to console
		syscall 1

		; Stop the process
		end