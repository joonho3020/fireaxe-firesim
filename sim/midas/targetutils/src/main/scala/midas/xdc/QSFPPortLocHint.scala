// See LICENSE for license details.

package midas.targetutils.xdc

import chisel3.experimental.ChiselAnnotation
import firrtl.annotations.ReferenceTarget

/**
  * Some rough guidance, based on Ultrascale+, is provided in the scala doc for
  * each hint. Consult the Xilinx UGs for your target architecture and the
  * synthesis UG (UG901).
  */


object QSFPPortLocHint {
  // _reg suffix is applied to memory cells by Vivado, the glob manages
  // duplication for multibit memories.
  private [midas] def portLoc: String =
    """set_property -dict {PACKAGE_PIN AW19 IOSTANDARD LVDS} [get_ports default_300mhz_clk1_clk_n]
set_property -dict {PACKAGE_PIN AW20 IOSTANDARD LVDS} [get_ports default_300mhz_clk1_clk_p]

set_property -dict {PACKAGE_PIN E32  IOSTANDARD DIFF_POD12_DCI } [get_ports default_300mhz_clk2_clk_n]; 
set_property -dict {PACKAGE_PIN F32  IOSTANDARD DIFF_POD12_DCI } [get_ports default_300mhz_clk2_clk_p]; 

set_property PACKAGE_PIN M10 [get_ports qsfp0_156mhz_clk_n]
set_property PACKAGE_PIN M11 [get_ports qsfp0_156mhz_clk_p]

set_property PACKAGE_PIN T10 [get_ports qsfp1_156mhz_clk_n];
set_property PACKAGE_PIN T11 [get_ports qsfp1_156mhz_clk_p];

set_property PACKAGE_PIN N3  [get_ports qsfp0_RX_N0];
set_property PACKAGE_PIN N4  [get_ports qsfp0_RX_P0];
set_property PACKAGE_PIN M1  [get_ports qsfp0_RX_N1];
set_property PACKAGE_PIN M2  [get_ports qsfp0_RX_P1];
set_property PACKAGE_PIN L3  [get_ports qsfp0_RX_N2];
set_property PACKAGE_PIN L4  [get_ports qsfp0_RX_P2];
set_property PACKAGE_PIN K1  [get_ports qsfp0_RX_N3];
set_property PACKAGE_PIN K2  [get_ports qsfp0_RX_P3];
set_property PACKAGE_PIN N8  [get_ports qsfp0_TX_N0];
set_property PACKAGE_PIN N9  [get_ports qsfp0_TX_P0];
set_property PACKAGE_PIN M6  [get_ports qsfp0_TX_N1];
set_property PACKAGE_PIN M7  [get_ports qsfp0_TX_P1];
set_property PACKAGE_PIN L8  [get_ports qsfp0_TX_N2];
set_property PACKAGE_PIN L9  [get_ports qsfp0_TX_P2];
set_property PACKAGE_PIN K6  [get_ports qsfp0_TX_N3];
set_property PACKAGE_PIN K7  [get_ports qsfp0_TX_P3];

set_property PACKAGE_PIN U3  [get_ports qsfp1_RX_N0];
set_property PACKAGE_PIN U4  [get_ports qsfp1_RX_P0];
set_property PACKAGE_PIN T1  [get_ports qsfp1_RX_N1];
set_property PACKAGE_PIN T2  [get_ports qsfp1_RX_P1];
set_property PACKAGE_PIN R3  [get_ports qsfp1_RX_N2];
set_property PACKAGE_PIN R4  [get_ports qsfp1_RX_P2];
set_property PACKAGE_PIN P1  [get_ports qsfp1_RX_N3];
set_property PACKAGE_PIN P2  [get_ports qsfp1_RX_P3];
set_property PACKAGE_PIN U8  [get_ports qsfp1_TX_N0];
set_property PACKAGE_PIN U9  [get_ports qsfp1_TX_P0];
set_property PACKAGE_PIN T6  [get_ports qsfp1_TX_N1];
set_property PACKAGE_PIN T7  [get_ports qsfp1_TX_P1];
set_property PACKAGE_PIN R8  [get_ports qsfp1_TX_N2];
set_property PACKAGE_PIN R9  [get_ports qsfp1_TX_P2];
set_property PACKAGE_PIN P6  [get_ports qsfp1_TX_N3];
set_property PACKAGE_PIN P7  [get_ports qsfp1_TX_P3];
"""

  private def annotate(): Unit = {
    chisel3.experimental.annotate(new ChiselAnnotation {
      def toFirrtl = XDCAnnotation(
        XDCFiles.Implementation,
        portLoc)
    })
  }

  /**
    * Annotates a chisel3 Mem indicating it should be implemented with a particular
    * Xilinx RAM structure.
    */
  def apply(): Unit = {
    annotate()
  }

  // /**
  //   * Annotates a FIRRTL ReferenceTarget indicating it should be implemented with a particular
  //   * Xilinx RAM structure.
  //   * 
  //   * Note: the onus is on the user to ensure the RT points at a mem-like structure. In general, 
  //   * one should prefer using the apply method that accepts a chisel3.MemBase[_] to get compile-time errors.
  //   */
  // def apply(): Unit = {
  //   annotate()
  // }
}
