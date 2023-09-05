from __future__ import annotations
from typing import *


"""
@file:   assembler.py

@author: Yetong Li

@date:   Aug. 2023

@detail:

Very simple prototype of own assembler.
Meant to simplify testing process for processors; NOT for professional use.

"""

"""
; A sample of own .asm file
; Comments start with ';'

.data
Arr: 	1, 34, 62, 4, 100

.extra
Fld: 	17

.text
	    ; Equal to move 23 to R0
	    XOR     R0, R0, R0
	    ADD     R0, R0, 23

	    PUSH    R0
	    JMP     Label

Ret:	POP     R0
	    END     ; Put 'END' where the program returns

Label:  JMP Ret ; Perform very complicated calculations

"""


import os
import enum
import logging
import argparse
import contextlib


def getArgumentParser():
    """ Create and return the argument parser. """

    parser = argparse.ArgumentParser(
        prog='Misim Assembler',
        description='Very simple prototype of own assembler for Misim project',
    )

    parser.add_argument(
        '-f', '--file',
        dest='filepath',
        action='store',
        type=str,
        metavar='PATH',
        required=True,
        help='Provide the path to the asm file.'
    )
    parser.add_argument(
        '-c', '--check',
        dest='check',
        action='store_true',
        required=False,
        help='Check for errors without compiling.'
    )

    return parser


class CommandlineArgs:
    filepath: str
    check: bool

    def __init__(self) -> None:
        self.populate()

    @classmethod
    def populate(cls) -> None:
        parser = getArgumentParser()
        args = parser.parse_args()
        cls.filepath = args.filepath
        cls.check = args.check


def asmReaderGenerator(file_path: str) -> (Generator | list):
    """ Return generator that read asm file line by line. """

    if not os.path.exists(file_path) or not os.access(file_path, os.R_OK):
        return []
    
    def generator():
        nonlocal file_path
        with contextlib.closing(open(file_path)) as source_file:
            for line in source_file:
                if line.strip().startswith(';') or line == '\n':
                    continue
                yield line.strip()
    
    return generator()


def caselessCompare(s1: str, s2: str) -> bool:
    return s1.casefold() == s2.casefold()


def caselessIn(s: str, container: Iterable[str]) -> bool:
    for e in container:
        if caselessCompare(s, e):
            return True
    else:
        return False


class Token:
    """ Smallest unit of text that convey meanings. """

    @enum.unique
    class TokenType(enum.Enum):
        SPECIAL    = "Special"
        NUMBER     = "Number"
        IDENTIFIER = "Identifier"

        def __str__(self) -> str:
            return self.value

    special_tokens: list[str] = [".", ":", ",", ";"]

    def __init__(self, type: TokenType, value: str) -> None:
        self.type: Token.TokenType = type
        self.value: str = value

        if not self.validateToken():
            raise ValueError(
                "Given token type and value does not match."
            )
    
    def validateToken(self) -> bool:
        """ Validate the type and value of token. """
        
        def validateSpecial(value: str) -> bool:
            return value in Token.special_tokens
        
        def validateNumber(value: str) -> bool:
            return value.isnumeric()
        
        def validateIdentifier(value: str) -> bool:
            return (
                not validateSpecial(value) and 
                not validateNumber(value)
            )
        
        if self.type == Token.TokenType.SPECIAL:
            return validateSpecial(self.value)
        elif self.type == Token.TokenType.NUMBER:
            return validateNumber(self.value)
        else:
            return validateIdentifier(self.value)
        
    def __str__(self) -> str:
        return f"Token Type: {self.type}\nToken Value: '{self.value}'"


def lexer(loc: str) -> list[Token]:
    """
    Given a line of code, the function parse the string
    into a stream of tokens.
    """

    result: list[Token] = list()

    # The stack to keep track of identifiers and numbers
    stack: str = ""

    def recordStack(stack: str, result: list[Token]) -> None:
        """ Pushback stack into result. """

        if stack == "":
            return
        
        result.append(
            Token(
                type=Token.TokenType.NUMBER, 
                value=stack
            )
            if stack.isnumeric() else
            Token(
                type=Token.TokenType.IDENTIFIER,
                value=stack
            )
        )

    for char in loc:
        # Special case - char is ';'
        if char == ';':
            break
        
        # case 1 - char is a special token
        if char in Token.special_tokens:
            # when meet a special token,
            # whats stored in stack needs to be recorded
            recordStack(stack, result)
            stack = ""
            
            # then append the special token
            result.append(
                Token(
                    type=Token.TokenType.SPECIAL,
                    value=char
                )
            )
            continue

        # case 2 - char is a seperator
        if char == ' ' or char == '\t':
            # when meet a seperator (such as space or tab)
            # record content in the stack
            recordStack(stack, result)
            stack = ""
            continue

        # case 3, 4 - char is a digit or character
        if char.isdigit() or char.isalpha():
            # simply push the character onto the stack
            stack += char
            continue
    
    # At the end of line, push what's left in stack
    recordStack(stack, result)
    stack = ""

    return result


