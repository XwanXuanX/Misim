# Instruction Set Design


## Introduction

It's time to make a change!

Although the existing instruction set works fine, it is necessary to sit back and rethink whether it is truly optimal. From the current point of view, there are several obvious drawbacks in the existing instruction set, which prevents from further software development. The goal is to optimize and fix each drawbacks respectively, and redefine the new instruction set, similar to RISC-V instruction set.

This document is meant to record the evolution of own instruction set(s). The following topics will be covered:
1. __drawback__ of existing instruction set;
2. __principle__ of designing the new instruction set;
3. The proposed new __instruction set__;
4. The proposed new __instruction encoding__;
5. __Differences__ between new and old IS and reasons to change;
6. Future __improvements__;
7. Appendix: A __detailed analysis__ for each instruction, and how other operations can be generated by the superposition of others.


## Drawback of existing instruction set

Here is a full reference of original instruction set:
```
Arithmetic: ADD, UMUL, UDIV, UMOL
Logic:      AND, ORR, XOR, SHL, SHR, RTL, RTR, NOT
Memory:     LDR, STR, PUSH, POP,
Branching:  JMP, JZ, JC, JV, JZN, JN, SYSCALL
```

There are several drawbacks that can be concluded:
* __Redundant or unnecessary instructions__: Some instructions are redundant or unnecessary. For instance, the `PUSH` and `POP` commands can be simply implemented via several combinations of `ADD`, `STR`, `LDR` commands. One better proposed design is letting `PUSH` and `POP` be sudo-commands, which are provided by the assembler. The assembler will generate the corresponding sequences of actual machine instruction to perform the job.

* __Lack of signed integer support__: The original instruction set does not have arithmetic operation for signed integers at all. All the arithmetic operations (e.g. `ADD`, `UMUL`, `UDIV`, `UMOL`) are done using unsigned integers, or in other words, the underlying bit representations for signed integers. This is inconvenient, since to perform signed integers subtraction, one has to perform 2's compliment on one integer then call `ADD` operation on two integers, for example.

* __Lack of other kinds of load instructions__: The original instruction set has one load instruction: `LDR`. This instruction will load entire memory slot into a register. However, this is completely different from what's happening in real life. For simplicity sake, I simplify the memory model to array of integers with various size, while the actual computer memory only consists of a sequence of byte. The system bit and endianness are independent of memory model. Therefore, more load/store instructions are needed to indicate the size of operation (e.g. byte, half-word, or word).

* __Confusing and (almost) unnecessary OpType__: Instructions in current instruction set are classified into 5 types of instructions: `R type`, `I type`, `U type`, `J type`, `S type`. However, within those 5 types, only 1 type is actually used to differentiate instructions. Thus, the others are not necessary. According to the RISC-V instruction set design, they classify instructions into different classes because they define different encodings for each instruction type. However, in my design, all instructions share a common encoding, thus there is no need to classify types.

* __Operation on register and immediate share same OpCode__: In the original instruction set, an operation that can take in a register or an immediate share the same OpCode (e.g. `ADD R1, R2, R3` and `ADD R1, R2, 42`). It's not a huge issue, but definitely make the processor design and software design more challenging. According to the RISC-V instruction set, they make variations on one instruction, so that the operation to registers or immediate values are recognizable.


## Principle of designing new instruction set

The design of the new instruction set follows several principles as listed below:
* ___Completeness___
* ___Compactness___
* ___User-friendly___
* ___Implementation-friendly___

#### Completeness
According to Wikipedia, the formal definition for Turing completeness is:
> A computational system that can compute every Turing-computable function is called Turing-complete (or Turing-powerful). Alternatively, such a system is one that can simulate a universal Turing machine.

In other words, given infinitely many resources and time, one computer system should be able to compute anything. The new instruction set must be turing complete for it to be useful.

