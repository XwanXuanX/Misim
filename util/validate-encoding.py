from __future__ import annotations
from typing import *


"""
@file:   validate-encoding.py

@author: Yetong Li

@date:   Feb. 2024

@detail:

This script is used to validate the uniqueness of instruction encodings.

"""


import os
import argparse
import json
import logging
import contextlib


""" Global variables. """
logger: logging.Logger

encoding  : dict  # Dictionary holds the result of reading instruction encoding file
definition: dict  # Dictionary holds the result of reading instruction definition file


def getArgumentParser():
    """ Create and return the argument parser. """

    parser = argparse.ArgumentParser(
        prog="Validate Encodings",
        description="Validate the uniqueness of instruction encodings."
    )

    # For the instruction encoding file
    parser.add_argument(
        '-c', '--encode',
        dest='inst_encoding',
        action='store',
        type=str,
        metavar='PATH',
        required=True,
        help="Provide the path to the instruction encoding file."
    )

    # For the instruction definition file
    parser.add_argument(
        '-d', '--def',
        dest='inst_definition',
        action='store',
        type=str,
        metavar='PATH',
        required=True,
        help="Provide the path to the instruction definition file."
    )

    return parser


class CustomLoggingFormatter(logging.Formatter):
    """ Custom logging formatter for error reporting. """

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


def getConfigLogger(level: int, fmt: str) -> logging.Logger:
    """ Create and config logger """

    logger = logging.getLogger(__name__)
    logger.setLevel(level)

    # Create stdout handler
    stdout_handler = logging.StreamHandler()
    stdout_handler.setLevel(level)
    stdout_handler.setFormatter(CustomLoggingFormatter(fmt))

    logger.addHandler(stdout_handler)

    return logger


def exitProgram(exit_code: int) -> None:
    """ Log and exit program. """

    # Log exit message
    if exit_code != 0:
        logger.error("Program quitted with error...")
    else:
        logger.info("Program quitted successfully.")

    exit(None)


class CommandLineArgs:
    """ Class that stores the command line arguments """

    inst_encoding: str    # Path to the instruction encoding file.
    inst_definition: str  # Path to the instruction definition file.

    def __init__(self) -> None:
        """ When obj initialized, command line arguments will be
            automatically collected and parsed.    
        """
        self.populate()
    
    @classmethod
    def populate(cls) -> None:
        """ Get the command line arguments and populate the class entries. """
        
        # Get command line arguments
        parser = getArgumentParser()
        args = parser.parse_args()

        # Populate class entries (with absolute paths)
        cls.inst_encoding = os.path.abspath(args.inst_encoding)
        cls.inst_definition = os.path.abspath(args.inst_definition)

        # Check the arguments validity
        if cls.inst_encoding is None or cls.inst_definition is None:
            logger.error("Some required arguments are empty.")
            exitProgram(1)
            return
        
        # Check file paths
        def checkFilePath(path: str) -> None:
            """ Check if path exists and file extension."""

            if not os.path.exists(path):
                logger.error(f"{path} does not exists.")
                exitProgram(1)
                return
            
            # Get file extension
            _, ext = os.path.splitext(path)

            # File must be a json file format
            if ext != ".json":
                logger.error(f"{path} is not a json file.")
                exitProgram(1)
                return
            
            return
        
        # Check path and extension for arguments
        checkFilePath(cls.inst_encoding)
        checkFilePath(cls.inst_definition)

        # Clear of errors!
        return


def JSON2Dict(files: dict[str, str]) -> dict[str, dict]:
    """ Convert JSON file to python dictionary. """

    results: dict[str, dict] = dict()

    for file, path in files.items():
        with contextlib.closing(open(path)) as json_file:
            results[file] = json.load(json_file)
    
    return results


# # # # # # # # # # #
#     Testcases
# # # # # # # # # # #


def testcase(func: Callable) -> Callable:
    """ Testcase decorator. """
    
    def wrapper():
        try:
            func()
        except AssertionError as ae:
            logger.error(ae)
            exitProgram(1)
        except Exception as e:
            logger.error(f"Unexpected exception: {e}")
            exitProgram(1)
        
        # If no exception occurs
        logger.info(f"{func.__doc__} - Passed!")

    return wrapper