class CustomeLoggingFormatter(logging.Formatter):
    """ Custome logging formatter for error reporting. """

    class ConsoleColor:
        grey    = '\033[38;21m'
        blue    = '\033[38;5;39m'
        yellow  = '\033[38;5;226m'
        red     = '\033[38;5;196m'
        reset   = '\033[0m'

    def __init__(self, fmt):
        super().__init__()
        
        self.fmt = fmt
        
        self.FORMAT = {
            logging.DEBUG   : self.ConsoleColor.grey + self.fmt + self.ConsoleColor.reset,
            logging.INFO    : self.ConsoleColor.blue + self.fmt + self.ConsoleColor.reset,
            logging.WARNING : self.ConsoleColor.yellow + self.fmt + self.ConsoleColor.reset,
            logging.ERROR   : self.ConsoleColor.red + self.fmt + self.ConsoleColor.reset
        }

    def format(self, record):
        log_fmt = self.FORMAT.get(record.levelno)
        formatter = logging.Formatter(log_fmt)
        return formatter.format(record)


def getConfigedLogger(level: int, fmt: str) -> logging.Logger:
    """ Create and config logger """

    logger = logging.getLogger(__name__)
    logger.setLevel(level)

    # Create stdout handler
    stdout_handler = logging.StreamHandler()
    stdout_handler.setLevel(level)
    stdout_handler.setFormatter(CustomeLoggingFormatter(fmt))

    logger.addHandler(stdout_handler)

    return logger


