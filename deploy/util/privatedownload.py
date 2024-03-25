import logging
import argparse
import subprocess
import os
import sys
import shlex
import time
from pathlib import Path

WLRUNNER, LBAGENERATOR = ('WLRUNNER', 'LBAGENERATOR')

rootLogger = logging.getLogger()

def shcmd(cmd, ignore_error=False):
    ret = subprocess.call(cmd, shell=True)
    if ignore_error == False and ret != 0:
        raise RuntimeError("Failed to execute {}. Return code:{}".format(
            cmd, ret))
    return ret

def downloadURIPrivate(uri, target_dir, tries: int = 4):
    token = ''
    if token == None:
        raise RuntimeError("Environment Var GITHUB_TOKEN does not exist"\
            ". Search your Evernote to find it. "
            "intitle:github")

    cmd = f"curl -H 'Authorization: token {token}' "\
        "-H 'Accept: application/vnd.github.v3.raw' -O "\
        f"-L {uri} "

    bit_tar = os.path.basename(uri)

    rootLogger.debug(f"cmd {cmd}")
    rootLogger.debug(f"uri {uri}")
    assert tries > 0, "tries arguement must be larger than 0"
    for attempt in range(tries):
        rootLogger.debug(f"Download attempt {attempt+1} of {tries}")
        try:
            shcmd(cmd)
        except Exception as e:
            if attempt < tries -1:
                time.sleep(1) # Sleep only after a failure
                continue
            else:
                raise # tries have been exhausted, raise the last exception
        rootLogger.debug(f"Successfully fetched '{uri}' to '{target_dir}'")
        break
    shcmd(f"cp {bit_tar} {target_dir}")
