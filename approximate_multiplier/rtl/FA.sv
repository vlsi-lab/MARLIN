/*
  FA design
*/
module FA 
(
  input logic   A,
  input logic   B,
  input logic   Cin, 
  output logic  S,
  output logic  Co
);

  assign S = A ^ B ^ Cin;
  assign Co = (Cin & A) | (Cin & B) |(A & B);

endmodule 
