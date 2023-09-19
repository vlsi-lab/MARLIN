/*
 Wrapper providing synchronous inputs to the reconfigurable multiplier and reading its output synchronously.
*/



module appr_wrapper 
#(
  localparam MAC_IN_WIDTH = 9,                  // input bit-width
  localparam MAC_OUT_WIDTH = MAC_IN_WIDTH*2,    // output bit-width
  parameter N_BIT_RES = 14,                     // res_mask bit-width
  parameter N_BIT_APPR = 8                      // appr_mask bit-width
)(
  input logic                       clk,  // sync
  input logic                       rst, 
  input logic [MAC_IN_WIDTH-1:0]    a,    
  input logic [MAC_IN_WIDTH-1:0]    b,
  input logic [N_BIT_RES-1:0]       res_mask,   // mask for the precision selection
  input logic [N_BIT_APPR-1:0]      appr_mask,  // mask for the approximation selection
  output logic [MAC_OUT_WIDTH-1:0]  res
);

  
  logic [MAC_IN_WIDTH-1:0]  a_tmp;
  logic [MAC_IN_WIDTH-1:0]  b_tmp;
  logic [N_BIT_RES-1:0]     res_mask_tmp;
  logic [N_BIT_APPR-1:0]    appr_mask_tmp;
  logic [MAC_OUT_WIDTH-1:0] res_tmp;
  
  
  // multiplier instance  
  mul_9x9_signed_bw mul_9x9_appr_i 
  (
    .a                  ( a_tmp           ),
    .b                  ( b_tmp           ),
    .res_mask           ( res_mask_tmp    ),
    .appr_mask          ( appr_mask_tmp   ),
    .res                ( res_tmp         )    
  
  );
  // synchronize inputs and outputs
  always_ff @ (posedge clk) begin : in_out_ff
    if (rst) begin  //sync reset
       a_tmp = '0;
       b_tmp = '0;
       res_mask_tmp = '0;
       appr_mask_tmp = '0;
       res = '0;
    end
    else begin
       a_tmp = a;
       b_tmp = b;
       res_mask_tmp = res_mask;
       appr_mask_tmp = appr_mask;
       res = res_tmp;
    end
  end

endmodule
