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

SEGMENT data
Arr: 1, 34, 62, 4, 100

SEGMENT extra
Fld: 17

SEGMENT text
; Equal to move 23 to R0
XOR     R0, R0, R0
ADD     R0, R0, 23

PUSH    R0
JMP     Label

Ret:
POP     R0
END     ; Put 'END' where the program returns

Label:
; Perform very complicated calculations
JMP Ret

"""


import os
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








def main():
    args = CommandlineArgs()

    g = asmReaderGenerator(args.filepath)

    for line in g:
        print(line)
    
     


if __name__ == "__main__":
    main()
