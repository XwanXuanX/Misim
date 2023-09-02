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

        'SYSCALL'	#  invokes system calls	    #		SYSCALL 1				#		N/A
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


        






def main():
    args = CommandlineArgs()

    logger = getConfigedLogger(
        logging.INFO, 
        '%(asctime)s | %(levelname)8s | %(message)s'
    )
    ds = DataSegment(logger)

    for line in asmReaderGenerator(args.filepath):
        tokens = lexer(line)
        if tokens[0].value == '.':
            continue

        ds.parse(tokens)
        print(ds.value_table)



if __name__ == "__main__":
    main()
