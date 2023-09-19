/*
 HA design
*/
module HA
(
  input logic   A,
  input logic   B,
  output logic  S,
  output logic  Co
);

  assign S = A ^ B;
  assign Co = A & B;

endmodule