#### Compactness
Compactness measures to what extend an instruction set can be reduced and compressed, while maintaining its turing completeness. For instance, consider the instruction `SUB R1, R2, R3`. It can be decomposed into this sequence of instructions: `NOT R3, R3` $\rightarrow$ `ADD R3, R3, 1` $\rightarrow$ `ADD R1, R2, R3`. Similarly, the `PUSH` and `POP` instructions can also be decomposed and re-implemented. The subset of the full set of instruction that can be used to implement other instructions are called __Axiom Instruction__. The goal is to find the correct balance between __user-friendly__ and __implementation-friendly__, by adjusting the __non-axiom instructions__.

#### User-friendly
User-friendly defines how a instruction set is easy to use for users (users refer to compiler authors and software developers). Assume a turing complete instruction set; if so many basic operations need to be decomposed into many lines of axiom-instructions, it can be imagined that this instruction set must be difficult to use. For an extreme example, consider the one-instruction machine.

#### Implementation-friendly
Implementation-friendly defines how a instruction set is easy to be implemented for the processor designer. As illustrated by the graph below:
* __CISC:__
```mermaid
flowchart LR
    C1["Compiler (Simple)"] -- Code generation ---> P1["Processor (Complex)"]
```
* __RISC:__
```mermaid
flowchart LR
    C2["Compiler (Complex)"] -- Code generation ---> P2["Processor (Simple)"]
```
Obviously, the more complex the instruction set, the more complex the processor architecture. On the other hand, although simpler (reduced) instruction set requires less complex processor hardware, it imposes more burden on the compiler author and software developers. Due to the lack of hardware and processor design knowledge, I decided to make the new instruction set as minimal as possible, while not compromising usability.


## New instruction set
Below is a full reference of proposed new instruction set:

#### Arithmetic
| Opcode | Operands | Type | Explanation |
| :----- | :------- | :--- | :---------- |
| __add__ | rd, rs1, rs2 | R | Add registers |
| __sub__ | rd, rs1, rs2 | R | Subtract registers |
| __sra__ | rd, rs1, rs2 | R | Shift right arithmetic by register |
| __slt__ | rd, rs1, rs2 | R | Set if less than register, 2's complement |
| __addi__ | rd, rs1, imm[11:0] | I | Add immediate |
| __srai__ | rd, rs1, imm[4:0] | I | Shift right arithmetic by immediate |
| __slti__ | rd, rs1, imm[11:0] | I | Set if less than immediate, 2's complement |
| __mul__ | rd, rs1, rs2 | R | Multiply and return lower bits |
| __mulh__ | rd, rs1, rs2 | R | Multiply signed and return upper bits |
| __mulhu__ | rd, rs1, rs2 | R | Multiply unsigned and return upper bits |
| __mulhsu__ | rd, rs1, rs2 | R | Multiply signed-unsigned and return upper bits |
| __div__ | rd, rs1, rs2 | R | Signed division |
| __divu__ | rd, rs1, rs2 | R | Unsigned division |
| __rem__ | rd, rs1, rs2 | R | Signed remainder |
| __remu__ | rd, rs1, rs2 | R | Unsigned remainder |

#### Logical
| Opcode | Operands | Type | Explanation |
| :----- | :------- | :--- | :---------- |
| __sll__ | rd, rs1, rs2 | R | Shift left logical by register |
| __srl__ | rd, rs1, rs2 | R | Shift right logical by register |
| __and__ | rd, rs1, rs2 | R | Bitwise `AND` with register |
| __or__ | rd, rs1, rs2 | R | Bitwise `OR` with register |
| __xor__ | rd, rs1, rs2 | R | Bitwise `XOR` with register |
| __sltu__ | rd, rs1, rs2 | R | Set if less than register, unsigned |
| __slli__ | rd, rs1, imm[4:0] | I | Shift left logical by immediate |
| __srli__ | rd, rs1, imm[4:0] | I | Shift right logical by immediate |
| __andi__ | rd, rs1, imm[11:0] | I | Bitwise `AND` with immediate |
| __ori__ | rd, rs1, imm[11:0] | I | Bitwise `OR` with immediate |
| __xori__ | rd, rs1, imm[11:0] | I | Bitwise `XOR` with immediate |
| __sltiu__ | rd, rs1, imm[11:0] | I | Set if less than immediate, unsigned |
| __rll__ | rd, rs1, rs2 | R | Rotate left logical by register |
| __rlli__ | rd, rs1, imm[4:0] | I | Rotate left logical by immediate |
| __rrl__ | rd, rs1, rs2 | R | Rotate right logical by register |
| __rrli__ | rd, rs1, imm[4:0] | I | Rotate right logical by immediate |

