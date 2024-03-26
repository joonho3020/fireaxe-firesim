#!/usr/bin/env python3

import yaml
import os
import argparse

parser = argparse.ArgumentParser(description="Arguments for generating config hwdb")
parser.add_argument("--bitstream-dir", type=str, required=True, help="Abs path to repo containing the bitstreams")
args = parser.parse_args()


ae_bitstreams = [
    #################################################
    # FAME5 perf sweeps
    #################################################
    "qsfp_fame5_fastmode_one_large_boom_tile_15MHz",
    "qsfp_fame5_fastmode_two_large_boom_tile_15MHz",
    "qsfp_fame5_fastmode_three_large_boom_tile_15MHz",
    "qsfp_fame5_fastmode_four_large_boom_tile_15MHz",
    "qsfp_fame5_fastmode_five_large_boom_tile_15MHz",
    "qsfp_fame5_fastmode_six_large_boom_tile_15MHz",
    "qsfp_fame5_fastmode_one_large_boom_soc_20MHz",
    "qsfp_fame5_fastmode_two_large_boom_soc_20MHz",
    "qsfp_fame5_fastmode_three_large_boom_soc_20MHz",
    "qsfp_fame5_fastmode_four_large_boom_soc_20MHz",
    "qsfp_fame5_fastmode_five_large_boom_soc_20MHz",
    "qsfp_fame5_fastmode_six_large_boom_soc_20MHz",
    "qsfp_fame5_fastmode_one_large_boom_soc_30MHz",
    "qsfp_fame5_fastmode_two_large_boom_soc_30MHz",
    "qsfp_fame5_fastmode_three_large_boom_soc_30MHz",
    "qsfp_fame5_fastmode_four_large_boom_soc_30MHz",
    "qsfp_fame5_fastmode_five_large_boom_soc_30MHz",
    "qsfp_fame5_fastmode_six_large_boom_soc_30MHz",

    #################################################
    # Fastmode 70MHz
    #################################################
    "qsfp_fastmode_one_big_rocket_soc_70MHz",
    "qsfp_fastmode_one_big_rocket_tile_70MHz",
    "qsfp_fastmode_two_big_rocket_soc_70MHz",
    "qsfp_fastmode_two_big_rocket_tile_70MHz",
    "qsfp_fastmode_four_big_rocket_soc_70MHz",
    "qsfp_fastmode_four_big_rocket_tile_70MHz",
    "qsfp_fastmode_eight_big_rocket_soc_70MHz",
    "qsfp_fastmode_eight_big_rocket_tile_70MHz",
    "qsfp_fastmode_sixteen_big_rocket_soc_70MHz",
    "qsfp_fastmode_sixteen_big_rocket_tile_70MHz",

    #################################################
    # Fastmode 50MHz
    #################################################
    "qsfp_fastmode_one_big_rocket_soc_50MHz",
    "qsfp_fastmode_one_big_rocket_tile_50MHz",
    "qsfp_fastmode_two_big_rocket_soc_50MHz",
    "qsfp_fastmode_two_big_rocket_tile_50MHz",
    "qsfp_fastmode_four_big_rocket_soc_50MHz",
    "qsfp_fastmode_four_big_rocket_tile_50MHz",
    "qsfp_fastmode_eight_big_rocket_soc_50MHz",
    "qsfp_fastmode_eight_big_rocket_tile_50MHz",
    "qsfp_fastmode_sixteen_big_rocket_soc_50MHz",
    "qsfp_fastmode_sixteen_big_rocket_tile_50MHz",

    #################################################
    # Fastmode 10MHz
    #################################################
    "qsfp_fastmode_one_big_rocket_soc_10MHz",
    "qsfp_fastmode_one_big_rocket_tile_10MHz",
    "qsfp_fastmode_two_big_rocket_soc_10MHz",
    "qsfp_fastmode_two_big_rocket_tile_10MHz",
    "qsfp_fastmode_four_big_rocket_soc_10MHz",
    "qsfp_fastmode_four_big_rocket_tile_10MHz",
    "qsfp_fastmode_eight_big_rocket_soc_10MHz",
    "qsfp_fastmode_eight_big_rocket_tile_10MHz",
    "qsfp_fastmode_sixteen_big_rocket_soc_10MHz",
    "qsfp_fastmode_sixteen_big_rocket_tile_10MHz",

    #################################################
    # Exactmode 50MHz
    #################################################
    "qsfp_exactmode_one_big_rocket_soc_50MHz",
    "qsfp_exactmode_one_big_rocket_tile_50MHz",
    "qsfp_exactmode_two_big_rocket_soc_50MHz",
    "qsfp_exactmode_two_big_rocket_tile_50MHz",
    "qsfp_exactmode_four_big_rocket_soc_50MHz",
    "qsfp_exactmode_four_big_rocket_tile_50MHz",
    "qsfp_exactmode_eight_big_rocket_soc_50MHz",
    "qsfp_exactmode_eight_big_rocket_tile_50MHz",
    "qsfp_exactmode_sixteen_big_rocket_soc_50MHz",
    "qsfp_exactmode_sixteen_big_rocket_tile_50MHz",

    #################################################
    # Exactmode 10MHz
    #################################################
    "qsfp_exactmode_one_big_rocket_soc_10MHz",
    "qsfp_exactmode_one_big_rocket_tile_10MHz",
    "qsfp_exactmode_two_big_rocket_soc_10MHz",
    "qsfp_exactmode_two_big_rocket_tile_10MHz",
    "qsfp_exactmode_four_big_rocket_soc_10MHz",
    "qsfp_exactmode_four_big_rocket_tile_10MHz",
    "qsfp_exactmode_eight_big_rocket_soc_10MHz",
    "qsfp_exactmode_eight_big_rocket_tile_10MHz",
    "qsfp_exactmode_sixteen_big_rocket_soc_10MHz",
    "qsfp_exactmode_sixteen_big_rocket_tile_10MHz",

    ################################################
    # 4 Gigaboom 20MHz (go-gc experiment)
    ################################################
    "xilinx_u250_4boom_2Ml2_base_20MHz",
    "xilinx_u250_4boom_2Ml2_split_20MHz",

    ################################################
    # Golden Cove config (core splitting experiment)
    ################################################
    "xilinx_u250_firesim_40_golden_cove_boom_backend",
    "xilinx_u250_firesim_40_golden_cove_boom_soc",

    #################################################
    # DDIO case study
    #################################################
    "xilinx_u250_firesim_nic_fame5_dodeca_boom_128kB_l2_256b_sbus_xbar_soc_10MHz",
    "xilinx_u250_firesim_nic_fame5_dodeca_boom_128kB_l2_256b_sbus_xbar_tiles_0_10MHz",
    "xilinx_u250_firesim_nic_fame5_dodeca_boom_128kB_l2_256b_sbus_xbar_tiles_1_10MHz",
    "xilinx_u250_firesim_nic_dodeca_rocket_128kB_l2_config_30MHz"
]


config_hwdb = dict()
for bitstream in ae_bitstreams:
    config_hwdb[bitstream] = dict()
    config_hwdb[bitstream]['bitstream_tar'] = f'file://{args.bitstream_dir}/xilinx_alveo_u250/{bitstream}.tar.gz'
    config_hwdb[bitstream]['deploy_quintuplet_override'] = None
    config_hwdb[bitstream]['custom_runtime_config'] = None

with open('config_hwdb.yaml', 'w') as f:
    yaml.dump(config_hwdb, f, default_flow_style=False, sort_keys=False)
