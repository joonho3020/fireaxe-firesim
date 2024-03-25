`timescale 1ns/1ps

module toy_wrapper(
    TO_QSFP_DATA,
    TO_QSFP_VALID,
    TO_QSFP_READY,
    FROM_QSFP_DATA,
    FROM_QSFP_VALID,
    FROM_QSFP_READY,
    
    sys_clk_30,
    sys_reset_n
);

output [255:0] TO_QSFP_DATA;
output TO_QSFP_VALID;
input TO_QSFP_READY;

input [255:0] FROM_QSFP_DATA;
input FROM_QSFP_VALID;
output FROM_QSFP_READY;

input sys_clk_30;
input sys_reset_n;

simple_frame_gen frame_gen_i
(
// AXI4-S DATA output signals
    .tx_tvalid(TO_QSFP_VALID),
    .tx_tdata(TO_QSFP_DATA),
    .tx_tready(TO_QSFP_READY),
    .clk(sys_clk_30),
    .reset(sys_reset_n)
);

    (* KEEP = "TRUE" , mark_debug = "TRUE"*) wire data_err_count_o;
simple_frame_check frame_check_i
    (
        // AXI4-S input signals
    .rx_tvalid(FROM_QSFP_VALID),
    .rx_tready(FROM_QSFP_READY),
    .rx_tdata(FROM_QSFP_DATA),
    .error_count(data_err_count_o),

    .clk(sys_clk_30),
    .reset(sys_reset_n)
);


endmodule