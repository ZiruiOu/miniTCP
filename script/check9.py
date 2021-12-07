import argparse
import os
import sys
from typing import List
from utils import install_ns, lauch_terminals_by_scripts, delete_ns

CHECKPOINT9_INSTALL_PATH: str = "script/install_topo0.sh"
CHECKPOINT9_SCRIPT_PATH: str = "script/enter_ns.sh"
CHECKPOINT9_EXEC_PATHS: List[str] = ["../../../script/exec_echo_client.sh", 
                                     "../../../script/exec_router.sh", 
                                     "../../../script/exec_router.sh",
                                     "../../../script/exec_echo_server.sh"]
CHECKPOINT9_DELETE_PATH: str = "script/exit_ns.sh"
NUM_NETWORK: int = 4

parser = argparse.ArgumentParser()
parser.add_argument("-i", "--install", help="install checkpoint topology")
parser.add_argument("-d", "--delete", help="delete installed NSs")
args = parser.parse_args()

if __name__ == "__main__":
    if args.install:
        install_ns(CHECKPOINT9_INSTALL_PATH)
        lauch_terminals_by_scripts(CHECKPOINT9_SCRIPT_PATH, NUM_NETWORK, CHECKPOINT9_EXEC_PATHS)
    elif args.delete:
        delete_ns(CHECKPOINT9_DELETE_PATH, NUM_NETWORK)