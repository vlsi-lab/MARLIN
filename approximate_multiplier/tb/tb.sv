module tb
#(
  parameter MAC_IN_WIDTH = 9,
  parameter EFF_MAC_IN_WIDTH_A = 5, // <= 9
  parameter EFF_MAC_IN_WIDTH_B = 9, // <= 9
  parameter N_BIT_APPR = 8,
  parameter N_BIT_RES = 14,
  localparam MAC_OUT_WIDTH = MAC_IN_WIDTH*2,
  localparam EFF_MAC_OUT_WIDTH = EFF_MAC_IN_WIDTH_A + EFF_MAC_IN_WIDTH_B
);
  
  logic [MAC_IN_WIDTH-1:0]        a_i;
  logic [MAC_IN_WIDTH-1:0]        b_i;
  logic [N_BIT_RES-1:0]           res_mask_i;
  logic [N_BIT_APPR-1:0]          appr_mask_i;
  logic [MAC_OUT_WIDTH-1:0]       res_i;
  logic [MAC_OUT_WIDTH-1:0]       correct_res;
  logic [EFF_MAC_IN_WIDTH_A-1:0]  a_eff;
  logic [EFF_MAC_IN_WIDTH_B-1:0]  b_eff;

  // mask generation, should be handled by other stages
  if (EFF_MAC_OUT_WIDTH >4)
    assign res_mask_i[EFF_MAC_OUT_WIDTH-1-4:0] = '1;
  if (EFF_MAC_OUT_WIDTH < MAC_OUT_WIDTH)
    assign res_mask_i[N_BIT_RES-1:EFF_MAC_OUT_WIDTH-1-3] = '0;
  // correct sign-extension : should be handlesd by sw
  assign a_i = $signed(a_eff);
  assign b_i = $signed(b_eff);

  //////////////////
  // MUL instanxe //
  //////////////////

  mul_9x9_signed_bw
  dut
  (
    .a          ( a_i         ),
    .b          ( b_i         ),
    .res_mask   ( res_mask_i  ),
    .appr_mask  ( appr_mask_i ),
    .res        ( res_i       )
  );
  // correct result
  assign correct_res = $signed(a_eff)*$signed(b_eff);

  initial begin
    a_eff = '0;   //initialization not to have 'X'
    b_eff = '0;
    appr_mask_i = '1;
    $display("%d,%d", EFF_MAC_IN_WIDTH_A, EFF_MAC_IN_WIDTH_B);
    repeat (50) begin
      repeat (100) begin  
        #2 
        std::randomize (a_eff,b_eff);  // generate 100 random stimuli every 10 units
        // debug for exact multiplier configuration
        if (correct_res != res_i && appr_mask_i =='1)
          $display("Error in result @%t: res= %d, correct_res= %d", $time, res_i, correct_res);  
      end
      std::randomize(appr_mask_i);   // randomly change the approx level
    end   
    //$finish;
  end
endmodule
