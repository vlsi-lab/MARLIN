//////////////////////////////////////////////////////////////
// Approximate ALU supporting ADD, SUB, ADDI, MUL operations
//////////////////////////////////////////////////////////////

import riscv_defines::*;

module riscv_alu_approx
#(
  parameter MUL_BIT   = 9
)(
  input  logic                     clk,
  input  logic                     rst_n,
  input  logic                     enable_i,
  input  logic [APP_OP_WIDTH-1:0]  operator_i,
  input  logic [N_BIT_APPR-1:0]    approx_mask_i,
  input  logic [N_BIT_PREC-1:0]    precision_mask_i,
  input  logic [31:0]              operand_a_i,
  input  logic [31:0]              operand_b_i,
  input  logic [31:0]              operand_c_i,  
  input  logic [ 1:0]              short_signed_i,
  input  logic [ 4:0]              imm_i,
  input  logic [ 1:0]              dot_signed_i,
  output logic [31:0]              result_o,
  output logic                     ready_o 
);

  //mul signals
  logic [3:0][MUL_BIT-1:0]          mul_op_a;
  logic [3:0][MUL_BIT-1:0]          mul_op_b;
  logic [3:0][2*MUL_BIT-1:0]        result_mul_tmp;
  logic [31:0]                      result_mac;
  logic [31:0]                      result_mul;
  logic [31:0]                      short_result; //CHECK: non capisco perchè servono 33 bit, forse servono per mulh
  logic [31:0]                      dot_result; 
  
  assign mul_op_a[0] = (operator_i != APP_DOT8) ? operand_a_i[MUL_BIT-1:0]: {dot_signed_i[1] & operand_a_i[7] ,operand_a_i[7:0]};  // CHECK, visto che MULS accetta 16 bit, noi supponiamo che il software passi 8 bit sign extended su 16, quindi non ho bisogno di generare il bit 9 come sign extension, è già lì
  assign mul_op_b[0] = (operator_i != APP_DOT8) ? operand_b_i[MUL_BIT-1:0]: {dot_signed_i[0] & operand_b_i[7] ,operand_b_i[7:0]};  // CHECK, visto che MULS accetta 16 bit, noi supponiamo che il software passi 8 bit sign extended su 16, quindi non ho bisogno di generare il bit 9 come sign extension, è già lì
//b_i è il peso, a_i attivazione as dot_signed_i[0]=1 dot_signed_i[1]=0
 assign mul_op_a[1] = (operator_i != APP_DOT8) ? '0 : {dot_signed_i[1] & operand_a_i[15], operand_a_i[15:8]};
 assign mul_op_b[1] = (operator_i != APP_DOT8) ? '0 : {dot_signed_i[0] & operand_b_i[15], operand_b_i[15:8]};
 
 assign mul_op_a[2] = (operator_i != APP_DOT8) ? '0 : {dot_signed_i[1] & operand_a_i[23], operand_a_i[23:16]};
 assign mul_op_b[2] = (operator_i != APP_DOT8) ? '0 : {dot_signed_i[0] & operand_b_i[23], operand_b_i[23:16]};
 
 assign mul_op_a[3] = (operator_i != APP_DOT8) ? '0 : {dot_signed_i[1] & operand_a_i[31], operand_a_i[31:24]};
 assign mul_op_b[3] = (operator_i != APP_DOT8) ? '0 : {dot_signed_i[0] & operand_b_i[31], operand_b_i[31:24]};

 
  
  /////////////////////////////
  //      Multipliers        //
  /////////////////////////////
  
  mul_9x9_signed_bw 
    #(
      .MAC_IN_WIDTH     (MUL_BIT            ),
      .N_BIT_RES        (N_BIT_PREC         ),
      .N_BIT_APPR       (N_BIT_APPR         )
    )
    mult_0
    ( 
      .a                (mul_op_a[0]        ),
      .b                (mul_op_b[0]        ),
      .res_mask         (precision_mask_i   ),
      .appr_mask        (approx_mask_i      ),
      .res              (result_mul_tmp[0]  )
    );
  
  mul_9x9_signed_bw 
    #(
      .MAC_IN_WIDTH     (MUL_BIT            ),
      .N_BIT_RES        (N_BIT_PREC         ),
      .N_BIT_APPR       (N_BIT_APPR         )
    )
    mult_1
    ( 
      .a                (mul_op_a[1]        ),
      .b                (mul_op_b[1]        ),
      .res_mask         (precision_mask_i   ),
      .appr_mask        (approx_mask_i      ),
      .res              (result_mul_tmp[1]  )
    );
    
  mul_9x9_signed_bw 
    #(
      .MAC_IN_WIDTH     (MUL_BIT            ),
      .N_BIT_RES        (N_BIT_PREC         ),
      .N_BIT_APPR       (N_BIT_APPR         )
    )
    mult_2
    ( 
      .a                (mul_op_a[2]        ),
      .b                (mul_op_b[2]        ),
      .res_mask         (precision_mask_i   ),
      .appr_mask        (approx_mask_i      ),
      .res              (result_mul_tmp[2]  )
    );
    
  mul_9x9_signed_bw 
    #(
      .MAC_IN_WIDTH     (MUL_BIT            ),
      .N_BIT_RES        (N_BIT_PREC         ),
      .N_BIT_APPR       (N_BIT_APPR         )
    )
    mult_3
    ( 
      .a                (mul_op_a[3]        ),
      .b                (mul_op_b[3]        ),
      .res_mask         (precision_mask_i   ),
      .appr_mask        (approx_mask_i      ),
      .res              (result_mul_tmp[3]  )
    );
    
  
  assign result_mul     = {{(32-2*MUL_BIT){result_mul_tmp[0][2*MUL_BIT-1]}}, result_mul_tmp[0]}; //sign extend the result
   
  /////////////////////////////
  //     Short-mac, mac      //
  /////////////////////////////
  assign result_mac       = $signed(operand_c_i) + $signed(result_mul);

  //assign short_result     = $signed({short_signed_i & result_mac[31], short_signed_i & result_mac[31], short_mac[31:0]}) >>> imm_i;
  assign short_result     = $signed({result_mac[31:0]}) >>> imm_i; //CHECK: use 31 instead of 33 bits
  /////////////////////////////
  //      dot products       //
  /////////////////////////////
  assign dot_result  = $signed(result_mul_tmp[0]) + $signed(result_mul_tmp[1]) + $signed(result_mul_tmp[2]) + $signed(result_mul_tmp[3]) + $signed(operand_c_i);  

  

  assign ready_o = 1'b1;

  // Assign the output signal
  always_comb
  begin
    result_o    ='0;
    unique case (operator_i)
      APP_MULMAC:   result_o = result_mac;
      APP_MULSMACS: result_o = short_result;
      APP_DOT8:     result_o = dot_result;
      default:      result_o = result_mac;
     endcase
  end

endmodule