@testcase
def TC1():
    """ Check all instruction types defined in inst_definition has a corresponding encoding. """
    
    inst_type = set(encoding.keys())
    
    assert len(inst_type) == len(encoding.keys()), "Instruction type is not uniquely defined."
    
    for k, v in definition.items():
        assert isinstance(v, dict), \
               f"Instruction {k} is not of dictionary type."
        
        assert v.get("type") in inst_type, \
               f"Instruction {k} type is not defined in encoding.json."
    
    return


@testcase
def TC2():
    """ Check all instructions' opcodes and funct3/7 defined in inst_definition are unique. """

    """
    There are 3 things that differ from instructions: opcode, funct3, and funct7.

    How to validate for uniqueness?

    For example, an instruction only has opcode and funct3.
    Then we should be able to tell it apart from the rest of instructions only based on opcode and funct3.

    Do that for every single instruction.

    """

    for k, v in definition.items():
        # 3 cases (v contain only opcode, both opcode and funct3, and all 3 identifiers)

        assert isinstance(v, dict), f"Instruction {k} is not of dictionary type."

        # Case 3
        if "funct7" in v:
            assert "funct3" in v and "opcode" in v, \
                   f"Instruction {k} has funct7, but doesn't have funct3 or opcode"
            
            # The opcode, funct7, and funct3 combined should uniquely identify that instruction
            encounter: int = 0

            for encoding in definition.values():
                # Concat checker's opcode, funct3, and funct7
                checker = v.get("funct7") + v.get("funct3") + v.get("opcode")
                # Concat target's opcode, funct3, and funct7
                # Assumption: ALL instruction MUST have the opcode section!
                assert "opcode" in encoding, "Some instruction doesn't contain opcode!"
                if "funct7" not in encoding:
                    continue
                
                target = encoding.get("funct7")   \
                         + encoding.get("funct3") \
                         + encoding.get("opcode")
                
                if checker == target:
                    encounter += 1
            
            assert encounter == 1, \
                   f"funct7, funct3, and opcode does not uniquely identify instruction {k}"

        # Case 2
        elif "funct3" in v:
            assert "funct7" not in v and "opcode" in v, \
                   f"Instruction {k} has funct3, but has funct7 or doesn't have opcode"
            
            # The opcode and funct3 combined should uniquely identify that instruction
            encounter: int = 0

            for encoding in definition.values():
                # Concat checker's opcode and funct3
                checker = v.get("funct3") + v.get("opcode")
                # Concat target's opcode and funct3
                # Assumption: ALL instruction MUST have the opcode section!
                assert "opcode" in encoding, "Some instruction doesn't contain opcode!"
                # But if the target doesn't have funct3, it will be checked in the
                # next 'elif' clause, which is still okay.
                if "funct3" not in encoding:
                    continue
                
                target = encoding.get("funct3") + encoding.get("opcode")

                if checker == target:
                    encounter += 1
            
            assert encounter == 1, \
                   f"funct3 & opcode does not uniquely identify instruction {k}"

        # Case 1
        elif "opcode" in v:
            assert "funct3" not in v and "funct7" not in v, \
                   f"Instruction {k} has opcode, but has funct3 or funct7"

            # Only the opcode should uniquely identify that instruction
            encounter: int = 0

            for encoding in definition.values():
                # Assumption: ALL instruction MUST have the opcode section!
                assert "opcode" in encoding, "Some instruction doesn't contain opcode!"

                if v.get("opcode") == encoding.get("opcode"):
                    encounter += 1
            
            assert encounter == 1, \
                   f"Only the opcode does not uniquely identify instruction {k}"
    
    return


        
    



        




def main():
    """ Main function, entry point to the program. """

    # Get the reference of global objects
    global logger
    global encoding
    global definition

    # Create the logger
    logger = getConfigLogger(
        logging.INFO, 
        '%(asctime)s | %(levelname)8s | %(message)s'
    )

    # Parse arguments
    args = CommandLineArgs()

    # Convert JSON to dictionary
    dicts: dict[str, dict] = JSON2Dict({"encoding" : args.inst_encoding,
                                        "definition" : args.inst_definition})
    encoding = dicts.get("encoding")
    definition = dicts.get("definition")

    if encoding is None or definition is None:
        logger.error("Failed to parse JSON files.")
        exitProgram(1)
        return
    
    # Testcases
    
    TC1()
    TC2()








if __name__ == "__main__":
    main()
