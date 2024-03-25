
################################################################
# This is a generated script based on design: design_1
#
# Though there are limitations about the generated script,
# the main purpose of this utility is to make learning
# IP Integrator Tcl commands easier.
################################################################

namespace eval _tcl {
proc get_script_folder {} {
   set script_path [file normalize [info script]]
   set script_folder [file dirname $script_path]
   return $script_folder
}
}
variable script_folder
set script_folder [_tcl::get_script_folder]

################################################################
# Check if script is running in correct Vivado version.
################################################################
set scripts_vivado_version 2021.1
set current_vivado_version [version -short]

if { [string first $scripts_vivado_version $current_vivado_version] == -1 } {
   puts ""
   catch {common::send_gid_msg -ssname BD::TCL -id 2041 -severity "ERROR" "This script was generated using Vivado <$scripts_vivado_version> and is being run in <$current_vivado_version> of Vivado. Please run the script in Vivado <$scripts_vivado_version> then open the design in Vivado <$current_vivado_version>. Upgrade the design by running \"Tools => Report => Report IP Status...\", then run write_bd_tcl to create an updated script."}

   return 1
}

################################################################
# START
################################################################

# To test this script, run the following commands from Vivado Tcl console:
# source design_1_script.tcl


# The design that will be created by this Tcl script contains the following 
# module references:
# aurora_64b66b_0_driver, aurora_64b66b_0_driver, aurora_gt_wrapper, aurora_gt_wrapper, axi_tieoff_master, firesim_wrapper

# Please add the sources of those modules before sourcing this Tcl script.

# If there is no project opened, this script will create a
# project, but make sure you do not have an existing project
# <./myproj/project_1.xpr> in the current working folder.

set list_projs [get_projects -quiet]
if { $list_projs eq "" } {
   create_project project_1 myproj -part xcu250-figd2104-2L-e
   set_property BOARD_PART xilinx.com:au250:part0:1.3 [current_project]
}


# CHANGE DESIGN NAME HERE
variable design_name
set design_name design_1

# If you do not already have an existing IP Integrator design open,
# you can create a design using the following command:
#    create_bd_design $design_name

# Creating design if needed
set errMsg ""
set nRet 0

set cur_design [current_bd_design -quiet]
set list_cells [get_bd_cells -quiet]

if { ${design_name} eq "" } {
   # USE CASES:
   #    1) Design_name not set

   set errMsg "Please set the variable <design_name> to a non-empty value."
   set nRet 1

} elseif { ${cur_design} ne "" && ${list_cells} eq "" } {
   # USE CASES:
   #    2): Current design opened AND is empty AND names same.
   #    3): Current design opened AND is empty AND names diff; design_name NOT in project.
   #    4): Current design opened AND is empty AND names diff; design_name exists in project.

   if { $cur_design ne $design_name } {
      common::send_gid_msg -ssname BD::TCL -id 2001 -severity "INFO" "Changing value of <design_name> from <$design_name> to <$cur_design> since current design is empty."
      set design_name [get_property NAME $cur_design]
   }
   common::send_gid_msg -ssname BD::TCL -id 2002 -severity "INFO" "Constructing design in IPI design <$cur_design>..."

} elseif { ${cur_design} ne "" && $list_cells ne "" && $cur_design eq $design_name } {
   # USE CASES:
   #    5) Current design opened AND has components AND same names.

   set errMsg "Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 1
} elseif { [get_files -quiet ${design_name}.bd] ne "" } {
   # USE CASES: 
   #    6) Current opened design, has components, but diff names, design_name exists in project.
   #    7) No opened design, design_name exists in project.

   set errMsg "Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 2

} else {
   # USE CASES:
   #    8) No opened design, design_name not in project.
   #    9) Current opened design, has components, but diff names, design_name not in project.

   common::send_gid_msg -ssname BD::TCL -id 2003 -severity "INFO" "Currently there is no design <$design_name> in project, so creating one..."

   create_bd_design $design_name

   common::send_gid_msg -ssname BD::TCL -id 2004 -severity "INFO" "Making design <$design_name> as current_bd_design."
   current_bd_design $design_name

}

common::send_gid_msg -ssname BD::TCL -id 2005 -severity "INFO" "Currently the variable <design_name> is equal to \"$design_name\"."

if { $nRet != 0 } {
   catch {common::send_gid_msg -ssname BD::TCL -id 2006 -severity "ERROR" $errMsg}
   return $nRet
}

set bCheckIPsPassed 1
##################################################################
# CHECK IPs
##################################################################
set bCheckIPs 1
if { $bCheckIPs == 1 } {
   set list_check_ips "\ 
xilinx.com:ip:aurora_64b66b:12.0\
xilinx.com:ip:axi_clock_converter:2.1\
xilinx.com:ip:axi_dwidth_converter:2.1\
xilinx.com:ip:axis_clock_converter:1.1\
xilinx.com:ip:axis_data_fifo:2.0\
xilinx.com:ip:clk_wiz:6.0\
xilinx.com:ip:ddr4:2.2\
xilinx.com:ip:proc_sys_reset:5.0\
xilinx.com:ip:util_vector_logic:2.0\
xilinx.com:ip:util_ds_buf:2.2\
xilinx.com:ip:xdma:4.1\
xilinx.com:ip:xlconstant:1.1\
"

   set list_ips_missing ""
   common::send_gid_msg -ssname BD::TCL -id 2011 -severity "INFO" "Checking if the following IPs exist in the project's IP catalog: $list_check_ips ."

   foreach ip_vlnv $list_check_ips {
      set ip_obj [get_ipdefs -all $ip_vlnv]
      if { $ip_obj eq "" } {
         lappend list_ips_missing $ip_vlnv
      }
   }

   if { $list_ips_missing ne "" } {
      catch {common::send_gid_msg -ssname BD::TCL -id 2012 -severity "ERROR" "The following IPs are not found in the IP Catalog:\n  $list_ips_missing\n\nResolution: Please add the repository containing the IP(s) to the project." }
      set bCheckIPsPassed 0
   }

}