#### Load & Store
| Opcode | Operands | Type | Explanation |
| :----- | :------- | :--- | :---------- |
| __lui__ | rd, imm[31:12] | U | Load upper immediate |
| __lb__ | rd, imm[11:0] / rs1 | I | Load byte, signed (sign-extend) |
| __lbu__ | rd, imm[11:0] / rs1 | I | Load byte, unsigned (zero-extend) |
| __lh__ | rd, imm[11:0] / rs1 | I | Load half-word, signed (sign-extend) |
| __lhu__ | rd, imm[11:0] / rs1 | I | Load half-word, unsigned (zero-extend) |
| __lw__ | rd, imm[11:0] / rs1 | I | Load word |
| __sb__ | rs2, imm[11:0] / rs1 | S | Store byte |
| __sh__ | rs2, imm[11:0] / rs1 | S | Store half-word |
| __sw__ | rs2, imm[11:0] / rs1 | S | Store word |

#### Control Flow
| Opcode | Operands | Type | Explanation |
| :----- | :------- | :--- | :---------- |
| __beq__ | rs1, rs2, imm[12:1] | SB | Branch if equal |
| __bne__ | rs1, rs2, imm[12:1] | SB | Branch if not equal |
| __blt__ | rs1, rs2, imm[12:1] | SB | Branch if less than, 2's complement |
| __bltu__ | rs1, rs2, imm[12:1] | SB | Branch if less than, unsigned |
| __bge__ | rs1, rs2, imm[12:1] | SB | Branch if greater or equal, 2's complement |
| __bgeu__ | rs1, rs2, imm[12:1] | SB | Branch if greater or equal, unsigned |
| __bz__ | imm[31:12] | U | Branch if Z flag set |
| __bc__ | imm[31:12] | U | Branch if C flag set |
| __bv__ | imm[31:12] | U | Branch if V flag set |
| __bn__ | imm[31:12] | U | Branch if N flag set |

#### System instruction
| Opcode | Operands | Type | Explanation |
| :----- | :------- | :--- | :---------- |
| __scall__ | N/A | I | System call |


## New instruction encodings

Encodings should be implementations-defined.


## Differences between the new and the old

There are many obvious differences that can be seen between the new and the old instruction set.
* __Better signed and unsigned support__: In the new instruction set, most arithmetic and logical operations have both signed and unsigned variations, whereas the old instruction set only have the unsigned version.
* __Better identification of register or immediate operand__: In the new instruction set, most arithmetic and logical operations have both register and immediate variations. The immediate variation is obtained by appending `i` to the register variation. This change makes the development of software easier.
* __Better naming__: The instructions in the new instruction set uses better naming that deliver clearer meaning to the user.
* __More intuitive branch instructions__: New instruction set introduces 6 new compare-and-branch instructions, which makes software development easier.
* __More load and store options__: Due to the fundamental changes in the memory model, the load and store in the old instruction set is insufficient. The new instruction set introduces more variations (options) for load and store, which allow user to load / store different length of data.


## Future improvements

So, what can be done to improve in the future?

Well, there are a few:
* __Floating point__: Floating point is essential for serious real-life development. The current instruction set and computer architecture does not count for floating point types and their operations. Therefore, support for floating point types should be considered to be added.
* __Atomic operations__: In multi-threading environment, atomic operations are essential to prevent data race. Atomic operations require hardware support. The current instruction set and computer architecture does not count for multi-threading environment, which is a huge drawback.


## Appendix

Skipped for now...