class Keywords:
    """ Contain all allowed keywords. """

    segments: tuple[str, ...] = (
        'data', 'extra', 'text'
    )

    OpCode: tuple[str, ...] = (
        'ADD',      #  add two operands		    #		ADD  R1, R2, R3/imm		#		R1 <- R2 + R3/imm
        'UMUL',	    #  multiple two operands	#		UMUL R1, R2, R3/imm		#		R1 <- R2 * R3/imm
        'UDIV',		#  divide two operands		#		UDIV R1, R2, R3/imm		#		R1 <- R2 / R3/imm
        'UMOL',		#		op1 % op2			#		UMOL R1, R2, R3/imm		#		R1 <- R2 % R3/imm

        'AND',		#  bitwise And of A & B	    #		AND  R1, R2, R3/imm		#		R1 <- R2 & R3/imm
        'ORR',		#  bitwise Or of A & B		#		ORR  R1, R2, R3/imm		#		R1 <- R2 | R3/imm
        'XOR',		#  bitwise Xor of A & B	    #		XOR  R1, R2, R3/imm		#		R1 <- R2 ^ R3/imm
        'SHL',		#  logical shift left		#		SHL  R1, R2, R3/imm		#		R1 <- R2 << R3/imm
        'SHR',		#  logical shift right		#		SHR  R1, R2, R3/imm		#		R1 <- R2 >> R3/imm
        'RTL',		#  logical rotate left		#		RTL  R1, R2, R3/imm		#		R1 <- R2 <~ R3/imm
        'RTR',		#  logical rotate right	    #		RTR  R1, R2, R3/imm		#		R1 <- R2 ~> R3/imm

        'NOT',		#  comp all the bits		#		NOT  R1, R2				#		R1 <- ~R2

        'LDR',		#  load reg from mem		#		LDR  R1, R2				#		R1 <- [R2]
        'STR',		#  store reg in mem		    #		STR  R1, R2				#		[R1] <- R2

        'PUSH',		#  push reg onto stack		#		PUSH R1					#		[SP - 4] <- R1;
        'POP',		#  pop top ele into reg	    #		POP	 R1					#		R1 <- [SP] + 4

        'JMP',		#	unconditional jump		#		JMP label				#		N/A
        'JZ',		#  jump if Z flag is set	#		JZ	label				#		N/A
        'JN',		#  jump if N flag is set	#		JN	label				#		N/A
        'JC',		#  jump if C flag is set	#		JC	label				#		N/A
        'JV',		#  jump if V flag is set	#		JV	label				#		N/A
        'JZN',		#  jump if Z or N is set	#		JZN label				#		N/A

        'SYSCALL',	#  invokes system calls	    #		SYSCALL 1				#		N/A
        'END'       #  end of execution
    )

    registers: tuple[str, ...] = (
        'R0',   #  \ 
        'R1',   #   |
        'R2',   #   |
        'R3',   #   |
        'R4',   #   |
        'R5',   #   |  General
        'R6',   #   |  Purpose
        'R7',   #   |  Registers
        'R8',   #   |
        'R9',   #   |
        'R10',  #   |
        'R11',  #   |
        'R12',  #  /

        'SP',   #      Stack Pointer
        'LR',   #      Link Register
        'PC'    #      Program Counter
    )

    @classmethod
    def isKeyword(cls, token: Token) -> bool:
        """ Check if a token is a keyword token. """

        if token.type != Token.TokenType.IDENTIFIER:
            return False
        
        # Using caselessIn to ignore case differences
        return (
            caselessIn(token.value, cls.segments) or
            caselessIn(token.value, cls.OpCode)   or
            caselessIn(token.value, cls.registers)
        )
    
    @classmethod
    def isLabel(cls, token: Token) -> bool:
        """ Check if a token is a label token. """

        if token.type != Token.TokenType.IDENTIFIER:
            return False
        
        # An identifier can only be a label or a keyword.
        return not cls.isKeyword(token)
    
    @classmethod
    def isSegment(cls, token: Token) -> bool:
        """ Check if a token is a segment token. """

        if token.type != Token.TokenType.IDENTIFIER:
            return False
        
        return caselessIn(token.value, cls.segments)
    
    @classmethod
    def isOpcode(cls, token: Token) -> bool:
        """ Check if a token is a opcode token. """

        if token.type != Token.TokenType.IDENTIFIER:
            return False
        
        return caselessIn(token.value, cls.OpCode)
    
    @classmethod
    def isRegister(cls, token: Token) -> bool:
        """ Check if a token is a register token. """

        if token.type != Token.TokenType.IDENTIFIER:
            return False
        
        return caselessIn(token.value, cls.registers)


def exitProgram(exit_code: int) -> None:
    """ Log and exit program. """

    # Log exit message
    print(
        "Assembler quited with error..."
        if exit_code != 0 else
        "Assembler quited successfully."
    )
    exit(None)