##################################################################
# CHECK Modules
##################################################################
set bCheckModules 1
if { $bCheckModules == 1 } {
   set list_check_mods "\ 
aurora_64b66b_0_driver\
aurora_64b66b_0_driver\
aurora_gt_wrapper\
aurora_gt_wrapper\
axi_tieoff_master\
firesim_wrapper\
"

   set list_mods_missing ""
   common::send_gid_msg -ssname BD::TCL -id 2020 -severity "INFO" "Checking if the following modules exist in the project's sources: $list_check_mods ."

   foreach mod_vlnv $list_check_mods {
      if { [can_resolve_reference $mod_vlnv] == 0 } {
         lappend list_mods_missing $mod_vlnv
      }
   }

   if { $list_mods_missing ne "" } {
      catch {common::send_gid_msg -ssname BD::TCL -id 2021 -severity "ERROR" "The following module(s) are not found in the project: $list_mods_missing" }
      common::send_gid_msg -ssname BD::TCL -id 2022 -severity "INFO" "Please add source files for the missing module(s) above."
      set bCheckIPsPassed 0
   }
}

if { $bCheckIPsPassed != 1 } {
  common::send_gid_msg -ssname BD::TCL -id 2023 -severity "WARNING" "Will not continue with creation of design due to the error(s) above."
  return 3
}

##################################################################
# DESIGN PROCs
##################################################################



