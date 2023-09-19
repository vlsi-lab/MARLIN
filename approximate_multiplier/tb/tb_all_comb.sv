module tb_all_comb 
#(
  parameter MAC_IN_WIDTH = 9,
  parameter EFF_MAC_IN_WIDTH_A =9, // <= 9
  parameter EFF_MAC_IN_WIDTH_B =9, // <= 9
  parameter N_BIT_APPR = 8,
  parameter N_BIT_RES = 14,
  parameter file_name = "./results/results_8x8_approx.txt",
  localparam MAC_OUT_WIDTH = MAC_IN_WIDTH*2,
  localparam EFF_MAC_OUT_WIDTH = EFF_MAC_IN_WIDTH_A + EFF_MAC_IN_WIDTH_B
);
  
  logic [MAC_IN_WIDTH-1:0] a_i;
  logic [MAC_IN_WIDTH-1:0] b_i;
  logic [N_BIT_RES-1:0] res_mask_i;
  logic [N_BIT_APPR-1:0] appr_mask_i;
  logic [MAC_OUT_WIDTH-1:0] res_i;
    
  logic [MAC_OUT_WIDTH-1:0] correct_res;

  logic [EFF_MAC_IN_WIDTH_A-1:0] a_eff;
  logic [EFF_MAC_IN_WIDTH_B-1:0] b_eff;

  // mask generation, should be handled by other stages
  if (EFF_MAC_OUT_WIDTH >4)
    assign res_mask_i[EFF_MAC_OUT_WIDTH-1-4:0] = '1;
  if (EFF_MAC_OUT_WIDTH < MAC_OUT_WIDTH)
    assign res_mask_i[N_BIT_RES-1:EFF_MAC_OUT_WIDTH-1-3] = '0;
  // correct sign-extension : should be handlesd by sw
  assign a_i = $signed(a_eff);
  assign b_i = $signed(b_eff);

  //////////////////
  // MUL instance //
  //////////////////

  mul_9x9_signed_bw
  #( 
    .N_BIT_APPR ( N_BIT_APPR  )
    )
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

  int lower_i = (-1)* (2**(EFF_MAC_IN_WIDTH_A-1)); // lowest value for a 
  int lower_j = (-1)* (2**(EFF_MAC_IN_WIDTH_B-1)); // lowest value for b
  initial begin
    int fd;
    fd = $fopen (file_name,"w");
    if (fd) 
      $display("File successfully opened");
    else
      $error ("Cannot open file");
    
    for (int appr = 0; appr < 2**N_BIT_APPR; appr++) begin
      appr_mask_i = ~appr;
      $fdisplay (fd, "APPROX = %d", appr);
      for (int i = lower_i; i < 2**(EFF_MAC_IN_WIDTH_A-1); i++) begin
        for (int j = lower_j ; j < 2**(EFF_MAC_IN_WIDTH_B-1); j++) begin
          a_eff = i;
          b_eff = j;
          #10
          // save to file the result
          $fdisplay (fd, "%d x %d = %d",$signed(a_i),$signed(b_i),$signed(res_i));
        end 
      end
    end
    $fclose(fd);
  end
endmodule
