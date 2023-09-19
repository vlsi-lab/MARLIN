/*
  9x9 fully-combinational signed multiplier.
  Based on Baugh-Wooley algorithm and Dadda reduction tree, with reconfigurable result precision and accuracy.
*/


module mul_9x9_signed_bw
#(
  parameter MAC_IN_WIDTH = 9,                               // input bit-width  
  parameter MAC_OUT_WIDTH = MAC_IN_WIDTH*2,                 // output bit-width
  parameter N_LEVELS = 5,                                   // levels of the Dadda-tree
  parameter N_PP = MAC_IN_WIDTH,                            // number of partial products
  parameter N_BIT_APPR = 8,                                 // appr_mask bit-width
  parameter N_BIT_RES = 14,                                 // res_mask bit-width , accept signed 2x2 as minimum input bit-width
  parameter int n_rows [N_LEVELS-1:0] = {2,3,4,6,9},        // n_rows for each level of the reduction tree 
  parameter int col_start [N_LEVELS-2:0] = {2,3,4,6},       // column idx pointers to build the Dadda-tree structure
  parameter int col_end [N_LEVELS-2:0] = {15,14,13,11}      // column idx pointers to build the Dadda-tree structure
)(
  input logic [MAC_IN_WIDTH-1 : 0]          a,
  input logic [MAC_IN_WIDTH-1 : 0]          b,
  input logic [N_BIT_RES-1 : 0]             res_mask,       // mask to select result precision
  input logic [N_BIT_APPR-1 : 0]            appr_mask,      // mask to select result approximation
  output logic [MAC_OUT_WIDTH-1 : 0]        res

);

  logic [N_LEVELS-1:0][N_PP-1:0][MAC_OUT_WIDTH-1:0] q;      // matrix for each level, with maximum size as level 0
  logic [N_PP-1:0][MAC_IN_WIDTH-1:0] p;                     // aligned matrix of pp
  logic [N_PP-1:0][MAC_OUT_WIDTH-1:0] p_shifted;            // pp matrix correctly shifted

  ///////////////////////////////
  // Partial-products creation //
  ///////////////////////////////

  for (genvar i = 0; i < MAC_IN_WIDTH; i++)
  begin : row_creation
    if (i < MAC_IN_WIDTH -1)
    begin : first_rows
      for (genvar j = 0; j < MAC_IN_WIDTH; j++)
      begin : column_creation
        if (j < MAC_IN_WIDTH -1)
          assign p[i][j] = a[j] & b[i];
        else
          assign p[i][j] = ~(a[j] & b[i]);
      end
    end
    else
    begin : last_row
      for (genvar j = 0; j < MAC_IN_WIDTH; j++)
      begin : column_creation
        if (j < MAC_IN_WIDTH -1)
          assign p[i][j] = ~(a[j] & b[i]);
        else
          assign p[i][j] = (a[j] & b[i]);
      end
    end
  end

  ///////////////////
  //  Data-gating  //
  // Approximation //
  ///////////////////

  for (genvar i = 0; i < n_rows[0]; i++)
  begin : shift_left_rows
    for (genvar j = 0; j < MAC_IN_WIDTH; j++)
    begin : shift_left_el
      if (((j+i) >= (MAC_OUT_WIDTH - N_BIT_RES)) && ((j+i) < N_BIT_APPR) && i!=0)         // j+i >= 4 && j+i < 8
        assign p_shifted[i][j+i] = p[i][j] & res_mask[j+i-4] & appr_mask[j+i];
      else if (((j+i) >= (MAC_OUT_WIDTH - N_BIT_RES)) && ((j+i) < N_BIT_APPR) && i==0)    // j+i >= 4 && j+i < 8
        assign p_shifted[i][j+i] = ((~(appr_mask[j+i]) | p[i][j]) & res_mask[j+i-4]);
      else if (j+i < MAC_OUT_WIDTH - N_BIT_RES && ((j!=3 && j!=2) || i!=0))
        assign p_shifted[i][j+i] = p[i][j] & appr_mask[j+i]; 
      else if (j+i < MAC_OUT_WIDTH - N_BIT_RES && (j==3 || j==2) && i==0)
        assign p_shifted[i][j+i] = (~(appr_mask[j+i])|p[i][j]); 
      else if (j+i >= N_BIT_APPR)                                                          //j+i >= 8
      begin : apply_res_mask
        assign p_shifted[i][j+i] = p[i][j] & res_mask[j+i-4];
      end
    end
  end
  assign p_shifted[0][MAC_IN_WIDTH] = 1 & res_mask[5];                      // modified b-w extension
  assign p_shifted [N_PP-1][MAC_OUT_WIDTH-1] = 1 & res_mask[N_BIT_RES-1];   // modified b-w extension

  //////////////////////////////
  // Conic-shape level 0 tree //
  //////////////////////////////

  for (genvar i = 0; i < n_rows[0]; i++)
  begin : rows_level_0
    for (genvar j = 0; j < MAC_OUT_WIDTH; j++)
    begin : columns_level_0
      if (j <= MAC_IN_WIDTH)
      begin : copy_unchanged
        assign q[0][i][j] = p_shifted[i][j];
      end
      else if ( j < MAC_OUT_WIDTH -1)
      begin : raise_columns
        if (i+j-8 <= 8 )   // leave blanck spaces with no dots
          assign q[0][i][j] = p_shifted[i+j-8][j];
      end
      else // last column
      begin : raise_last_column
        if (i==0)
          assign q[0][i][j] = p_shifted[8][j];  // only one element
      end
    end
  end

  //////////////////////////
  // Dadda-tree structure //
  //////////////////////////

  for (genvar level = 0; level < N_LEVELS-1; level++) 
  begin
    for (genvar i = 0; i < n_rows[level+1]; i++)  
    begin : rows_iteration_right_side
      // right-side dots remaining in same position
      for (genvar j = 0; j < col_start[level]; j++)
      begin : dots_rside
        if (j >= i)   //stop i idx at correct row, copy only valid values
          assign q[level+1][i][j] = q[level][i][j];
      end
    end
    
    // left-side dots remaining in same position
    for (genvar j = col_end[level]+1; j < MAC_OUT_WIDTH; j++)
    begin : dots_lside
      // select only valid positions
      for (genvar i = 0; i < MAC_OUT_WIDTH-1-j ; i++)
      begin : rows_iteration_left_side
        if (j != col_end[level]+1)
          assign q[level+1][i][j] = q[level][i][j];
        else
          assign q[level+1][i+1][j] = q[level][i][j]; //leave space for Cout
      end
    end
    // last column copy single dot
    assign q[level+1][0][MAC_OUT_WIDTH-1] = q[level][0][MAC_OUT_WIDTH-1];

    // Instation of right part HAs
    for (genvar i = 0; i < n_rows[level]; i++)
    begin : rows_selection_ha_fa
      if(i%3 == 0 && i+1 < n_rows[level])
      begin : ha_right  
        HA ha_li_right (
                          .A      (q[level][i][col_start[level]+(i/3)]), 
                          .B      (q[level][i+1][col_start[level]+(i/3)]),
                          .S      (q[level+1][i-i/3][col_start[level]+(i/3)]),
                          .Co     (q[level+1][i+1-i/3][col_start[level]+(i/3)+1]) 
        ); 
      end

      // Instantiation of central part FAs
      if(i%3 == 0 && i+2< n_rows[level])
      begin : fa_central_rows
        for (genvar j = col_start[level]; j<= col_end[level];j++)
        begin : fa_central_cols
          if ( j >= col_start[level]+(i/3)+1 && j <= col_end[level]-(i/3) ) 
          begin 
            if (j == col_end[level]-(i/3))
            begin : fa_left_side // save carry in same line
              FA fa_li_left (
                                .A      (q[level][i][j]),
                                .B      (q[level][i+1][j]),
                                .Cin    (q[level][i+2][j]),
                                .S      (q[level+1][i-i/3][j]),
                                .Co     (q[level+1][i-i/3][j+1])
                    
              );
            end
            else   
            begin : all_fas_central
              FA fa_li_right (
                                .A      (q[level][i][j]),
                                .B      (q[level][i+1][j]),
                                .Cin    (q[level][i+2][j]),
                                .S      (q[level+1][i-i/3][j]),
                                .Co     (q[level+1][i+1-i/3][j+1])
                    
              );
            end
          end
        end
      end
      // save central dots
      if (level!=3) 
      begin
        for (genvar j=col_start[level]; j< col_start[level]+3-level;j++)
        begin : central_dots_right
          if (i >= 3*(j-col_start[level])+2 && i < j-col_start[level]+ n_rows[level]-2+level)
            assign q[level+1][i-(j-col_start[level]+1)][j] = q[level][i][j]; 
        end
      end
      
      if (level == 0 || level == 1)
      begin : central_dots_left
        for (genvar j = col_end[level]-1+level; j<col_end[level]+1; j++) 
        begin : central_dots_left_cols
          if (i >= 3 + 3*(col_end[level]-j) && i < col_end[level]-j+n_rows[level]-3+level )
            assign q[level+1][i-(col_end[level]-j)][j] = q[level][i][j];  
        end
      end 
    end   
    // exception at level 3 
    if (level == 3)
      assign q[level+1][1][col_start[level]] = q[level][2][col_start[level]];
    
    // copy central dots if present
    if (level == 2) 
    begin : level2_central_dots
      for(genvar j = col_start[level]+1; j<col_end[level]+1;j++)
      begin : central_dots_l2l3
        assign q[level+1][2][j] = q[level][3][j];
      end
    end 
  end // close for levels   
       
      
  // assign result taking into account truncation of precision
  
  logic [MAC_OUT_WIDTH-1:0] res_tmp_1;
  logic [MAC_OUT_WIDTH-1:0] res_tmp_2;
  logic sign_res;
  
  assign res_tmp_2[0] = q[N_LEVELS-1][0][0];
  assign q[N_LEVELS-1][1][17] = 0;
  assign res_tmp_1[MAC_OUT_WIDTH-1:1] = q[N_LEVELS-1][0][MAC_OUT_WIDTH-1:1] + q[N_LEVELS-1][1][MAC_OUT_WIDTH-1:1];
  
  // always valid part of res
  assign res_tmp_2 [MAC_OUT_WIDTH-N_BIT_RES-1:1] = res_tmp_1 [MAC_OUT_WIDTH-N_BIT_RES-1:1];
  // actual sign evaluation 
  assign sign_res = (a[MAC_IN_WIDTH-1] ^ b[MAC_IN_WIDTH-1]);
  // correct sign extension for reduced precision configuration
  assign res_tmp_2 [MAC_OUT_WIDTH-1 : MAC_OUT_WIDTH-N_BIT_RES] = (sign_res == 0) ? (res_mask & res_tmp_1[MAC_OUT_WIDTH-1 : MAC_OUT_WIDTH-N_BIT_RES]) :
                                                            (~(res_mask) | res_tmp_1[MAC_OUT_WIDTH-1 : MAC_OUT_WIDTH-N_BIT_RES] );
  // assign final result to output
  assign res = (a!='0 && b!='0) ? res_tmp_2 : '0; // always correct  0 mult

endmodule
