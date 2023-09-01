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











def main():
    args = CommandlineArgs()

    g = asmReaderGenerator(args.filepath)

    tokens: list[list[Token]] = []

    for line in g:
        tokens.append(lexer(line))

    for t in tokens:
        for s in t:
            print(s)
        print('\n')

    
     


if __name__ == "__main__":
    main()
