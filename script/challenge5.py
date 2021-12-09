import argparse
import os
import sys
from typing import List
from utils import install_ns, lauch_terminals_by_scripts, delete_ns

CHALLENGE5_INSTALL_PATH: str = "script/install_topo.sh"
CHALLENGE5_SCRIPT_PATH: str = "script/enter_ns.sh"
CHALLENGE5_EXEC_PATHS: List[str] = ["../../../script/exec_cong_router.sh", 
                                    "../../../script/exec_cong_client.sh",
                                    "../../../script/exec_cong_client.sh",
                                    "../../../script/exec_cong_client.sh",
                                    "../../../script/exec_cong_client.sh",
                                    "../../../script/exec_cong_client.sh",
                                    "../../../script/exec_cong_server.sh"]
CHALLENGE5_DELETE_PATH: str = "script/exit_ns.sh"
NUM_NETWORK: int = 7

parser = argparse.ArgumentParser()
parser.add_argument("-i", "--install", help="install checkpoint topology")
parser.add_argument("-d", "--delete", help="delete installed NSs")
args = parser.parse_args()

if __name__ == "__main__":
    if args.install:
        install_ns(CHALLENGE5_INSTALL_PATH, topology_name = "topology3.txt")
        lauch_terminals_by_scripts(CHALLENGE5_SCRIPT_PATH, NUM_NETWORK, CHALLENGE5_EXEC_PATHS)
    elif args.delete:
        delete_ns(CHALLENGE5_DELETE_PATH, NUM_NETWORK)