import argparse
import os
import sys
from typing import List
from utils import install_ns, lauch_terminals_by_scripts, delete_ns

CHECKPOINT8_INSTALL_PATH: str = "script/install_topo1.sh"
CHECKPOINT8_SCRIPT_PATH: str = "script/enter_ns.sh"
CHECKPOINT8_EXEC_PATHS: List[str] = ["../../../script/exec_broken_send.sh", "../../../script/exec_broken_recv.sh"]
CHECKPOINT8_DELETE_PATH: str = "script/exit_ns.sh"
NUM_NETWORK: int = 2

parser = argparse.ArgumentParser()
parser.add_argument("-i", "--install", help="install checkpoint topology")
parser.add_argument("-d", "--delete", help="delete installed NSs")
args = parser.parse_args()

if __name__ == "__main__":
    if args.install:
        install_ns(CHECKPOINT8_INSTALL_PATH)
        lauch_terminals_by_scripts(CHECKPOINT8_SCRIPT_PATH, NUM_NETWORK, CHECKPOINT8_EXEC_PATHS)
    elif args.delete:
        delete_ns(CHECKPOINT8_DELETE_PATH, NUM_NETWORK)