class DataSegment:
    """ Parse and record relavant information about data segment. """

    def __init__(self, logger: logging.Logger) -> None:
        """ Initialize data container. """

        # Label as key - Label values as value
        self.value_table: dict[str, list[int]] = dict()

        # logging facility
        self.logger: logging.Logger = logger

    class StateMachine:
        """ Use state machine to check for grammer validity. """

        @enum.unique
        class States(enum.Enum):
            NUMBER = enum.auto()
            COMMA  = enum.auto()

        def reset(self) -> None:
            """ Reset current state to starting state. """
            self.state = DataSegment.StateMachine.States.NUMBER

        def __init__(self) -> None:
            self.state: DataSegment.StateMachine.States
            
            # Initialize by reseting
            self.reset()

        def update(self, token: Token) -> bool:
            """ 
            Update current state with token;
            Then return validity.
            """

            # Two state state-machine:
            #              _____NUMBER___
            #        _____|___       ____v_____
            #       |         |     |          |
            #  <----|  NUMBER |     |   COMMA  |__else__-> Error
            #       |_________|     |__________|
            #             ^_____COMMA____|

            def isNumber(token: Token) -> bool:
                return token.type == Token.TokenType.NUMBER
            
            def isComma(token: Token) -> bool:
                return (
                    token.type == Token.TokenType.SPECIAL and
                    token.value == ","
                )

            if self.state == DataSegment.StateMachine.States.NUMBER:
                # Expect token to be a number
                if isNumber(token):
                    self.state = DataSegment.StateMachine.States.COMMA
                    return True
                else:
                    return False
            
            elif self.state == DataSegment.StateMachine.States.COMMA:
                # Expect token to be a comman
                if isComma(token):
                    self.state = DataSegment.StateMachine.States.NUMBER
                    return True
                else:
                    return False
            
            # Anything else happend, return False anyways
            return False

    def parse(self, token_stream: list[Token]) -> None:
        """ Parse the token stream. """

        # The pattern goes like: label1: n1, n2, n3, n4, ...
        # Where label MUST NOT be a keyword.

        # Validate the length of token stream
        if len(token_stream) < 3:
            self.logger.error(
                "DataSegment: Not enough token to analyze."
            )
            exitProgram(1)
            return

        # The first token must be a label
        identifier: Token = token_stream.pop(0)

        if not Keywords.isLabel(identifier):
            self.logger.error(
                "DataSegment: Data declarations not start with a label."
            )
            exitProgram(1)
            return
        
        # Label must not be defined before
        if identifier.value in self.value_table.keys():
            self.logger.error("DataSegment: Label redefinition.")
            exitProgram(1)
            return
        
        separator: Token = token_stream.pop(0)

        if (separator.type  != Token.TokenType.SPECIAL or
            separator.value != ":"):
            self.logger.error(
                "DataSegment: No ':' following label declaration."
            )
            exitProgram(1)
            return
        
        # Now the token stream only contains "n1, n2, n3, n4, ..."

        # Use state machine to validate syntax
        sm = DataSegment.StateMachine()

        values: list[Token] = list()

        for token in token_stream:
            # Check syntax error
            if not sm.update(token):
                self.logger.error(
                    "DataSegment: "
                    "Declaration not follow the pattern 'n1, n2, n3, ...'."
                )
                exitProgram(1)
                return
            
            # Store tokens
            if token.type == Token.TokenType.NUMBER:
                values.append(token)
        
        # Populate value table
        self.value_table[identifier.value] = list()

        for token in values:
            try:
                self.value_table[identifier.value].append(
                    int(token.value)
                )
            except ValueError:
                self.logger.error(
                    "DataSegment: Some data is not integer."
                )
                exitProgram(1)
                return
        
        return
    
    def size(self) -> int:
        """ Calculate the size of this segment. """

        segment_size: int = 0

        for label, data in self.value_table.items():
            assert isinstance(data, list)
            segment_size += len(data)

        return segment_size
    
    def symbolTable(self) -> dict[str, int]:
        """ Parse value table into a local symbol table. """

        symbol_table: dict[str, int] = dict()

        # Keep track of current address
        curr_address: int = 0

        for label, data in self.value_table.items():
            assert isinstance(data, list)

            symbol_table[label] = curr_address
            curr_address += len(data)
        
        return symbol_table


class ExtraSegment:
    """ Parse and record relavant information about extra segment. """

    def __init__(self, logger: logging.Logger) -> None:
        """ Initialize data container. """

        # Label as key - Label value as value
        self.value_table: dict[str, int] = dict()

        # logging facility
        self.logger: logging.Logger = logger

    def parse(self, token_stream: list[Token]) -> None:
        """ Parse the token stream. """

        # The pattern goes like: label1: number
        # Where label MUST NOT be a keyword.

        if len(token_stream) != 3:
            self.logger.error(
                "ExtraSegment: Wrong number of tokens to analyze."
            )
            exitProgram(1)
            return
        
        # The first token must be a label
        identifier: Token = token_stream.pop(0)
    
        if not Keywords.isLabel(identifier):
            self.logger.error(
                "ExtraSegment: "
                "Space declaration not start with a label."
            )
            exitProgram(1)
            return

        # Label must not be defined before
        if identifier.value in self.value_table.keys():
            self.logger.error("ExtraSegment: Label redefinition.")
            exitProgram(1)
            return
        
        separator: Token = token_stream.pop(0)

        if (separator.type  != Token.TokenType.SPECIAL or
            separator.value != ":"):
            self.logger.error(
                "ExtraSegment: No ':' following label declaration."
            )
            exitProgram(1)
            return
        
        # Now the token stream only contains one token "number"
        assert len(token_stream) == 1

        if token_stream[0].type != Token.TokenType.NUMBER:
            self.logger.error(
                "ExtraSegment: "
                "Expecting a number token after label declaration."
            )
            exitProgram(1)
            return
        
        # Populate value table
        try:
            self.value_table[identifier.value] = int(
                token_stream[0].value
            )
        except ValueError:
            self.logger.error(
                "ExtraSegment: Data is not integer."
            )
            exitProgram(1)
            return
        
        return
    
    def size(self) -> int:
        """ Calculate the size of this segment. """

        segment_size: int = 0

        for label, space in self.value_table.items():
            segment_size += space

        return segment_size
    
    def symbolTable(self) -> dict[str, int]:
        """ Parse value table into a local symbol table. """

        symbol_table: dict[str, int] = dict()

        # Record the space used by previous declarations
        prev_space: int = 0

        for label, space in self.value_table.items():
            symbol_table[label] = prev_space
            prev_space += space

        return symbol_table