# Procedure to create entire design; Provide argument to make
# procedure reusable. If parentCell is "", will use root.
proc create_root_design { parentCell firesim_freq } {

  variable script_folder
  variable design_name

  if { $parentCell eq "" } {
     set parentCell [get_bd_cells /]
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2090 -severity "ERROR" "Unable to find parent cell <$parentCell>!"}
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2091 -severity "ERROR" "Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."}
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj


  # Create interface ports
  set ddr4_sdram_c0 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:ddr4_rtl:1.0 ddr4_sdram_c0 ]

  set default_300mhz_clk0 [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 default_300mhz_clk0 ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {300000000} \
   ] $default_300mhz_clk0

  set default_300mhz_clk1 [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 default_300mhz_clk1 ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {300000000} \
   ] $default_300mhz_clk1

  set default_300mhz_clk2 [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 default_300mhz_clk2 ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {300000000} \
   ] $default_300mhz_clk2

  set pci_express_x16 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:pcie_7x_mgt_rtl:1.0 pci_express_x16 ]

  set pcie_refclk [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 pcie_refclk ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {100000000} \
   ] $pcie_refclk

  set qsfp0_156mhz [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 qsfp0_156mhz ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {156250000} \
   ] $qsfp0_156mhz

  set qsfp0_4x [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gt_rtl:1.0 qsfp0_4x ]

  set qsfp1_156mhz [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 qsfp1_156mhz ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {156250000} \
   ] $qsfp1_156mhz

  set qsfp1_4x [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gt_rtl:1.0 qsfp1_4x ]


  # Create ports
  set pcie_perstn [ create_bd_port -dir I -type rst pcie_perstn ]
  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_LOW} \
 ] $pcie_perstn
  set resetn [ create_bd_port -dir I -type rst resetn ]
  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_LOW} \
 ] $resetn

  # Create instance: aurora_64b66b_0, and set properties
  set aurora_64b66b_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:aurora_64b66b:12.0 aurora_64b66b_0 ]
  set_property -dict [ list \
   CONFIG.CHANNEL_ENABLE {X1Y44 X1Y45 X1Y46 X1Y47} \
   CONFIG.C_AURORA_LANES {4} \
   CONFIG.C_GT_LOC_2 {2} \
   CONFIG.C_GT_LOC_3 {3} \
   CONFIG.C_GT_LOC_4 {4} \
   CONFIG.C_LINE_RATE {15} \
   CONFIG.C_REFCLK_SOURCE {MGTREFCLK0_of_Quad_X1Y11} \
   CONFIG.C_START_LANE {X1Y44} \
   CONFIG.C_START_QUAD {Quad_X1Y11} \
   CONFIG.SupportLevel {1} \
   CONFIG.drp_mode {Disabled} \
   CONFIG.interface_mode {Streaming} \
 ] $aurora_64b66b_0

  # Create instance: aurora_64b66b_0_driver, and set properties
  set block_name aurora_64b66b_0_driver
  set block_cell_name aurora_64b66b_0_driver
  if { [catch {set aurora_64b66b_0_driver [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $aurora_64b66b_0_driver eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_HIGH} \
 ] [get_bd_pins /aurora_64b66b_0_driver/reset_pb]

  # Create instance: aurora_64b66b_1, and set properties
  set aurora_64b66b_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:aurora_64b66b:12.0 aurora_64b66b_1 ]
  set_property -dict [ list \
   CONFIG.CHANNEL_ENABLE {X1Y40 X1Y41 X1Y42 X1Y43} \
   CONFIG.C_AURORA_LANES {4} \
   CONFIG.C_GT_LOC_2 {2} \
   CONFIG.C_GT_LOC_3 {3} \
   CONFIG.C_GT_LOC_4 {4} \
   CONFIG.C_LINE_RATE {15} \
   CONFIG.C_REFCLK_SOURCE {MGTREFCLK0_of_Quad_X1Y10} \
   CONFIG.C_START_LANE {X1Y40} \
   CONFIG.C_START_QUAD {Quad_X1Y10} \
   CONFIG.SupportLevel {1} \
   CONFIG.drp_mode {Disabled} \
   CONFIG.interface_mode {Streaming} \
 ] $aurora_64b66b_1

  # Create instance: aurora_64b66b_1_driver, and set properties
  set block_name aurora_64b66b_0_driver
  set block_cell_name aurora_64b66b_1_driver
  if { [catch {set aurora_64b66b_1_driver [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $aurora_64b66b_1_driver eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_HIGH} \
 ] [get_bd_pins /aurora_64b66b_1_driver/reset_pb]

  # Create instance: aurora_gt_wrapper_0, and set properties
  set block_name aurora_gt_wrapper
  set block_cell_name aurora_gt_wrapper_0
  if { [catch {set aurora_gt_wrapper_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $aurora_gt_wrapper_0 eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: aurora_gt_wrapper_1, and set properties
  set block_name aurora_gt_wrapper
  set block_cell_name aurora_gt_wrapper_1
  if { [catch {set aurora_gt_wrapper_1 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $aurora_gt_wrapper_1 eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: axi_clock_converter_0, and set properties
  set axi_clock_converter_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_clock_converter:2.1 axi_clock_converter_0 ]

  # Create instance: axi_clock_converter_1, and set properties
  set axi_clock_converter_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_clock_converter:2.1 axi_clock_converter_1 ]

  # Create instance: axi_dwidth_converter_0, and set properties
  set axi_dwidth_converter_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dwidth_converter:2.1 axi_dwidth_converter_0 ]
  set_property -dict [ list \
   CONFIG.ACLK_ASYNC {1} \
   CONFIG.FIFO_MODE {2} \
   CONFIG.MI_DATA_WIDTH {512} \
   CONFIG.SI_DATA_WIDTH {64} \
   CONFIG.SI_ID_WIDTH {16} \
 ] $axi_dwidth_converter_0

  # Create instance: axi_tieoff_master_0, and set properties
  set block_name axi_tieoff_master
  set block_cell_name axi_tieoff_master_0
  if { [catch {set axi_tieoff_master_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $axi_tieoff_master_0 eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: axis_clock_converter_0, and set properties
  set axis_clock_converter_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_clock_converter:1.1 axis_clock_converter_0 ]
  set_property -dict [ list \
   CONFIG.SYNCHRONIZATION_STAGES {3} \
   CONFIG.TDATA_NUM_BYTES {32} \
 ] $axis_clock_converter_0

  # Create instance: axis_clock_converter_1, and set properties
  set axis_clock_converter_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_clock_converter:1.1 axis_clock_converter_1 ]
  set_property -dict [ list \
   CONFIG.SYNCHRONIZATION_STAGES {3} \
   CONFIG.TDATA_NUM_BYTES {32} \
 ] $axis_clock_converter_1

  # Create instance: axis_clock_converter_2, and set properties
  set axis_clock_converter_2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_clock_converter:1.1 axis_clock_converter_2 ]
  set_property -dict [ list \
   CONFIG.SYNCHRONIZATION_STAGES {3} \
   CONFIG.TDATA_NUM_BYTES {32} \
 ] $axis_clock_converter_2

  # Create instance: axis_clock_converter_3, and set properties
  set axis_clock_converter_3 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_clock_converter:1.1 axis_clock_converter_3 ]
  set_property -dict [ list \
   CONFIG.SYNCHRONIZATION_STAGES {3} \
   CONFIG.TDATA_NUM_BYTES {32} \
 ] $axis_clock_converter_3

  # Create instance: axis_data_fifo_0, and set properties
  set axis_data_fifo_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:2.0 axis_data_fifo_0 ]
  set_property -dict [ list \
   CONFIG.FIFO_DEPTH {2048} \
   CONFIG.TDATA_NUM_BYTES {32} \
 ] $axis_data_fifo_0

  # Create instance: axis_data_fifo_1, and set properties
  set axis_data_fifo_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:2.0 axis_data_fifo_1 ]
  set_property -dict [ list \
   CONFIG.FIFO_DEPTH {2048} \
   CONFIG.TDATA_NUM_BYTES {32} \
 ] $axis_data_fifo_1

  # Create instance: axis_data_fifo_2, and set properties
  set axis_data_fifo_2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:2.0 axis_data_fifo_2 ]
  set_property -dict [ list \
   CONFIG.FIFO_DEPTH {2048} \
   CONFIG.TDATA_NUM_BYTES {32} \
 ] $axis_data_fifo_2

  # Create instance: axis_data_fifo_3, and set properties
  set axis_data_fifo_3 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_data_fifo:2.0 axis_data_fifo_3 ]
  set_property -dict [ list \
   CONFIG.FIFO_DEPTH {2048} \
   CONFIG.TDATA_NUM_BYTES {32} \
 ] $axis_data_fifo_3

  # Create instance: clk_wiz_0, and set properties
  set clk_wiz_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:clk_wiz:6.0 clk_wiz_0 ]
  set_property -dict [ list \
   CONFIG.CLKOUT1_REQUESTED_OUT_FREQ $firesim_freq \
   CONFIG.USE_LOCKED {false} \
 ] $clk_wiz_0

  # Create instance: clk_wiz_aurora_0, and set properties
  set clk_wiz_aurora_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:clk_wiz:6.0 clk_wiz_aurora_0 ]
  set_property -dict [ list \
   CONFIG.CLKIN1_JITTER_PS {33.330000000000005} \
   CONFIG.CLKOUT1_JITTER {101.475} \
   CONFIG.CLKOUT1_PHASE_ERROR {77.836} \
   CONFIG.CLKOUT1_REQUESTED_OUT_FREQ {100} \
   CONFIG.MMCM_CLKFBOUT_MULT_F {4.000} \
   CONFIG.MMCM_CLKIN1_PERIOD {3.333} \
   CONFIG.MMCM_CLKIN2_PERIOD {10.0} \
   CONFIG.MMCM_CLKOUT0_DIVIDE_F {12.000} \
   CONFIG.MMCM_DIVCLK_DIVIDE {1} \
   CONFIG.PRIM_IN_FREQ {300.000} \
   CONFIG.PRIM_SOURCE {Differential_clock_capable_pin} \
 ] $clk_wiz_aurora_0

  # Create instance: clk_wiz_aurora_1, and set properties
  set clk_wiz_aurora_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:clk_wiz:6.0 clk_wiz_aurora_1 ]
  set_property -dict [ list \
   CONFIG.CLKIN1_JITTER_PS {33.330000000000005} \
   CONFIG.CLKOUT1_JITTER {101.475} \
   CONFIG.CLKOUT1_PHASE_ERROR {77.836} \
   CONFIG.CLKOUT1_REQUESTED_OUT_FREQ {100} \
   CONFIG.MMCM_CLKFBOUT_MULT_F {4.000} \
   CONFIG.MMCM_CLKIN1_PERIOD {3.333} \
   CONFIG.MMCM_CLKIN2_PERIOD {10.0} \
   CONFIG.MMCM_CLKOUT0_DIVIDE_F {12.000} \
   CONFIG.MMCM_DIVCLK_DIVIDE {1} \
   CONFIG.PRIM_IN_FREQ {300.000} \
   CONFIG.PRIM_SOURCE {Differential_clock_capable_pin} \
 ] $clk_wiz_aurora_1

  # Create instance: ddr4_0, and set properties
  set ddr4_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:ddr4:2.2 ddr4_0 ]
  set_property -dict [ list \
   CONFIG.C0.DDR4_AUTO_AP_COL_A3 {true} \
   CONFIG.C0.DDR4_InputClockPeriod {3332} \
   CONFIG.C0.DDR4_MCS_ECC {false} \
   CONFIG.C0_CLOCK_BOARD_INTERFACE {default_300mhz_clk0} \
   CONFIG.C0_DDR4_BOARD_INTERFACE {ddr4_sdram_c0} \
   CONFIG.Debug_Signal {Disable} \
   CONFIG.RESET_BOARD_INTERFACE {resetn} \
 ] $ddr4_0

  # Create instance: firesim_wrapper_0, and set properties
  set block_name firesim_wrapper
  set block_cell_name firesim_wrapper_0
  if { [catch {set firesim_wrapper_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $firesim_wrapper_0 eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: proc_sys_reset_0, and set properties
  set proc_sys_reset_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 proc_sys_reset_0 ]

  # Create instance: proc_sys_reset_1, and set properties
  set proc_sys_reset_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 proc_sys_reset_1 ]

  # Create instance: resetn_inv_0, and set properties
  set resetn_inv_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_vector_logic:2.0 resetn_inv_0 ]
  set_property -dict [ list \
   CONFIG.C_OPERATION {not} \
   CONFIG.C_SIZE {1} \
 ] $resetn_inv_0

  # Create instance: util_ds_buf, and set properties
  set util_ds_buf [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_ds_buf:2.2 util_ds_buf ]
  set_property -dict [ list \
   CONFIG.C_BUF_TYPE {IBUFDSGTE} \
   CONFIG.DIFF_CLK_IN_BOARD_INTERFACE {pcie_refclk} \
   CONFIG.USE_BOARD_FLOW {true} \
 ] $util_ds_buf

  # Create instance: util_vector_logic_0, and set properties
  set util_vector_logic_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_vector_logic:2.0 util_vector_logic_0 ]
  set_property -dict [ list \
   CONFIG.C_OPERATION {not} \
   CONFIG.C_SIZE {1} \
   CONFIG.LOGO_FILE {data/sym_notgate.png} \
 ] $util_vector_logic_0

  # Create instance: util_vector_logic_1, and set properties
  set util_vector_logic_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_vector_logic:2.0 util_vector_logic_1 ]
  set_property -dict [ list \
   CONFIG.C_OPERATION {not} \
   CONFIG.C_SIZE {1} \
   CONFIG.LOGO_FILE {data/sym_notgate.png} \
 ] $util_vector_logic_1

  # Create instance: xdma_0, and set properties
  set xdma_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xdma:4.1 xdma_0 ]
  set_property -dict [ list \
   CONFIG.PCIE_BOARD_INTERFACE {pci_express_x16} \
   CONFIG.SYS_RST_N_BOARD_INTERFACE {pcie_perstn} \
   CONFIG.axilite_master_en {true} \
   CONFIG.axilite_master_size {32} \
   CONFIG.pcie_id_if {true} \
   CONFIG.pciebar2axibar_axist_bypass {0x0000000000000000} \
   CONFIG.pf0_msix_cap_pba_bir {BAR_1} \
   CONFIG.pf0_msix_cap_table_bir {BAR_1} \
   CONFIG.xdma_axi_intf_mm {AXI_Memory_Mapped} \
   CONFIG.xdma_rnum_chnl {4} \
   CONFIG.xdma_wnum_chnl {4} \
 ] $xdma_0

  # Create instance: xlconstant_0, and set properties
  set xlconstant_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 xlconstant_0 ]
  set_property -dict [ list \
   CONFIG.CONST_VAL {0} \
 ] $xlconstant_0

  # Create interface connections
  connect_bd_intf_net -intf_net aurora_64b66b_0_USER_DATA_M_AXIS_RX [get_bd_intf_pins aurora_64b66b_0/USER_DATA_M_AXIS_RX] [get_bd_intf_pins axis_data_fifo_0/S_AXIS]
  connect_bd_intf_net -intf_net aurora_64b66b_1_USER_DATA_M_AXIS_RX [get_bd_intf_pins aurora_64b66b_1/USER_DATA_M_AXIS_RX] [get_bd_intf_pins axis_data_fifo_2/S_AXIS]
  connect_bd_intf_net -intf_net aurora_out_0 [get_bd_intf_ports qsfp0_4x] [get_bd_intf_pins aurora_gt_wrapper_0/QSFP_GT]
  connect_bd_intf_net -intf_net aurora_out_1 [get_bd_intf_ports qsfp1_4x] [get_bd_intf_pins aurora_gt_wrapper_1/QSFP_GT]
  connect_bd_intf_net -intf_net axi_clock_converter_0_M_AXI [get_bd_intf_pins axi_clock_converter_0/M_AXI] [get_bd_intf_pins firesim_wrapper_0/S_AXI_DMA]
  connect_bd_intf_net -intf_net axi_clock_converter_1_M_AXI [get_bd_intf_pins axi_clock_converter_1/M_AXI] [get_bd_intf_pins firesim_wrapper_0/S_AXI_CTRL]
  connect_bd_intf_net -intf_net axi_dwidth_converter_0_M_AXI [get_bd_intf_pins axi_dwidth_converter_0/M_AXI] [get_bd_intf_pins ddr4_0/C0_DDR4_S_AXI]
  connect_bd_intf_net -intf_net axi_tieoff_master_0_TIEOFF_M_AXI_CTRL_0 [get_bd_intf_pins axi_tieoff_master_0/TIEOFF_M_AXI_CTRL_0] [get_bd_intf_pins ddr4_0/C0_DDR4_S_AXI_CTRL]
  connect_bd_intf_net -intf_net axis_clock_converter_1_M_AXIS [get_bd_intf_pins axis_clock_converter_1/M_AXIS] [get_bd_intf_pins axis_data_fifo_3/S_AXIS]
  connect_bd_intf_net -intf_net axis_clock_converter_3_M_AXIS [get_bd_intf_pins axis_clock_converter_3/M_AXIS] [get_bd_intf_pins axis_data_fifo_1/S_AXIS]
  connect_bd_intf_net -intf_net axis_data_fifo_0_M_AXIS [get_bd_intf_pins axis_clock_converter_2/S_AXIS] [get_bd_intf_pins axis_data_fifo_0/M_AXIS]
  connect_bd_intf_net -intf_net axis_data_fifo_1_M_AXIS [get_bd_intf_pins aurora_64b66b_0/USER_DATA_S_AXIS_TX] [get_bd_intf_pins axis_data_fifo_1/M_AXIS]
  connect_bd_intf_net -intf_net axis_data_fifo_2_M_AXIS [get_bd_intf_pins axis_clock_converter_0/S_AXIS] [get_bd_intf_pins axis_data_fifo_2/M_AXIS]
  connect_bd_intf_net -intf_net axis_data_fifo_3_M_AXIS [get_bd_intf_pins aurora_64b66b_1/USER_DATA_S_AXIS_TX] [get_bd_intf_pins axis_data_fifo_3/M_AXIS]
  connect_bd_intf_net -intf_net ddr4_0_C0_DDR4 [get_bd_intf_ports ddr4_sdram_c0] [get_bd_intf_pins ddr4_0/C0_DDR4]
  connect_bd_intf_net -intf_net default_100mhz_clk2_1 [get_bd_intf_ports default_300mhz_clk2] [get_bd_intf_pins clk_wiz_aurora_1/CLK_IN1_D]
  connect_bd_intf_net -intf_net default_300mhz_clk0_1 [get_bd_intf_ports default_300mhz_clk0] [get_bd_intf_pins ddr4_0/C0_SYS_CLK]
  connect_bd_intf_net -intf_net default_300mhz_clk1_1 [get_bd_intf_ports default_300mhz_clk1] [get_bd_intf_pins clk_wiz_aurora_0/CLK_IN1_D]
  connect_bd_intf_net -intf_net firesim_wrapper_0_M_AXI_DDR0 [get_bd_intf_pins axi_dwidth_converter_0/S_AXI] [get_bd_intf_pins firesim_wrapper_0/M_AXI_DDR0]
  connect_bd_intf_net -intf_net pcie_refclk_1 [get_bd_intf_ports pcie_refclk] [get_bd_intf_pins util_ds_buf/CLK_IN_D]
  connect_bd_intf_net -intf_net qsfp0_156mhz_1 [get_bd_intf_ports qsfp0_156mhz] [get_bd_intf_pins aurora_64b66b_0/GT_DIFF_REFCLK1]
  connect_bd_intf_net -intf_net qsfp1_156mhz_1 [get_bd_intf_ports qsfp1_156mhz] [get_bd_intf_pins aurora_64b66b_1/GT_DIFF_REFCLK1]
  connect_bd_intf_net -intf_net xdma_0_M_AXI [get_bd_intf_pins axi_clock_converter_0/S_AXI] [get_bd_intf_pins xdma_0/M_AXI]
  connect_bd_intf_net -intf_net xdma_0_M_AXI_LITE [get_bd_intf_pins axi_clock_converter_1/S_AXI] [get_bd_intf_pins xdma_0/M_AXI_LITE]
  connect_bd_intf_net -intf_net xdma_0_pcie_mgt [get_bd_intf_ports pci_express_x16] [get_bd_intf_pins xdma_0/pcie_mgt]

  # Create port connections
  connect_bd_net -net Net [get_bd_pins aurora_64b66b_1/user_clk_out] [get_bd_pins aurora_64b66b_1_driver/user_clk_i] [get_bd_pins axis_clock_converter_0/s_axis_aclk] [get_bd_pins axis_clock_converter_1/m_axis_aclk] [get_bd_pins axis_data_fifo_2/s_axis_aclk] [get_bd_pins axis_data_fifo_3/s_axis_aclk]
  connect_bd_net -net aurora_64b66b_0_channel_up [get_bd_pins aurora_64b66b_0/channel_up] [get_bd_pins aurora_64b66b_0_driver/channel_up_i] [get_bd_pins firesim_wrapper_0/QSFP0_CHANNEL_UP]
  connect_bd_net -net aurora_64b66b_0_driver_INIT_CLK_i [get_bd_pins aurora_64b66b_0/init_clk] [get_bd_pins aurora_64b66b_0_driver/INIT_CLK_i]
  connect_bd_net -net aurora_64b66b_0_driver_gt_reset_i [get_bd_pins aurora_64b66b_0/pma_init] [get_bd_pins aurora_64b66b_0_driver/gt_reset_i]
  connect_bd_net -net aurora_64b66b_0_driver_gt_rxcdrovrden_i [get_bd_pins aurora_64b66b_0/gt_rxcdrovrden_in] [get_bd_pins aurora_64b66b_0_driver/gt_rxcdrovrden_i]
  connect_bd_net -net aurora_64b66b_0_driver_loopback_i [get_bd_pins aurora_64b66b_0/loopback] [get_bd_pins aurora_64b66b_0_driver/loopback_i]
  connect_bd_net -net aurora_64b66b_0_driver_power_down_i [get_bd_pins aurora_64b66b_0/power_down] [get_bd_pins aurora_64b66b_0_driver/power_down_i]
  connect_bd_net -net aurora_64b66b_0_driver_reset_pb [get_bd_pins aurora_64b66b_0/reset_pb] [get_bd_pins aurora_64b66b_0_driver/reset_pb]
  connect_bd_net -net aurora_64b66b_0_sys_reset_out [get_bd_pins aurora_64b66b_0/sys_reset_out] [get_bd_pins aurora_64b66b_0_driver/system_reset_i] [get_bd_pins util_vector_logic_1/Op1]
  connect_bd_net -net aurora_64b66b_0_txn [get_bd_pins aurora_64b66b_0/txn] [get_bd_pins aurora_gt_wrapper_0/TXN_in]
  connect_bd_net -net aurora_64b66b_0_txp [get_bd_pins aurora_64b66b_0/txp] [get_bd_pins aurora_gt_wrapper_0/TXP_in]
  connect_bd_net -net aurora_64b66b_0_user_clk_out [get_bd_pins aurora_64b66b_0/user_clk_out] [get_bd_pins aurora_64b66b_0_driver/user_clk_i] [get_bd_pins axis_clock_converter_2/s_axis_aclk] [get_bd_pins axis_clock_converter_3/m_axis_aclk] [get_bd_pins axis_data_fifo_0/s_axis_aclk] [get_bd_pins axis_data_fifo_1/s_axis_aclk]
  connect_bd_net -net aurora_64b66b_1_channel_up [get_bd_pins aurora_64b66b_1/channel_up] [get_bd_pins aurora_64b66b_1_driver/channel_up_i] [get_bd_pins firesim_wrapper_0/QSFP1_CHANNEL_UP]
  connect_bd_net -net aurora_64b66b_1_driver_INIT_CLK_i [get_bd_pins aurora_64b66b_1/init_clk] [get_bd_pins aurora_64b66b_1_driver/INIT_CLK_i]
  connect_bd_net -net aurora_64b66b_1_driver_gt_reset_i [get_bd_pins aurora_64b66b_1/pma_init] [get_bd_pins aurora_64b66b_1_driver/gt_reset_i]
  connect_bd_net -net aurora_64b66b_1_driver_gt_rxcdrovrden_i [get_bd_pins aurora_64b66b_1/gt_rxcdrovrden_in] [get_bd_pins aurora_64b66b_1_driver/gt_rxcdrovrden_i]
  connect_bd_net -net aurora_64b66b_1_driver_loopback_i [get_bd_pins aurora_64b66b_1/loopback] [get_bd_pins aurora_64b66b_1_driver/loopback_i]
  connect_bd_net -net aurora_64b66b_1_driver_power_down_i [get_bd_pins aurora_64b66b_1/power_down] [get_bd_pins aurora_64b66b_1_driver/power_down_i]
  connect_bd_net -net aurora_64b66b_1_driver_reset_pb [get_bd_pins aurora_64b66b_1/reset_pb] [get_bd_pins aurora_64b66b_1_driver/reset_pb]
  connect_bd_net -net aurora_64b66b_1_sys_reset_out [get_bd_pins aurora_64b66b_1/sys_reset_out] [get_bd_pins aurora_64b66b_1_driver/system_reset_i] [get_bd_pins util_vector_logic_0/Op1]
  connect_bd_net -net aurora_64b66b_1_txn [get_bd_pins aurora_64b66b_1/txn] [get_bd_pins aurora_gt_wrapper_1/TXN_in]
  connect_bd_net -net aurora_64b66b_1_txp [get_bd_pins aurora_64b66b_1/txp] [get_bd_pins aurora_gt_wrapper_1/TXP_in]
  connect_bd_net -net aurora_gt_wrapper_0_RXN_in [get_bd_pins aurora_64b66b_0/rxn] [get_bd_pins aurora_gt_wrapper_0/RXN_in]
  connect_bd_net -net aurora_gt_wrapper_0_RXP_in [get_bd_pins aurora_64b66b_0/rxp] [get_bd_pins aurora_gt_wrapper_0/RXP_in]
  connect_bd_net -net aurora_gt_wrapper_1_RXN_in [get_bd_pins aurora_64b66b_1/rxn] [get_bd_pins aurora_gt_wrapper_1/RXN_in]
  connect_bd_net -net aurora_gt_wrapper_1_RXP_in [get_bd_pins aurora_64b66b_1/rxp] [get_bd_pins aurora_gt_wrapper_1/RXP_in]
  connect_bd_net -net axis_clock_converter_0_m_axis_tdata [get_bd_pins axis_clock_converter_0/m_axis_tdata] [get_bd_pins firesim_wrapper_0/FROM_QSFP1_DATA]
  connect_bd_net -net axis_clock_converter_0_m_axis_tvalid [get_bd_pins axis_clock_converter_0/m_axis_tvalid] [get_bd_pins firesim_wrapper_0/FROM_QSFP1_VALID]
  connect_bd_net -net axis_clock_converter_1_s_axis_tready [get_bd_pins axis_clock_converter_1/s_axis_tready] [get_bd_pins firesim_wrapper_0/TO_QSFP1_READY]
  connect_bd_net -net axis_clock_converter_2_m_axis_tdata [get_bd_pins axis_clock_converter_2/m_axis_tdata] [get_bd_pins firesim_wrapper_0/FROM_QSFP0_DATA]
  connect_bd_net -net axis_clock_converter_2_m_axis_tvalid [get_bd_pins axis_clock_converter_2/m_axis_tvalid] [get_bd_pins firesim_wrapper_0/FROM_QSFP0_VALID]
  connect_bd_net -net axis_clock_converter_3_s_axis_tready [get_bd_pins axis_clock_converter_3/s_axis_tready] [get_bd_pins firesim_wrapper_0/TO_QSFP0_READY]
  connect_bd_net -net clk_wiz_aurora_0_clk_out1 [get_bd_pins aurora_64b66b_0_driver/INIT_CLK_IN] [get_bd_pins clk_wiz_aurora_0/clk_out1]
  connect_bd_net -net clk_wiz_aurora_0_locked [get_bd_pins aurora_64b66b_0_driver/locked] [get_bd_pins clk_wiz_aurora_0/locked]
  connect_bd_net -net clk_wiz_aurora_1_clk_out1 [get_bd_pins aurora_64b66b_1_driver/INIT_CLK_IN] [get_bd_pins clk_wiz_aurora_1/clk_out1]
  connect_bd_net -net clk_wiz_aurora_1_locked [get_bd_pins aurora_64b66b_1_driver/locked] [get_bd_pins clk_wiz_aurora_1/locked]
  connect_bd_net -net ddr4_0_c0_ddr4_ui_clk [get_bd_pins axi_dwidth_converter_0/m_axi_aclk] [get_bd_pins clk_wiz_0/clk_in1] [get_bd_pins ddr4_0/c0_ddr4_ui_clk] [get_bd_pins proc_sys_reset_1/slowest_sync_clk]
  connect_bd_net -net firesim_wrapper_0_FROM_QSFP0_READY [get_bd_pins axis_clock_converter_2/m_axis_tready] [get_bd_pins firesim_wrapper_0/FROM_QSFP0_READY]
  connect_bd_net -net firesim_wrapper_0_FROM_QSFP1_READY [get_bd_pins axis_clock_converter_0/m_axis_tready] [get_bd_pins firesim_wrapper_0/FROM_QSFP1_READY]
  connect_bd_net -net firesim_wrapper_0_TO_QSFP0_DATA [get_bd_pins axis_clock_converter_3/s_axis_tdata] [get_bd_pins firesim_wrapper_0/TO_QSFP0_DATA]
  connect_bd_net -net firesim_wrapper_0_TO_QSFP0_VALID [get_bd_pins axis_clock_converter_3/s_axis_tvalid] [get_bd_pins firesim_wrapper_0/TO_QSFP0_VALID]
  connect_bd_net -net firesim_wrapper_0_TO_QSFP1_DATA [get_bd_pins axis_clock_converter_1/s_axis_tdata] [get_bd_pins firesim_wrapper_0/TO_QSFP1_DATA]
  connect_bd_net -net firesim_wrapper_0_TO_QSFP1_VALID [get_bd_pins axis_clock_converter_1/s_axis_tvalid] [get_bd_pins firesim_wrapper_0/TO_QSFP1_VALID]
  connect_bd_net -net pcie_perstn_1 [get_bd_ports pcie_perstn] [get_bd_pins xdma_0/sys_rst_n]
  connect_bd_net -net proc_sys_reset_0_interconnect_aresetn [get_bd_pins axi_clock_converter_0/m_axi_aresetn] [get_bd_pins axi_clock_converter_1/m_axi_aresetn] [get_bd_pins axi_dwidth_converter_0/s_axi_aresetn] [get_bd_pins axis_clock_converter_0/m_axis_aresetn] [get_bd_pins axis_clock_converter_1/s_axis_aresetn] [get_bd_pins axis_clock_converter_2/m_axis_aresetn] [get_bd_pins axis_clock_converter_3/s_axis_aresetn] [get_bd_pins firesim_wrapper_0/sys_reset_n] [get_bd_pins proc_sys_reset_0/interconnect_aresetn]
  connect_bd_net -net resetn_1 [get_bd_ports resetn] [get_bd_pins proc_sys_reset_0/ext_reset_in] [get_bd_pins proc_sys_reset_1/ext_reset_in] [get_bd_pins resetn_inv_0/Op1]
  connect_bd_net -net resetn_inv_0_Res [get_bd_pins clk_wiz_0/reset] [get_bd_pins clk_wiz_aurora_0/reset] [get_bd_pins clk_wiz_aurora_1/reset] [get_bd_pins ddr4_0/sys_rst] [get_bd_pins resetn_inv_0/Res]
  connect_bd_net -net rst_ddr4_0_300M_interconnect_aresetn [get_bd_pins axi_dwidth_converter_0/m_axi_aresetn] [get_bd_pins ddr4_0/c0_ddr4_aresetn] [get_bd_pins proc_sys_reset_1/interconnect_aresetn]
  connect_bd_net -net sys_clk_30 [get_bd_pins axi_clock_converter_0/m_axi_aclk] [get_bd_pins axi_clock_converter_1/m_axi_aclk] [get_bd_pins axi_dwidth_converter_0/s_axi_aclk] [get_bd_pins axis_clock_converter_0/m_axis_aclk] [get_bd_pins axis_clock_converter_1/s_axis_aclk] [get_bd_pins axis_clock_converter_2/m_axis_aclk] [get_bd_pins axis_clock_converter_3/s_axis_aclk] [get_bd_pins clk_wiz_0/clk_out1] [get_bd_pins firesim_wrapper_0/sys_clk_30] [get_bd_pins proc_sys_reset_0/slowest_sync_clk]
  connect_bd_net -net util_ds_buf_IBUF_DS_ODIV2 [get_bd_pins util_ds_buf/IBUF_DS_ODIV2] [get_bd_pins xdma_0/sys_clk]
  connect_bd_net -net util_ds_buf_IBUF_OUT [get_bd_pins util_ds_buf/IBUF_OUT] [get_bd_pins xdma_0/sys_clk_gt]
  connect_bd_net -net util_vector_logic_0_Res [get_bd_pins axis_clock_converter_0/s_axis_aresetn] [get_bd_pins axis_clock_converter_1/m_axis_aresetn] [get_bd_pins axis_data_fifo_2/s_axis_aresetn] [get_bd_pins axis_data_fifo_3/s_axis_aresetn] [get_bd_pins util_vector_logic_0/Res]
  connect_bd_net -net util_vector_logic_1_Res [get_bd_pins axis_clock_converter_2/s_axis_aresetn] [get_bd_pins axis_clock_converter_3/m_axis_aresetn] [get_bd_pins axis_data_fifo_0/s_axis_aresetn] [get_bd_pins axis_data_fifo_1/s_axis_aresetn] [get_bd_pins util_vector_logic_1/Res]
  connect_bd_net -net xdma_0_axi_aclk [get_bd_pins axi_clock_converter_0/s_axi_aclk] [get_bd_pins axi_clock_converter_1/s_axi_aclk] [get_bd_pins xdma_0/axi_aclk]
  connect_bd_net -net xdma_0_axi_aresetn [get_bd_pins axi_clock_converter_0/s_axi_aresetn] [get_bd_pins axi_clock_converter_1/s_axi_aresetn] [get_bd_pins xdma_0/axi_aresetn]
  connect_bd_net -net xlconstant_0_dout [get_bd_pins xdma_0/usr_irq_req] [get_bd_pins xlconstant_0/dout]

  # Create address segments


  # Restore current instance
  current_bd_instance $oldCurInst

  validate_bd_design
  save_bd_design
}
# End of create_root_design()


##################################################################
# MAIN FLOW
##################################################################

create_root_design "" $desired_host_frequency
