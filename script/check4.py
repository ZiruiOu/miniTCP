import argparse
import os
import sys
from utils import install_ns, lauch_terminals, delete_ns

CHECKPOINT4_INSTALL_PATH: str = "script/install_topo0.sh"
CHECKPOINT4_SCRIPT_PATH: str = "script/enter_ns.sh"
CHECKPOINT4_DELETE_PATH: str = "script/exit_ns.sh"
NUM_NETWORK: int = 4

parser = argparse.ArgumentParser()
parser.add_argument("-i", "--install", help="install checkpoint topology")
parser.add_argument("-d", "--delete", help="delete installed NSs")
args = parser.parse_args()

if __name__ == "__main__":
    if args.install:
        install_ns(CHECKPOINT4_INSTALL_PATH)
        lauch_terminals(CHECKPOINT4_SCRIPT_PATH, NUM_NETWORK)
    elif args.delete:
        delete_ns(CHECKPOINT4_DELETE_PATH, NUM_NETWORK)