class Instruction:
    """ Container class for single instruction. """

    # Basic layout:
    # 1 0 9 8   7 6 5 4   3 2 1 0   9 8 7 6   5 4 3 2   1 0 9 8   7 6 5 4   3 2 1 0
    # 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0
    # 					   \____/    \____/    \_____/   \_______________/   \_____/
    # \_____________________|_/	   |		  |				  |				|
    #			imm			 Rn		   Rm		  Rd			 OpCode		  OpType

    @enum.unique
    class InstructionType(enum.Enum):
        """ Enum class for the type of instructions. """

        # R_type: 2 source registers, 1 dest registers, no imm; Binary operations
        # ADD Rd, Rs, Rm ; MUL Rd, Rs, Rm
        Rt = "R Type"

        # I_type: 1 source register, 1 dest register, 1 imm; Binary operations, with imm
        # ADD Rd, Rs, imm
        It = "I Type"

        # U_type: 1 source register, 1 dest register, 0 imm; Unary operations
        # NEG Rd, Rs ; NOT Rd, Rs ; LDR Rd, Rs ; STR Rd, Rs
        Ut = "U Type"

        # S_type: 0 source register, 1 dest register, 0 imm (only for stack operations)
        # PUSH Rd ; POP Rd
        St = "S Type"

        # J_type: 0 source register, 0 dest register 1 imm (only for branching)
        # JMP imm ; JEQ imm
        Jt = "J Type"

        def __str__(self) -> str:
            return self.value
    
    def __init__(self) -> None:
        # What's won't be in binary
        self.label: str = None

        # What's will be in binary
        self.type: Instruction.InstructionType = None
        self.code: str = None
        self.Rd: str = None
        self.Rm: str = None
        self.Rn: str = None
        self.imm: (str | int) = None
    
    def __str__(self) -> str:
        """ String rep of instruction """

        dmn: str = (
            f"{self.Rd if self.Rd is not None else ''} "
            f"{self.Rm if self.Rm is not None else ''} "
            f"{self.Rn if self.Rn is not None else ''} "
        )

        return (
            f"Label: {self.label if self.label is not None else ''}\n"
            f"Type : {self.type  if self.type  is not None else ''}\n"
            f"Code : {self.code  if self.code  is not None else ''}\n"
            f"dmn  : {dmn}\n"
            f"imm  : {self.imm   if self.imm   is not None else ''}\n"
        )


