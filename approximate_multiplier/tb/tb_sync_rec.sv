module tb 
#(
  parameter N_SAMPLES = 100000,
  parameter MAC_IN_WIDTH = 9,
  parameter EFF_MAC_IN_WIDTH_A = 4, // <= 9
  parameter EFF_MAC_IN_WIDTH_B = 4, // <= 9
  parameter N_BIT_APPR = 8,
  parameter N_BIT_RES =14,
  localparam MAC_OUT_WIDTH = MAC_IN_WIDTH*2,
  localparam EFF_MAC_OUT_WIDTH = EFF_MAC_IN_WIDTH_A + EFF_MAC_IN_WIDTH_B
);

  logic clk_i;
  logic rst_i;
  // const time period = 10ns;
  const time period = 2ns;

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
  assign a_i = $signed(a_eff[EFF_MAC_IN_WIDTH_A-1:0]);
  assign b_i = $signed(b_eff[EFF_MAC_IN_WIDTH_B-1:0]);


  // set approx level externally
  logic [N_BIT_APPR-1:0] appr_mask_i_n;

  //////////////////
  // DUT instance //
  //////////////////

  appr_wrapper
  dut
  (
    .clk        ( clk_i       ),
    .rst        ( rst_i       ),
    .a          ( a_i         ),
    .b          ( b_i         ),
    .res_mask   ( res_mask_i  ),
    .appr_mask  ( appr_mask_i ),
    .res        ( res_i       )
  );

  ////////////////////
  // CLK generation //
  ////////////////////
  always
  begin
    clk_i = 1'b0;
    #(period/2);
    clk_i = 1'b1;
    #(period/2);
  end

  assign correct_res = $signed(a_eff)*$signed(b_eff);
  
  initial begin
    // read external flag
    if($value$plusargs ("APPROX_LEVEL=%0d", appr_mask_i_n)) begin
      appr_mask_i = ~(appr_mask_i_n);
      $display ("Read approx_level=%d",appr_mask_i_n);
    end
    else
      appr_mask_i = '1;
    
    
    a_eff = '0;     // initialization not to have 'X'
    b_eff = '0;
    rst_i = 1'b1;   // reset circuit
    #9.5
    rst_i = 1'b0;
    $urandom(125);  // assign a fixed seed

    repeat (N_SAMPLES) begin  
      #(period) 
      a_eff = $urandom(); //_range ();  
      b_eff = $urandom(); //_range (); 
    end
    $finish;
  end
endmodule
