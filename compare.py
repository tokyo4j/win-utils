import os
import subprocess
import filecmp
import glob
import pathlib
import re

RED = '\033[31m'
GREEN = '\033[32m'
YELLOW = '\033[33m'
RESET = '\033[0m'

def print_colored(msg, color):
    print(f"{color}{msg}{RESET}")

for tmpl_file in glob.glob("config\\**\\*", recursive=True, include_hidden=True):
    if not os.path.isfile(tmpl_file):
        continue
    fs_file = re.sub(r"^config\\", "", tmpl_file)
    home = re.escape(os.path.expanduser('~') + "\\")
    fs_file = re.sub(r"^HOME\\", home, fs_file)
    if not os.path.exists(fs_file):
        print_colored(f"Not exist: {fs_file}", RED)
        continue
    if filecmp.cmp(tmpl_file, fs_file):
        print_colored(f"Identical: {fs_file}", GREEN)
        continue
    print_colored(f"Opening: {fs_file}", YELLOW)
    print(fs_file, tmpl_file)
    subprocess.call(
        ["code", "-d", fs_file, tmpl_file],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        shell=True,
    )