class TextSegment:
    """ Parse and record information about text segment. """

    def __init__(self, logger: logging.Logger) -> None:
        """ Initialize data container. """

        # Sequentially stores instructions
        self.instruction: list[Instruction] = list()

        # Keep a list of used labels
        self.used_label: list[str] = list()

        # Logging facility
        self.logger: logging.Logger = logger
    
    class StateMachine:
        """ Use state machine to check grammer validity. """

        @enum.unique
        class States(enum.Enum):
            COMMA = enum.auto()
            OTHER = enum.auto()

        def reset(self) -> None:
            """ Reset current state to starting state. """
            self.state = TextSegment.StateMachine.States.OTHER
        
        def __init__(self) -> None:
            self.state: TextSegment.StateMachine.States

            # Initialize by reseting
            self.reset()

        @staticmethod
        def isComma(token: Token) -> bool:
            return (
                token.type == Token.TokenType.SPECIAL and 
                token.value == ","
            )
        
        @staticmethod
        def isOther(token: Token) -> bool:
            # Other can be register, number, label
            return (
                Keywords.isRegister(token) or
                token.type == Token.TokenType.NUMBER or
                Keywords.isLabel(token)
            )
        
        def update(self, token: Token) -> bool:
            """
            Update current state with token;
            Return the validity of grammar.
            """

            if self.state == TextSegment.StateMachine.States.OTHER:
                # Expect the token to be other
                if self.isOther(token):
                    self.state = TextSegment.StateMachine.States.COMMA
                    return True
                else:
                    return False

            elif self.state == TextSegment.StateMachine.States.COMMA:
                # Expect the token to be a comma
                if self.isComma(token):
                    self.state = TextSegment.StateMachine.States.OTHER
                    return True
                else:
                    return False

            # Other than those, return False anyways
            return False
    
    class ValidateOpcodeType:
        """ Check if the opcode can have certain type. """

        lookup_table: dict[Instruction.InstructionType, tuple[str, ...]] = {
            Instruction.InstructionType.Rt: (
                'ADD', 'UMUL', 'UDIV', 'UMOL', 'AND',
                'ORR', 'XOR' , 'SHL' , 'RTL' , 'RTR'
            ),
            Instruction.InstructionType.It: (
                'ADD', 'UMUL', 'UDIV', 'UMOL', 'AND',
                'ORR', 'XOR' , 'SHL' , 'RTL' , 'RTR'
            ),
            Instruction.InstructionType.Ut: (
                'NOT', 'LDR', 'STR'
            ),
            Instruction.InstructionType.St: (
                'PUSH', 'POP'
            ),
            Instruction.InstructionType.Jt: (
                'JMP', 'JZ', 'JN' ,
                'JC' , 'JV', 'JZN',
                'SYSCALL'
            )
        }

        @classmethod
        def validate(cls, inst: Instruction) -> bool:
            """ Validate opcode and optype """
            return caselessIn(inst.code, cls.lookup_table[inst.type])

    def parse(self, token_stream: list[Token]) -> None:
        """ Parse the token stream. """

        inst: Instruction = Instruction()

        # Two possibilities: Code with label, and code that don't

        def parseLabel() -> None:
            """ Parse the label part of instruction """
            nonlocal inst
            nonlocal token_stream

            identifier: Token = token_stream.pop(0)
            assert Keywords.isLabel(identifier)

            # Label must not be defined before
            if identifier.value in self.used_label:
                self.logger.error("TextSegment: Label redefinition.")
                exitProgram(1)
                return

            # Validate if there is a ':' followed
            separator: Token = token_stream.pop(0)

            if (separator.type  != Token.TokenType.SPECIAL or
                separator.value != ":"):
                self.logger.error(
                    "TextSegment: No ':' following label declaration."
                )
                exitProgram(1)
                return
            
            # Now the label is validated
            inst.label = identifier.value
            self.used_label.append(identifier.value)
            return
        
        def parseOpcode() -> None:
            """ Parse the Opcode of the instruction. """
            nonlocal inst
            nonlocal token_stream

            identifier: Token = token_stream.pop(0)
            assert Keywords.isOpcode(identifier)

            # Simply store the opcode
            inst.code = identifier.value
            return
        
        # First validate if any instruction start with a label or a opcode
        if (not Keywords.isLabel(token_stream[0]) and
            not Keywords.isOpcode(token_stream[0])):
            self.logger.error(
                "TextSegment: Instruction not start with label or Opcode."
            )
            exitProgram(1)
            return
        
        # If the instruction starts with a label
        if Keywords.isLabel(token_stream[0]):
            parseLabel()

        # Then we parse opcode anyways
        parseOpcode()

        # For instructions with opcode only (such as 'END')
        if len(token_stream) == 0:
            # Record instruction and leave
            self.instruction.append(inst)
            return
        
        # Use the state machine to extract relavant information and validate grammar.
        sm = TextSegment.StateMachine()

        values: list[Token] = list()

        for token in token_stream:
            # Check syntax error
            if not sm.update(token):
                self.logger.error(
                    "TextSegment: Opcode operands syntax error."
                )
                exitProgram(1)
                return
            
            # Store valuable token
            if TextSegment.StateMachine.isOther(token):
                values.append(token)
        
        # Next analyze the value list

        def analyzeValueList(value_list: list[Token]) -> None:
            """
            Analyze value list to obtain register and imm information;
            Deduce instruction type;
            Fill instruction member.
            """
            nonlocal inst

            # Validate the minimum length
            assert len(value_list) >= 1
            # Validate the maximum length
            if len(value_list) > 3:
                self.logger.error(
                    "TextSegment: Too many operands in one instruction."
                )
                exitProgram(1)
                return
            
            # Now start parsing operands
            match len(value_list):
                case 3:
                    # If length is 3, then either Rt or It
                    if (not Keywords.isRegister(value_list[0]) or
                        not Keywords.isRegister(value_list[1])):
                        self.logger.error(
                            "TextSegment: "
                            "R-type or I-type instruction missing Rd and Rm."
                        )
                        exitProgram(1)
                        return
                    
                    inst.Rd = value_list[0].value
                    inst.Rm = value_list[1].value

                    if Keywords.isRegister(value_list[2]):
                        # If the last operand is a register, then R-type.
                        inst.type = Instruction.InstructionType.Rt
                        inst.Rn = value_list[2].value

                    elif Keywords.isLabel(value_list[2]):
                        # If the last operand is a label, then I-type.
                        inst.type = Instruction.InstructionType.It
                        inst.imm = value_list[2].value
                    
                    elif value_list[2].type == Token.TokenType.NUMBER:
                        # If the last operand is a number, then I-type.
                        inst.type = Instruction.InstructionType.It
                        try:
                            inst.imm = int(value_list[2].value)
                        except ValueError:
                            self.logger.error(
                                "TextSegment: "
                                "Failed to convert imm to integer."
                            )
                            exitProgram(1)
                            return
                    
                    else:
                        self.logger.error(
                            "TextSegment: Unexpected token."
                        )
                        exitProgram(1)
                        return
                    
                    return
                
                case 2:
                    # If length is 2, then only U-type instruction
                    if (not Keywords.isRegister(value_list[0]) or
                        not Keywords.isRegister(value_list[1])):
                        self.logger.error(
                            "TextSegment: "
                            "U-type instruction missing Rd or Rm."
                        )
                        exitProgram(1)
                        return
                    
                    inst.type = Instruction.InstructionType.Ut
                    inst.Rd = value_list[0].value
                    inst.Rm = value_list[1].value

                    return
                
                case 1:
                    # If length is 1, then either S-type or J-type
                    value: Token = value_list.pop(0)

                    if Keywords.isRegister(value):
                        # If the operand is a register, then S-type
                        inst.type = Instruction.InstructionType.St
                        inst.Rd = value.value
                    
                    elif Keywords.isLabel(value):
                        # If the operand is a label, then J-type
                        inst.type = Instruction.InstructionType.Jt
                        inst.imm = value.value
                    
                    elif value.type == Token.TokenType.NUMBER:
                        # If the operand is a number, then J-type
                        inst.type = Instruction.InstructionType.Jt
                        try:
                            inst.imm = int(value.value)
                        except ValueError:
                            self.logger.error(
                                "TextSegment: "
                                "Failed to convert imm to integer."
                            )
                            exitProgram(1)
                            return
                    
                    else:
                        self.logger.error(
                            "TextSegment: Unexpected token."
                        )
                        exitProgram(1)
                        return
                    
                    return
                
                case default:
                    self.logger.error("TextSegment: Unexpected Error!")
                    exitProgram(1)
                    return
        
        # Call the function
        analyzeValueList(values)

        # Need to validate if the code match the type
        if not TextSegment.ValidateOpcodeType.validate(inst):
            self.logger.error(
                "TextSegment: Opcode and Optype mismatch."
            )
            exitProgram(1)
            return
        
        # Push back the filled instruction
        self.instruction.append(inst)

        return
    
    def size(self) -> int:
        """ Calculate the size of this segment. """

        return len(self.instruction)
    
    def symbolTable(self) -> dict[str, int]:
        """ Parse instruction label into local symbol table. """

        symbol_table: dict[str, int] = dict()

        for idx, inst in enumerate(self.instruction):
            if inst.label is None:
                continue
            symbol_table[inst.label] = idx
        
        return symbol_table


