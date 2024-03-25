#!/usr/bin/env python3

import sys
from pathlib import Path

from fabric.api import prefix, run, settings, execute # type: ignore

from ci_variables import ci_env


topos = [
  "fireaxe_xilinx_u250_split_rocket_tile_from_soc_preserve_config"
# "fireaxe_xilinx_u250_split_rocket_tile_from_soc_config",
# "fireaxe_xilinx_u250_sbus_mesh_noc_eight_rocket_config"
]

log_tail_length = 200

def run_test(cfg_rt, topo, timeout):
    # Need to run firesim kill on a unexpected simulation failure
    # to kill all the process that uses shmem.
    run(f"firesim kill -c {cfg_rt}")
    run(f"firesim infrasetup -c {cfg_rt} --overrideconfigdata \"target_config topology {topo}\"")
    rc = run(f"timeout {timeout} firesim runworkload -c {cfg_rt} --overrideconfigdata \"target_config topology {topo}\" &> {topo}-runworkload.log", pty=False).return_code

    print(f"Printing last {log_tail_length} lines of log. See {topo}-runworkload.log for full info.")
    run(f"tail -n {log_tail_length} {topo}-runworkload.log")

    print(f"Printing last {log_tail_length} lines of all output files. See results-workload for more info.")
    run(f"""cd deploy/results-workload/ && LAST_DIR=$(ls | tail -n1) && if [ -d "$LAST_DIR" ]; then tail -n{log_tail_length} $LAST_DIR/*/*; fi""")

    if rc != 0:
        # need to confirm that instance is off
        print(f"topology {topo} failed. Terminating runfarm.")
        run(f"firesim terminaterunfarm -q -c {topo}")
        sys.exit(rc)
    else:
        print(f"topology {topo} successful.")
    run(f"firesim kill -c {cfg_rt}")

def run_parallel_metasim():
    """ Runs parallel baremetal metasimulations """

    # assumptions:
    #   - machine-launch-script requirements are already installed

    # repo should already be checked out

    with prefix(f"cd {ci_env['REMOTE_WORK_DIR']}"):
        with prefix('source sourceme-manager.sh --skip-ssh-setup'):
            for topo in topos:
                run_test(f"{ci_env['REMOTE_WORK_DIR']}/deploy/workloads/ci/hello-world-localhost-verilator-metasim-fireaxe.yaml", topo, "15m")

if __name__ == "__main__":
    execute(run_parallel_metasim, hosts=["localhost"])
