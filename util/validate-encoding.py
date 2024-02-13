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
        
        # If no exception occures
        logger.info(f"{func.__doc__} - Passed!")

    return wrapper


@testcase
def TC1():
    """ Check all instruction types defined in inst_definition has a corresponding encoding. """
    
    inst_type = set(encoding.keys())
    
    assert len(inst_type) == len(encoding.keys()), "Instruction type is not uniquely defined."
    
    for k, v in definition.items():
        assert isinstance(v, dict), f"Instruction {k} is not of dictionary type."
        assert v.get("type") in inst_type, f"Instruction {k} type is not defined in encoding.json."


@testcase
def TC2():
    """  """

        
    



        




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








if __name__ == "__main__":
    main()