@enum.unique
class SegmentType(enum.Enum):
    DATA  = "Data Segment"
    EXTRA = "Extra Segment"
    TEXT  = "Text Segment"

    def __str__(self) -> str:
        return self.value


def parseSegmentLabel(
        logger: logging.Logger,
        token_stream: list[Token]) -> (None | SegmentType):
    """ Parse the segment label; Return the segment type. """

    # In case non-label declaration code is passed in:
    # Return None to signal not segment declaration
    symbol: Token = token_stream.pop(0)
    if (symbol.type  != Token.TokenType.SPECIAL or
        symbol.value != "."):
        return None
    
    # Here the token stream must be a segment declaration
    # Validate the length
    if len(token_stream) != 1:
        logger.error("SegmentLabel: Unexpected number of tokens.")
        exitProgram(1)
        return
    
    segment: Token = token_stream.pop(0)
    # Validate the type
    if not Keywords.isSegment(segment):
        logger.error("SegmentLabel: Invalid segment symbol.")
        exitProgram(1)
        return
    
    # Pick the correct enum and return segment type
    segment_type: tuple[SegmentType, ...] = (
        SegmentType.DATA,
        SegmentType.EXTRA,
        SegmentType.TEXT
    )

    for label, type in zip(Keywords.segments, segment_type):
        # Check for the matching label
        if caselessCompare(label, segment.value):
            return type
    else:
        logger.error("SegmentLabel: Invalid segment symbol.")
        exitProgram(1)
        return


class Parser:
    """ Parse the file in segments and store the data. """

    def __init__(
            self,
            logger  : logging.Logger,
            filepath: str) -> None:
        """ Initialize variables and segments. """

        self.logger: logging.Logger = logger
        self.filepath: str = filepath

        # Three segments
        self.ds: DataSegment  = DataSegment(self.logger)
        self.es: ExtraSegment = ExtraSegment(self.logger)
        self.ts: TextSegment  = TextSegment(self.logger)

    def parse(self) -> None:
        """ Parse the file line by line into an intermediate representation. """

        # Validate file extension
        if not self.filepath.endswith(".asm"):
            self.logger.error("Invalid file name extension.")
            exitProgram(1)
            return
        
        # Validate the file path
        if not os.path.exists(self.filepath):
            self.logger.error("File path does not exist.")
            exitProgram(1)
            return
        
        # The active segment will be responsible for parsing
        active_segment: (DataSegment | ExtraSegment | TextSegment | None) = None

        def switchSegment(segment_type: (SegmentType | None)) -> bool:
            """ Attempt to switch segment; Return true if did. """
            nonlocal active_segment

            match segment_type:
                case SegmentType.DATA:
                    active_segment = self.ds
                    return True

                case SegmentType.EXTRA:
                    active_segment = self.es
                    return True

                case SegmentType.TEXT:
                    active_segment = self.ts
                    return True

                case None:
                    # Current line of code is not segment declaration
                    return False
            
            # Unknown match fall through
            self.logger.error("Unknown segment type.")
            exitProgram(1)
            return False
        
        # Now parse the file line by line
        for line in asmReaderGenerator(self.filepath):
            # Obtain the token stream
            token_stream: list[Token] = lexer(line)

            # Attempt to switch segment first
            if  switchSegment(
                    parseSegmentLabel(
                        self.logger,
                        # MUST copy the token stream here!
                        [token for token in token_stream]
                    )
                ):
                # If switch segment is successful, then ignore the line
                continue

            # The active segment should never be None
            if active_segment is None:
                self.logger.error("Missing segment declaration.")
                exitProgram(1)
                return
            
            # Use the active segment to parse the line
            active_segment.parse(token_stream)
        
        return
    
    def getSymbolTable(self) -> dict[str, int]:
        """ Obtain the global symbol table from each segment. """

        # Global symbol table
        symbol_table: dict[str, int] = dict()

        # Symbol table of each segment
        ds_st: dict[str, int] = self.ds.symbolTable()
        es_st: dict[str, int] = self.es.symbolTable()
        ts_st: dict[str, int] = self.ts.symbolTable()

        # Need to make sure no label redefinition.
        if (len(set(ds_st.keys()) & set(es_st.keys())) != 0 or
            len(set(ds_st.keys()) & set(ts_st.keys())) != 0 or
            len(set(es_st.keys()) & set(ts_st.keys())) != 0):
            self.logger.error("Label redefinition.")
            exitProgram(1)
            return
        
        # Generate the new symbol table
        # Order for segments: data -> extra -> text
        symbol_table.update(ds_st)

        ds_size: int = self.ds.size()
        for label in es_st.keys():
            es_st[label] += ds_size

        symbol_table.update(es_st)

        es_size: int = self.es.size()
        for label in ts_st.keys():
            ts_st[label] = ts_st[label] + es_size + ds_size

        symbol_table.update(ts_st)

        return symbol_table
    
    







def main():
    args = CommandlineArgs()

    logger = getConfigedLogger(
        logging.INFO, 
        '%(asctime)s | %(levelname)8s | %(message)s'
    )

    p = Parser(logger, args.filepath)
    p.parse()
    st = p.getSymbolTable()
    print(st)




if __name__ == "__main__":
    main()
