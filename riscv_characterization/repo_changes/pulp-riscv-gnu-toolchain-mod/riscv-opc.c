/* RISC-V opcode list
   Copyright (C) 2011-2017 Free Software Foundation, Inc.

   Contributed by Andrew Waterman (andrew@sifive.com).
   Based on MIPS target.

   PULP family support contributed by Eric Flamand (eflamand@iis.ee.ethz.ch) at ETH-Zurich
   and Greenwaves Technologies (eric.flamand@greenwaves-technologies.com)

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

#include "sysdep.h"
#include "opcode/riscv.h"
#include <stdio.h>

/* Register names used by gas and objdump.  */

const char * const riscv_gpr_names_numeric[NGPR] =
{
  "x0",   "x1",   "x2",   "x3",   "x4",   "x5",   "x6",   "x7",
  "x8",   "x9",   "x10",  "x11",  "x12",  "x13",  "x14",  "x15",
  "x16",  "x17",  "x18",  "x19",  "x20",  "x21",  "x22",  "x23",
  "x24",  "x25",  "x26",  "x27",  "x28",  "x29",  "x30",  "x31"
};

const char * const riscv_gpr_names_abi[NGPR] = {
  "zero", "ra", "sp",  "gp",  "tp", "t0",  "t1",  "t2",
  "s0",   "s1", "a0",  "a1",  "a2", "a3",  "a4",  "a5",
  "a6",   "a7", "s2",  "s3",  "s4", "s5",  "s6",  "s7",
  "s8",   "s9", "s10", "s11", "t3", "t4",  "t5",  "t6"
};

const char * const riscv_fpr_names_numeric[NFPR] =
{
  "f0",   "f1",   "f2",   "f3",   "f4",   "f5",   "f6",   "f7",
  "f8",   "f9",   "f10",  "f11",  "f12",  "f13",  "f14",  "f15",
  "f16",  "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23",
  "f24",  "f25",  "f26",  "f27",  "f28",  "f29",  "f30",  "f31"
};

const char * const riscv_fpr_names_abi[NFPR] = {
  "ft0", "ft1", "ft2",  "ft3",  "ft4", "ft5", "ft6",  "ft7",
  "fs0", "fs1", "fa0",  "fa1",  "fa2", "fa3", "fa4",  "fa5",
  "fa6", "fa7", "fs2",  "fs3",  "fs4", "fs5", "fs6",  "fs7",
  "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11"
};

/* The order of overloaded instructions matters.  Label arguments and
   register arguments look the same. Instructions that can have either
   for arguments must apear in the correct order in this table for the
   assembler to pick the right one. In other words, entries with
   immediate operands must apear after the same instruction with
   registers.

   Because of the lookup algorithm used, entries with the same opcode
   name must be contiguous.  */

#define MASK_RS1 (OP_MASK_RS1 << OP_SH_RS1)
#define MASK_RS2 (OP_MASK_RS2 << OP_SH_RS2)
#define MASK_RD (OP_MASK_RD << OP_SH_RD)
#define MASK_CRS2 (OP_MASK_CRS2 << OP_SH_CRS2)
#define MASK_IMM ENCODE_ITYPE_IMM (-1U)
#define MASK_RVC_IMM ENCODE_RVC_IMM (-1U)
#define MASK_UIMM ENCODE_UTYPE_IMM (-1U)
#define MASK_RM (OP_MASK_RM << OP_SH_RM)
#define MASK_PRED (OP_MASK_PRED << OP_SH_PRED)
#define MASK_SUCC (OP_MASK_SUCC << OP_SH_SUCC)
#define MASK_AQ (OP_MASK_AQ << OP_SH_AQ)
#define MASK_RL (OP_MASK_RL << OP_SH_RL)
#define MASK_AQRL (MASK_AQ | MASK_RL)

static int
match_opcode (const struct riscv_opcode *op, insn_t insn)
{
  return ((insn ^ op->match) & op->mask) == 0;
}

static int
match_never (const struct riscv_opcode *op ATTRIBUTE_UNUSED,
	     insn_t insn ATTRIBUTE_UNUSED)
{
  return 0;
}

static int
match_rs1_eq_rs2 (const struct riscv_opcode *op, insn_t insn)
{
  int rs1 = (insn & MASK_RS1) >> OP_SH_RS1;
  int rs2 = (insn & MASK_RS2) >> OP_SH_RS2;
  return match_opcode (op, insn) && rs1 == rs2;
}

static int
match_rd_nonzero (const struct riscv_opcode *op, insn_t insn)
{
  return match_opcode (op, insn) && ((insn & MASK_RD) != 0);
}

static int
match_c_add (const struct riscv_opcode *op, insn_t insn)
{
  return match_rd_nonzero (op, insn) && ((insn & MASK_CRS2) != 0);
}

static int
match_c_lui (const struct riscv_opcode *op, insn_t insn)
{
  return match_rd_nonzero (op, insn) && (((insn & MASK_RD) >> OP_SH_RD) != 2);
}

const struct riscv_opcode riscv_opcodes[] =
{
/* name,      isa,   operands, match, mask, match_func, pinfo.  */
{"unimp",     "C",   "",  0, 0xffffU,  match_opcode, 0 },
/* {"unimp",     "I",   "",  MATCH_CSRRW | (CSR_CYCLE << OP_SH_CSR), 0xffffffffU,  match_opcode, 0 }, */
/* csrw cycle, x0 */
{"ebreak",    "C",   "",  MATCH_C_EBREAK, MASK_C_EBREAK, match_opcode, INSN_ALIAS },
{"ebreak",    "I",   "",    MATCH_EBREAK, MASK_EBREAK, match_opcode, 0 },
{"sbreak",    "C",   "",  MATCH_C_EBREAK, MASK_C_EBREAK, match_opcode, INSN_ALIAS },
{"sbreak",    "I",   "",    MATCH_EBREAK, MASK_EBREAK, match_opcode, INSN_ALIAS },
{"ret",       "C",   "",  MATCH_C_JR | (X_RA << OP_SH_RD), MASK_C_JR | MASK_RD, match_opcode, INSN_ALIAS },
{"ret",       "I",   "",  MATCH_JALR | (X_RA << OP_SH_RS1), MASK_JALR | MASK_RD | MASK_RS1 | MASK_IMM, match_opcode, INSN_ALIAS },
{"jr",        "C",   "d",  MATCH_C_JR, MASK_C_JR, match_rd_nonzero, INSN_ALIAS },
{"jr",        "I",   "s",  MATCH_JALR, MASK_JALR | MASK_RD | MASK_IMM, match_opcode, INSN_ALIAS },
{"jr",        "I",   "o(s)",  MATCH_JALR, MASK_JALR | MASK_RD, match_opcode, INSN_ALIAS },
{"jr",        "I",   "s,j",  MATCH_JALR, MASK_JALR | MASK_RD, match_opcode, INSN_ALIAS },
{"jalr",      "C",   "d",  MATCH_C_JALR, MASK_C_JALR, match_rd_nonzero, INSN_ALIAS },
{"jalr",      "I",   "s",  MATCH_JALR | (X_RA << OP_SH_RD), MASK_JALR | MASK_RD | MASK_IMM, match_opcode, INSN_ALIAS },
{"jalr",      "I",   "o(s)",  MATCH_JALR | (X_RA << OP_SH_RD), MASK_JALR | MASK_RD, match_opcode, INSN_ALIAS },
{"jalr",      "I",   "s,j",  MATCH_JALR | (X_RA << OP_SH_RD), MASK_JALR | MASK_RD, match_opcode, INSN_ALIAS },
{"jalr",      "I",   "d,s",  MATCH_JALR, MASK_JALR | MASK_IMM, match_opcode, INSN_ALIAS },
{"jalr",      "I",   "d,o(s)",  MATCH_JALR, MASK_JALR, match_opcode, 0 },
{"jalr",      "I",   "d,s,j",  MATCH_JALR, MASK_JALR, match_opcode, 0 },
{"j",         "C",   "Ca",  MATCH_C_J, MASK_C_J, match_opcode, INSN_ALIAS },
{"j",         "I",   "a",  MATCH_JAL, MASK_JAL | MASK_RD, match_opcode, INSN_ALIAS },
{"jal",       "I",   "d,a",  MATCH_JAL, MASK_JAL, match_opcode, 0 },
{"jal",       "32C", "Ca",  MATCH_C_JAL, MASK_C_JAL, match_opcode, INSN_ALIAS },
{"jal",       "I",   "a",  MATCH_JAL | (X_RA << OP_SH_RD), MASK_JAL | MASK_RD, match_opcode, INSN_ALIAS },
{"call",      "I",   "d,c", (X_T1 << OP_SH_RS1), (int) M_CALL,  match_never, INSN_MACRO },
{"call",      "I",   "c", (X_RA << OP_SH_RS1) | (X_RA << OP_SH_RD), (int) M_CALL,  match_never, INSN_MACRO },
{"tail",      "I",   "c", (X_T1 << OP_SH_RS1), (int) M_CALL,  match_never, INSN_MACRO },
{"jump",      "I",   "c,s", 0, (int) M_CALL,  match_never, INSN_MACRO },
{"nop",       "C",   "",  MATCH_C_ADDI, 0xffff, match_opcode, INSN_ALIAS },
{"nop",       "I",   "",         MATCH_ADDI, MASK_ADDI | MASK_RD | MASK_RS1 | MASK_IMM, match_opcode, INSN_ALIAS },
{"lui",       "C",   "d,Cu",  MATCH_C_LUI, MASK_C_LUI, match_c_lui, INSN_ALIAS },
{"lui",       "I",   "d,u",  MATCH_LUI, MASK_LUI, match_opcode, 0 },
{"li",        "C",   "d,Cv",  MATCH_C_LUI, MASK_C_LUI, match_c_lui, INSN_ALIAS },
{"li",        "C",   "d,Cj",  MATCH_C_LI, MASK_C_LI, match_rd_nonzero, INSN_ALIAS },
{"li",        "C",   "d,0",  MATCH_C_LI, MASK_C_LI | MASK_RVC_IMM, match_rd_nonzero, INSN_ALIAS },
{"li",        "I",   "d,j",      MATCH_ADDI, MASK_ADDI | MASK_RS1, match_opcode, INSN_ALIAS }, /* addi */
{"li",        "I",   "d,I",  0,    (int) M_LI,  match_never, INSN_MACRO },
{"mv",        "C",   "d,CV",  MATCH_C_MV, MASK_C_MV, match_c_add, INSN_ALIAS },
{"mv",        "I",   "d,s",  MATCH_ADDI, MASK_ADDI | MASK_IMM, match_opcode, INSN_ALIAS },
{"move",      "C",   "d,CV",  MATCH_C_MV, MASK_C_MV, match_c_add, INSN_ALIAS },
{"move",      "I",   "d,s",  MATCH_ADDI, MASK_ADDI | MASK_IMM, match_opcode, INSN_ALIAS },
{"andi",      "C",   "Cs,Cw,Cj",  MATCH_C_ANDI, MASK_C_ANDI, match_opcode, INSN_ALIAS },
{"andi",      "I",   "d,s,j",  MATCH_ANDI, MASK_ANDI, match_opcode, 0 },
{"and",       "C",   "Cs,Cw,Ct",  MATCH_C_AND, MASK_C_AND, match_opcode, INSN_ALIAS },
{"and",       "C",   "Cs,Ct,Cw",  MATCH_C_AND, MASK_C_AND, match_opcode, INSN_ALIAS },
{"and",       "C",   "Cs,Cw,Cj",  MATCH_C_ANDI, MASK_C_ANDI, match_opcode, INSN_ALIAS },
{"and",       "I",   "d,s,t",  MATCH_AND, MASK_AND, match_opcode, 0 },
{"and",       "I",   "d,s,j",  MATCH_ANDI, MASK_ANDI, match_opcode, INSN_ALIAS },
{"beqz",      "C",   "Cs,Cp",  MATCH_C_BEQZ, MASK_C_BEQZ, match_opcode, INSN_ALIAS },
{"beqz",      "I",   "s,p",  MATCH_BEQ, MASK_BEQ | MASK_RS2, match_opcode, INSN_ALIAS },
{"beq",       "I",   "s,t,p",  MATCH_BEQ, MASK_BEQ, match_opcode, 0 },
{"blez",      "I",   "t,p",  MATCH_BGE, MASK_BGE | MASK_RS1, match_opcode, INSN_ALIAS },
{"bgez",      "I",   "s,p",  MATCH_BGE, MASK_BGE | MASK_RS2, match_opcode, INSN_ALIAS },
{"ble",       "I",   "t,s,p",  MATCH_BGE, MASK_BGE, match_opcode, INSN_ALIAS },
{"bleu",      "I",   "t,s,p",  MATCH_BGEU, MASK_BGEU, match_opcode, INSN_ALIAS },
{"bge",       "I",   "s,t,p",  MATCH_BGE, MASK_BGE, match_opcode, 0 },
{"bgeu",      "I",   "s,t,p",  MATCH_BGEU, MASK_BGEU, match_opcode, 0 },
{"bltz",      "I",   "s,p",  MATCH_BLT, MASK_BLT | MASK_RS2, match_opcode, INSN_ALIAS },
{"bgtz",      "I",   "t,p",  MATCH_BLT, MASK_BLT | MASK_RS1, match_opcode, INSN_ALIAS },
{"blt",       "I",   "s,t,p",  MATCH_BLT, MASK_BLT, match_opcode, 0 },
{"bltu",      "I",   "s,t,p",  MATCH_BLTU, MASK_BLTU, match_opcode, 0 },
{"bgt",       "I",   "t,s,p",  MATCH_BLT, MASK_BLT, match_opcode, INSN_ALIAS },
{"bgtu",      "I",   "t,s,p",  MATCH_BLTU, MASK_BLTU, match_opcode, INSN_ALIAS },
{"bnez",      "C",   "Cs,Cp",  MATCH_C_BNEZ, MASK_C_BNEZ, match_opcode, INSN_ALIAS },
{"bnez",      "I",   "s,p",  MATCH_BNE, MASK_BNE | MASK_RS2, match_opcode, INSN_ALIAS },
{"bne",       "I",   "s,t,p",  MATCH_BNE, MASK_BNE, match_opcode, 0 },
{"addi",      "C",   "Ct,Cc,CK", MATCH_C_ADDI4SPN, MASK_C_ADDI4SPN, match_opcode, INSN_ALIAS },
{"addi",      "C",   "d,CU,Cj",  MATCH_C_ADDI, MASK_C_ADDI, match_rd_nonzero, INSN_ALIAS },
{"addi",      "C",   "Cc,Cc,CL", MATCH_C_ADDI16SP, MASK_C_ADDI16SP, match_opcode, INSN_ALIAS },
{"addi",      "I",   "d,s,j",  MATCH_ADDI, MASK_ADDI, match_opcode, 0 },
{"add",       "C",   "d,CU,CV",  MATCH_C_ADD, MASK_C_ADD, match_c_add, INSN_ALIAS },
{"add",       "C",   "d,CV,CU",  MATCH_C_ADD, MASK_C_ADD, match_c_add, INSN_ALIAS },
{"add",       "C",   "d,CU,Cj",  MATCH_C_ADDI, MASK_C_ADDI, match_rd_nonzero, INSN_ALIAS },
{"add",       "C",   "Ct,Cc,CK", MATCH_C_ADDI4SPN, MASK_C_ADDI4SPN, match_opcode, INSN_ALIAS },
{"add",       "C",   "Cc,Cc,CL", MATCH_C_ADDI16SP, MASK_C_ADDI16SP, match_opcode, INSN_ALIAS },
{"add",       "I",   "d,s,t",  MATCH_ADD, MASK_ADD, match_opcode, 0 },
{"add",       "I",   "d,s,t,0",MATCH_ADD, MASK_ADD, match_opcode, 0 },
{"add",       "I",   "d,s,j",  MATCH_ADDI, MASK_ADDI, match_opcode, INSN_ALIAS },
{"la",        "I",   "d,A",  0,    (int) M_LA,  match_never, INSN_MACRO },
{"lla",       "I",   "d,A",  0,    (int) M_LLA,  match_never, INSN_MACRO },
{"la.tls.gd", "I",   "d,A",  0,    (int) M_LA_TLS_GD,  match_never, INSN_MACRO },
{"la.tls.ie", "I",   "d,A",  0,    (int) M_LA_TLS_IE,  match_never, INSN_MACRO },
{"neg",       "I",   "d,t",  MATCH_SUB, MASK_SUB | MASK_RS1, match_opcode, INSN_ALIAS }, /* sub 0 */
{"slli",      "C",   "d,CU,C>",  MATCH_C_SLLI, MASK_C_SLLI, match_rd_nonzero, INSN_ALIAS },
{"slli",      "I",   "d,s,>",   MATCH_SLLI, MASK_SLLI, match_opcode, 0 },
{"sll",       "C",   "d,CU,C>",  MATCH_C_SLLI, MASK_C_SLLI, match_rd_nonzero, INSN_ALIAS },
{"sll",       "I",   "d,s,t",   MATCH_SLL, MASK_SLL, match_opcode, 0 },
{"sll",       "I",   "d,s,>",   MATCH_SLLI, MASK_SLLI, match_opcode, INSN_ALIAS },
{"srli",      "C",   "Cs,Cw,C>",  MATCH_C_SRLI, MASK_C_SRLI, match_opcode, INSN_ALIAS },
{"srli",      "I",   "d,s,>",   MATCH_SRLI, MASK_SRLI, match_opcode, 0 },
{"srl",       "C",   "Cs,Cw,C>",  MATCH_C_SRLI, MASK_C_SRLI, match_opcode, INSN_ALIAS },
{"srl",       "I",   "d,s,t",   MATCH_SRL, MASK_SRL, match_opcode, 0 },
{"srl",       "I",   "d,s,>",   MATCH_SRLI, MASK_SRLI, match_opcode, INSN_ALIAS },
{"srai",      "C",   "Cs,Cw,C>",  MATCH_C_SRAI, MASK_C_SRAI, match_opcode, INSN_ALIAS },
{"srai",      "I",   "d,s,>",   MATCH_SRAI, MASK_SRAI, match_opcode, 0 },
{"sra",       "C",   "Cs,Cw,C>",  MATCH_C_SRAI, MASK_C_SRAI, match_opcode, INSN_ALIAS },
{"sra",       "I",   "d,s,t",   MATCH_SRA, MASK_SRA, match_opcode, 0 },
{"sra",       "I",   "d,s,>",   MATCH_SRAI, MASK_SRAI, match_opcode, INSN_ALIAS },
{"sub",       "C",   "Cs,Cw,Ct",  MATCH_C_SUB, MASK_C_SUB, match_opcode, INSN_ALIAS },
{"sub",       "I",   "d,s,t",  MATCH_SUB, MASK_SUB, match_opcode, 0 },
{"lb",        "I",   "d,o(s)",  MATCH_LB, MASK_LB, match_opcode, 0 },
{"lb",        "I",   "d,A",  0, (int) M_LB, match_never, INSN_MACRO },
{"lbu",       "I",   "d,o(s)",  MATCH_LBU, MASK_LBU, match_opcode, 0 },
{"lbu",       "I",   "d,A",  0, (int) M_LBU, match_never, INSN_MACRO },
{"lh",        "I",   "d,o(s)",  MATCH_LH, MASK_LH, match_opcode, 0 },
{"lh",        "I",   "d,A",  0, (int) M_LH, match_never, INSN_MACRO },
{"lhu",       "I",   "d,o(s)",  MATCH_LHU, MASK_LHU, match_opcode, 0 },
{"lhu",       "I",   "d,A",  0, (int) M_LHU, match_never, INSN_MACRO },
{"lw",        "C",   "d,Cm(Cc)",  MATCH_C_LWSP, MASK_C_LWSP, match_rd_nonzero, INSN_ALIAS },
{"lw",        "C",   "Ct,Ck(Cs)",  MATCH_C_LW, MASK_C_LW, match_opcode, INSN_ALIAS },
{"lw",        "I",   "d,o(s)",  MATCH_LW, MASK_LW, match_opcode, 0 },
{"lw",        "I",   "d,A",  0, (int) M_LW, match_never, INSN_MACRO },
{"not",       "I",   "d,s",  MATCH_XORI | MASK_IMM, MASK_XORI | MASK_IMM, match_opcode, INSN_ALIAS },
{"ori",       "I",   "d,s,j",  MATCH_ORI, MASK_ORI, match_opcode, 0 },
{"or",       "C",   "Cs,Cw,Ct",  MATCH_C_OR, MASK_C_OR, match_opcode, INSN_ALIAS },
{"or",       "C",   "Cs,Ct,Cw",  MATCH_C_OR, MASK_C_OR, match_opcode, INSN_ALIAS },
{"or",        "I",   "d,s,t",  MATCH_OR, MASK_OR, match_opcode, 0 },
{"or",        "I",   "d,s,j",  MATCH_ORI, MASK_ORI, match_opcode, INSN_ALIAS },
{"auipc",     "I",   "d,u",  MATCH_AUIPC, MASK_AUIPC, match_opcode, 0 },
{"seqz",      "I",   "d,s",  MATCH_SLTIU | ENCODE_ITYPE_IMM (1), MASK_SLTIU | MASK_IMM, match_opcode, INSN_ALIAS },
{"snez",      "I",   "d,t",  MATCH_SLTU, MASK_SLTU | MASK_RS1, match_opcode, INSN_ALIAS },
{"sltz",      "I",   "d,s",  MATCH_SLT, MASK_SLT | MASK_RS2, match_opcode, INSN_ALIAS },
{"sgtz",      "I",   "d,t",  MATCH_SLT, MASK_SLT | MASK_RS1, match_opcode, INSN_ALIAS },
{"slti",      "I",   "d,s,j",  MATCH_SLTI, MASK_SLTI, match_opcode, INSN_ALIAS },
{"slt",       "I",   "d,s,t",  MATCH_SLT, MASK_SLT, match_opcode, 0 },
{"slt",       "I",   "d,s,j",  MATCH_SLTI, MASK_SLTI, match_opcode, 0 },
{"sltiu",     "I",   "d,s,j",  MATCH_SLTIU, MASK_SLTIU, match_opcode, 0 },
{"sltu",      "I",   "d,s,t",  MATCH_SLTU, MASK_SLTU, match_opcode, 0 },
{"sltu",      "I",   "d,s,j",  MATCH_SLTIU, MASK_SLTIU, match_opcode, INSN_ALIAS },
{"sgt",       "I",   "d,t,s",  MATCH_SLT, MASK_SLT, match_opcode, INSN_ALIAS },
{"sgtu",      "I",   "d,t,s",  MATCH_SLTU, MASK_SLTU, match_opcode, INSN_ALIAS },
{"sb",        "I",   "t,q(s)",  MATCH_SB, MASK_SB, match_opcode, 0 },
{"sb",        "I",   "t,A,s",  0, (int) M_SB, match_never, INSN_MACRO },
{"sh",        "I",   "t,q(s)",  MATCH_SH, MASK_SH, match_opcode, 0 },
{"sh",        "I",   "t,A,s",  0, (int) M_SH, match_never, INSN_MACRO },
{"sw",        "C",   "CV,CM(Cc)",  MATCH_C_SWSP, MASK_C_SWSP, match_opcode, INSN_ALIAS },
{"sw",        "C",   "Ct,Ck(Cs)",  MATCH_C_SW, MASK_C_SW, match_opcode, INSN_ALIAS },
{"sw",        "I",   "t,q(s)",  MATCH_SW, MASK_SW, match_opcode, 0 },
{"sw",        "I",   "t,A,s",  0, (int) M_SW, match_never, INSN_MACRO },
{"fence",     "I",   "",  MATCH_FENCE | MASK_PRED | MASK_SUCC, MASK_FENCE | MASK_RD | MASK_RS1 | MASK_IMM, match_opcode, INSN_ALIAS },
{"fence",     "I",   "P,Q",  MATCH_FENCE, MASK_FENCE | MASK_RD | MASK_RS1 | (MASK_IMM & ~MASK_PRED & ~MASK_SUCC), match_opcode, 0 },
{"fence.i",   "I",   "",  MATCH_FENCE_I, MASK_FENCE | MASK_RD | MASK_RS1 | MASK_IMM, match_opcode, 0 },
{"rdcycle",   "I",   "d",  MATCH_RDCYCLE, MASK_RDCYCLE, match_opcode, INSN_ALIAS },
{"rdinstret", "I",   "d",  MATCH_RDINSTRET, MASK_RDINSTRET, match_opcode, INSN_ALIAS },
{"rdtime",    "I",   "d",  MATCH_RDTIME, MASK_RDTIME, match_opcode, INSN_ALIAS },
{"rdcycleh",  "32I", "d",  MATCH_RDCYCLEH, MASK_RDCYCLEH, match_opcode, INSN_ALIAS },
{"rdinstreth","32I", "d",  MATCH_RDINSTRETH, MASK_RDINSTRETH, match_opcode, INSN_ALIAS },
{"rdtimeh",   "32I", "d",  MATCH_RDTIMEH, MASK_RDTIMEH, match_opcode, INSN_ALIAS },
{"ecall",     "I",   "",    MATCH_SCALL, MASK_SCALL, match_opcode, 0 },
{"scall",     "I",   "",    MATCH_SCALL, MASK_SCALL, match_opcode, 0 },

{"scallimm",  "I",   "b3",   MATCH_SCALL, MASK_SCALL_IMM, match_opcode,   0 },

{"xori",      "I",   "d,s,j",  MATCH_XORI, MASK_XORI, match_opcode, 0 },
{"xor",       "C",   "Cs,Cw,Ct",  MATCH_C_XOR, MASK_C_XOR, match_opcode, INSN_ALIAS },
{"xor",       "C",   "Cs,Ct,Cw",  MATCH_C_XOR, MASK_C_XOR, match_opcode, INSN_ALIAS },
{"xor",       "I",   "d,s,t",  MATCH_XOR, MASK_XOR, match_opcode, 0 },
{"xor",       "I",   "d,s,j",  MATCH_XORI, MASK_XORI, match_opcode, INSN_ALIAS },
{"lwu",       "64I", "d,o(s)",  MATCH_LWU, MASK_LWU, match_opcode, 0 },
{"lwu",       "64I", "d,A",  0, (int) M_LWU, match_never, INSN_MACRO },
{"ld",        "64C", "d,Cn(Cc)",  MATCH_C_LDSP, MASK_C_LDSP, match_rd_nonzero, INSN_ALIAS },
{"ld",        "64C", "Ct,Cl(Cs)",  MATCH_C_LD, MASK_C_LD, match_opcode, INSN_ALIAS },
{"ld",        "64I", "d,o(s)", MATCH_LD, MASK_LD, match_opcode, 0 },
{"ld",        "64I", "d,A",  0, (int) M_LD, match_never, INSN_MACRO },
{"sd",        "64C", "CV,CN(Cc)",  MATCH_C_SDSP, MASK_C_SDSP, match_opcode, INSN_ALIAS },
{"sd",        "64C", "Ct,Cl(Cs)",  MATCH_C_SD, MASK_C_SD, match_opcode, INSN_ALIAS },
{"sd",        "64I", "t,q(s)",  MATCH_SD, MASK_SD, match_opcode, 0 },
{"sd",        "64I", "t,A,s",  0, (int) M_SD, match_never, INSN_MACRO },
{"sext.w",    "64C", "d,CU",  MATCH_C_ADDIW, MASK_C_ADDIW | MASK_RVC_IMM, match_rd_nonzero, INSN_ALIAS },
{"sext.w",    "64I", "d,s",  MATCH_ADDIW, MASK_ADDIW | MASK_IMM, match_opcode, INSN_ALIAS },
{"addiw",     "64C", "d,CU,Cj",  MATCH_C_ADDIW, MASK_C_ADDIW, match_rd_nonzero, INSN_ALIAS },
{"addiw",     "64I", "d,s,j",  MATCH_ADDIW, MASK_ADDIW, match_opcode, 0 },
{"addw",      "64C", "Cs,Cw,Ct",  MATCH_C_ADDW, MASK_C_ADDW, match_opcode, INSN_ALIAS },
{"addw",      "64C", "Cs,Ct,Cw",  MATCH_C_ADDW, MASK_C_ADDW, match_opcode, INSN_ALIAS },
{"addw",      "64C", "d,CU,Cj",  MATCH_C_ADDIW, MASK_C_ADDIW, match_rd_nonzero, INSN_ALIAS },
{"addw",      "64I", "d,s,t",  MATCH_ADDW, MASK_ADDW, match_opcode, 0 },
{"addw",      "64I", "d,s,j",  MATCH_ADDIW, MASK_ADDIW, match_opcode, INSN_ALIAS },
{"negw",      "64I", "d,t",  MATCH_SUBW, MASK_SUBW | MASK_RS1, match_opcode, INSN_ALIAS }, /* sub 0 */
{"slliw",     "64I", "d,s,<",   MATCH_SLLIW, MASK_SLLIW, match_opcode, 0 },
{"sllw",      "64I", "d,s,t",   MATCH_SLLW, MASK_SLLW, match_opcode, 0 },
{"sllw",      "64I", "d,s,<",   MATCH_SLLIW, MASK_SLLIW, match_opcode, INSN_ALIAS },
{"srliw",     "64I", "d,s,<",   MATCH_SRLIW, MASK_SRLIW, match_opcode, 0 },
{"srlw",      "64I", "d,s,t",   MATCH_SRLW, MASK_SRLW, match_opcode, 0 },
{"srlw",      "64I", "d,s,<",   MATCH_SRLIW, MASK_SRLIW, match_opcode, INSN_ALIAS },
{"sraiw",     "64I", "d,s,<",   MATCH_SRAIW, MASK_SRAIW, match_opcode, 0 },
{"sraw",      "64I", "d,s,t",   MATCH_SRAW, MASK_SRAW, match_opcode, 0 },
{"sraw",      "64I", "d,s,<",   MATCH_SRAIW, MASK_SRAIW, match_opcode, INSN_ALIAS },
{"subw",      "64C", "Cs,Cw,Ct",  MATCH_C_SUBW, MASK_C_SUBW, match_opcode, INSN_ALIAS },
{"subw",      "64I", "d,s,t",  MATCH_SUBW, MASK_SUBW, match_opcode, 0 },

/* Atomic memory operation instruction subset */
{"lr.w",         "A",   "d,0(s)",    MATCH_LR_W, MASK_LR_W | MASK_AQRL, match_opcode, 0 },
{"sc.w",         "A",   "d,t,0(s)",  MATCH_SC_W, MASK_SC_W | MASK_AQRL, match_opcode, 0 },
{"amoadd.w",     "A",   "d,t,0(s)",  MATCH_AMOADD_W, MASK_AMOADD_W | MASK_AQRL, match_opcode, 0 },
{"amoswap.w",    "A",   "d,t,0(s)",  MATCH_AMOSWAP_W, MASK_AMOSWAP_W | MASK_AQRL, match_opcode, 0 },
{"amoand.w",     "A",   "d,t,0(s)",  MATCH_AMOAND_W, MASK_AMOAND_W | MASK_AQRL, match_opcode, 0 },
{"amoor.w",      "A",   "d,t,0(s)",  MATCH_AMOOR_W, MASK_AMOOR_W | MASK_AQRL, match_opcode, 0 },
{"amoxor.w",     "A",   "d,t,0(s)",  MATCH_AMOXOR_W, MASK_AMOXOR_W | MASK_AQRL, match_opcode, 0 },
{"amomax.w",     "A",   "d,t,0(s)",  MATCH_AMOMAX_W, MASK_AMOMAX_W | MASK_AQRL, match_opcode, 0 },
{"amomaxu.w",    "A",   "d,t,0(s)",  MATCH_AMOMAXU_W, MASK_AMOMAXU_W | MASK_AQRL, match_opcode, 0 },
{"amomin.w",     "A",   "d,t,0(s)",  MATCH_AMOMIN_W, MASK_AMOMIN_W | MASK_AQRL, match_opcode, 0 },
{"amominu.w",    "A",   "d,t,0(s)",  MATCH_AMOMINU_W, MASK_AMOMINU_W | MASK_AQRL, match_opcode, 0 },
{"lr.w.aq",      "A",   "d,0(s)",    MATCH_LR_W | MASK_AQ, MASK_LR_W | MASK_AQRL, match_opcode, 0 },
{"sc.w.aq",      "A",   "d,t,0(s)",  MATCH_SC_W | MASK_AQ, MASK_SC_W | MASK_AQRL, match_opcode, 0 },
{"amoadd.w.aq",  "A",   "d,t,0(s)",  MATCH_AMOADD_W | MASK_AQ, MASK_AMOADD_W | MASK_AQRL, match_opcode, 0 },
{"amoswap.w.aq", "A",   "d,t,0(s)",  MATCH_AMOSWAP_W | MASK_AQ, MASK_AMOSWAP_W | MASK_AQRL, match_opcode, 0 },
{"amoand.w.aq",  "A",   "d,t,0(s)",  MATCH_AMOAND_W | MASK_AQ, MASK_AMOAND_W | MASK_AQRL, match_opcode, 0 },
{"amoor.w.aq",   "A",   "d,t,0(s)",  MATCH_AMOOR_W | MASK_AQ, MASK_AMOOR_W | MASK_AQRL, match_opcode, 0 },
{"amoxor.w.aq",  "A",   "d,t,0(s)",  MATCH_AMOXOR_W | MASK_AQ, MASK_AMOXOR_W | MASK_AQRL, match_opcode, 0 },
{"amomax.w.aq",  "A",   "d,t,0(s)",  MATCH_AMOMAX_W | MASK_AQ, MASK_AMOMAX_W | MASK_AQRL, match_opcode, 0 },
{"amomaxu.w.aq", "A",   "d,t,0(s)",  MATCH_AMOMAXU_W | MASK_AQ, MASK_AMOMAXU_W | MASK_AQRL, match_opcode, 0 },
{"amomin.w.aq",  "A",   "d,t,0(s)",  MATCH_AMOMIN_W | MASK_AQ, MASK_AMOMIN_W | MASK_AQRL, match_opcode, 0 },
{"amominu.w.aq", "A",   "d,t,0(s)",  MATCH_AMOMINU_W | MASK_AQ, MASK_AMOMINU_W | MASK_AQRL, match_opcode, 0 },
{"lr.w.rl",      "A",   "d,0(s)",    MATCH_LR_W | MASK_RL, MASK_LR_W | MASK_AQRL, match_opcode, 0 },
{"sc.w.rl",      "A",   "d,t,0(s)",  MATCH_SC_W | MASK_RL, MASK_SC_W | MASK_AQRL, match_opcode, 0 },
{"amoadd.w.rl",  "A",   "d,t,0(s)",  MATCH_AMOADD_W | MASK_RL, MASK_AMOADD_W | MASK_AQRL, match_opcode, 0 },
{"amoswap.w.rl", "A",   "d,t,0(s)",  MATCH_AMOSWAP_W | MASK_RL, MASK_AMOSWAP_W | MASK_AQRL, match_opcode, 0 },
{"amoand.w.rl",  "A",   "d,t,0(s)",  MATCH_AMOAND_W | MASK_RL, MASK_AMOAND_W | MASK_AQRL, match_opcode, 0 },
{"amoor.w.rl",   "A",   "d,t,0(s)",  MATCH_AMOOR_W | MASK_RL, MASK_AMOOR_W | MASK_AQRL, match_opcode, 0 },
{"amoxor.w.rl",  "A",   "d,t,0(s)",  MATCH_AMOXOR_W | MASK_RL, MASK_AMOXOR_W | MASK_AQRL, match_opcode, 0 },
{"amomax.w.rl",  "A",   "d,t,0(s)",  MATCH_AMOMAX_W | MASK_RL, MASK_AMOMAX_W | MASK_AQRL, match_opcode, 0 },
{"amomaxu.w.rl", "A",   "d,t,0(s)",  MATCH_AMOMAXU_W | MASK_RL, MASK_AMOMAXU_W | MASK_AQRL, match_opcode, 0 },
{"amomin.w.rl",  "A",   "d,t,0(s)",  MATCH_AMOMIN_W | MASK_RL, MASK_AMOMIN_W | MASK_AQRL, match_opcode, 0 },
{"amominu.w.rl", "A",   "d,t,0(s)",  MATCH_AMOMINU_W | MASK_RL, MASK_AMOMINU_W | MASK_AQRL, match_opcode, 0 },
{"lr.w.aqrl",    "A",   "d,0(s)",    MATCH_LR_W | MASK_AQRL, MASK_LR_W | MASK_AQRL, match_opcode, 0 },
{"sc.w.aqrl",    "A",   "d,t,0(s)",  MATCH_SC_W | MASK_AQRL, MASK_SC_W | MASK_AQRL, match_opcode, 0 },
{"amoadd.w.aqrl",  "A",   "d,t,0(s)",  MATCH_AMOADD_W | MASK_AQRL, MASK_AMOADD_W | MASK_AQRL, match_opcode, 0 },
{"amoswap.w.aqrl", "A",   "d,t,0(s)",  MATCH_AMOSWAP_W | MASK_AQRL, MASK_AMOSWAP_W | MASK_AQRL, match_opcode, 0 },
{"amoand.w.aqrl",  "A",   "d,t,0(s)",  MATCH_AMOAND_W | MASK_AQRL, MASK_AMOAND_W | MASK_AQRL, match_opcode, 0 },
{"amoor.w.aqrl",   "A",   "d,t,0(s)",  MATCH_AMOOR_W | MASK_AQRL, MASK_AMOOR_W | MASK_AQRL, match_opcode, 0 },
{"amoxor.w.aqrl",  "A",   "d,t,0(s)",  MATCH_AMOXOR_W | MASK_AQRL, MASK_AMOXOR_W | MASK_AQRL, match_opcode, 0 },
{"amomax.w.aqrl",  "A",   "d,t,0(s)",  MATCH_AMOMAX_W | MASK_AQRL, MASK_AMOMAX_W | MASK_AQRL, match_opcode, 0 },
{"amomaxu.w.aqrl", "A",   "d,t,0(s)",  MATCH_AMOMAXU_W | MASK_AQRL, MASK_AMOMAXU_W | MASK_AQRL, match_opcode, 0 },
{"amomin.w.aqrl",  "A",   "d,t,0(s)",  MATCH_AMOMIN_W | MASK_AQRL, MASK_AMOMIN_W | MASK_AQRL, match_opcode, 0 },
{"amominu.w.aqrl", "A",   "d,t,0(s)",  MATCH_AMOMINU_W | MASK_AQRL, MASK_AMOMINU_W | MASK_AQRL, match_opcode, 0 },
{"lr.d",         "64A", "d,0(s)",    MATCH_LR_D, MASK_LR_D | MASK_AQRL, match_opcode, 0 },
{"sc.d",         "64A", "d,t,0(s)",  MATCH_SC_D, MASK_SC_D | MASK_AQRL, match_opcode, 0 },
{"amoadd.d",     "64A", "d,t,0(s)",  MATCH_AMOADD_D, MASK_AMOADD_D | MASK_AQRL, match_opcode, 0 },
{"amoswap.d",    "64A", "d,t,0(s)",  MATCH_AMOSWAP_D, MASK_AMOSWAP_D | MASK_AQRL, match_opcode, 0 },
{"amoand.d",     "64A", "d,t,0(s)",  MATCH_AMOAND_D, MASK_AMOAND_D | MASK_AQRL, match_opcode, 0 },
{"amoor.d",      "64A", "d,t,0(s)",  MATCH_AMOOR_D, MASK_AMOOR_D | MASK_AQRL, match_opcode, 0 },
{"amoxor.d",     "64A", "d,t,0(s)",  MATCH_AMOXOR_D, MASK_AMOXOR_D | MASK_AQRL, match_opcode, 0 },
{"amomax.d",     "64A", "d,t,0(s)",  MATCH_AMOMAX_D, MASK_AMOMAX_D | MASK_AQRL, match_opcode, 0 },
{"amomaxu.d",    "64A", "d,t,0(s)",  MATCH_AMOMAXU_D, MASK_AMOMAXU_D | MASK_AQRL, match_opcode, 0 },
{"amomin.d",     "64A", "d,t,0(s)",  MATCH_AMOMIN_D, MASK_AMOMIN_D | MASK_AQRL, match_opcode, 0 },
{"amominu.d",    "64A", "d,t,0(s)",  MATCH_AMOMINU_D, MASK_AMOMINU_D | MASK_AQRL, match_opcode, 0 },
{"lr.d.aq",      "64A", "d,0(s)",    MATCH_LR_D | MASK_AQ, MASK_LR_D | MASK_AQRL, match_opcode, 0 },
{"sc.d.aq",      "64A", "d,t,0(s)",  MATCH_SC_D | MASK_AQ, MASK_SC_D | MASK_AQRL, match_opcode, 0 },
{"amoadd.d.aq",  "64A", "d,t,0(s)",  MATCH_AMOADD_D | MASK_AQ, MASK_AMOADD_D | MASK_AQRL, match_opcode, 0 },
{"amoswap.d.aq", "64A", "d,t,0(s)",  MATCH_AMOSWAP_D | MASK_AQ, MASK_AMOSWAP_D | MASK_AQRL, match_opcode, 0 },
{"amoand.d.aq",  "64A", "d,t,0(s)",  MATCH_AMOAND_D | MASK_AQ, MASK_AMOAND_D | MASK_AQRL, match_opcode, 0 },
{"amoor.d.aq",   "64A", "d,t,0(s)",  MATCH_AMOOR_D | MASK_AQ, MASK_AMOOR_D | MASK_AQRL, match_opcode, 0 },
{"amoxor.d.aq",  "64A", "d,t,0(s)",  MATCH_AMOXOR_D | MASK_AQ, MASK_AMOXOR_D | MASK_AQRL, match_opcode, 0 },
{"amomax.d.aq",  "64A", "d,t,0(s)",  MATCH_AMOMAX_D | MASK_AQ, MASK_AMOMAX_D | MASK_AQRL, match_opcode, 0 },
{"amomaxu.d.aq", "64A", "d,t,0(s)",  MATCH_AMOMAXU_D | MASK_AQ, MASK_AMOMAXU_D | MASK_AQRL, match_opcode, 0 },
{"amomin.d.aq",  "64A", "d,t,0(s)",  MATCH_AMOMIN_D | MASK_AQ, MASK_AMOMIN_D | MASK_AQRL, match_opcode, 0 },
{"amominu.d.aq", "64A", "d,t,0(s)",  MATCH_AMOMINU_D | MASK_AQ, MASK_AMOMINU_D | MASK_AQRL, match_opcode, 0 },
{"lr.d.rl",      "64A", "d,0(s)",    MATCH_LR_D | MASK_RL, MASK_LR_D | MASK_AQRL, match_opcode, 0 },
{"sc.d.rl",      "64A", "d,t,0(s)",  MATCH_SC_D | MASK_RL, MASK_SC_D | MASK_AQRL, match_opcode, 0 },
{"amoadd.d.rl",  "64A", "d,t,0(s)",  MATCH_AMOADD_D | MASK_RL, MASK_AMOADD_D | MASK_AQRL, match_opcode, 0 },
{"amoswap.d.rl", "64A", "d,t,0(s)",  MATCH_AMOSWAP_D | MASK_RL, MASK_AMOSWAP_D | MASK_AQRL, match_opcode, 0 },
{"amoand.d.rl",  "64A", "d,t,0(s)",  MATCH_AMOAND_D | MASK_RL, MASK_AMOAND_D | MASK_AQRL, match_opcode, 0 },
{"amoor.d.rl",   "64A", "d,t,0(s)",  MATCH_AMOOR_D | MASK_RL, MASK_AMOOR_D | MASK_AQRL, match_opcode, 0 },
{"amoxor.d.rl",  "64A", "d,t,0(s)",  MATCH_AMOXOR_D | MASK_RL, MASK_AMOXOR_D | MASK_AQRL, match_opcode, 0 },
{"amomax.d.rl",  "64A", "d,t,0(s)",  MATCH_AMOMAX_D | MASK_RL, MASK_AMOMAX_D | MASK_AQRL, match_opcode, 0 },
{"amomaxu.d.rl", "64A", "d,t,0(s)",  MATCH_AMOMAXU_D | MASK_RL, MASK_AMOMAXU_D | MASK_AQRL, match_opcode, 0 },
{"amomin.d.rl",  "64A", "d,t,0(s)",  MATCH_AMOMIN_D | MASK_RL, MASK_AMOMIN_D | MASK_AQRL, match_opcode, 0 },
{"amominu.d.rl", "64A", "d,t,0(s)",  MATCH_AMOMINU_D | MASK_RL, MASK_AMOMINU_D | MASK_AQRL, match_opcode, 0 },
{"lr.d.aqrl",    "64A", "d,0(s)",    MATCH_LR_D | MASK_AQRL, MASK_LR_D | MASK_AQRL, match_opcode, 0 },
{"sc.d.aqrl",    "64A", "d,t,0(s)",  MATCH_SC_D | MASK_AQRL, MASK_SC_D | MASK_AQRL, match_opcode, 0 },
{"amoadd.d.aqrl",  "64A", "d,t,0(s)",  MATCH_AMOADD_D | MASK_AQRL, MASK_AMOADD_D | MASK_AQRL, match_opcode, 0 },
{"amoswap.d.aqrl", "64A", "d,t,0(s)",  MATCH_AMOSWAP_D | MASK_AQRL, MASK_AMOSWAP_D | MASK_AQRL, match_opcode, 0 },
{"amoand.d.aqrl",  "64A", "d,t,0(s)",  MATCH_AMOAND_D | MASK_AQRL, MASK_AMOAND_D | MASK_AQRL, match_opcode, 0 },
{"amoor.d.aqrl",   "64A", "d,t,0(s)",  MATCH_AMOOR_D | MASK_AQRL, MASK_AMOOR_D | MASK_AQRL, match_opcode, 0 },
{"amoxor.d.aqrl",  "64A", "d,t,0(s)",  MATCH_AMOXOR_D | MASK_AQRL, MASK_AMOXOR_D | MASK_AQRL, match_opcode, 0 },
{"amomax.d.aqrl",  "64A", "d,t,0(s)",  MATCH_AMOMAX_D | MASK_AQRL, MASK_AMOMAX_D | MASK_AQRL, match_opcode, 0 },
{"amomaxu.d.aqrl", "64A", "d,t,0(s)",  MATCH_AMOMAXU_D | MASK_AQRL, MASK_AMOMAXU_D | MASK_AQRL, match_opcode, 0 },
{"amomin.d.aqrl",  "64A", "d,t,0(s)",  MATCH_AMOMIN_D | MASK_AQRL, MASK_AMOMIN_D | MASK_AQRL, match_opcode, 0 },
{"amominu.d.aqrl", "64A", "d,t,0(s)",  MATCH_AMOMINU_D | MASK_AQRL, MASK_AMOMINU_D | MASK_AQRL, match_opcode, 0 },

/* Multiply/Divide instruction subset */
{"mul",       "M",   "d,s,t",  MATCH_MUL, MASK_MUL, match_opcode, 0 },
{"mulh",      "M",   "d,s,t",  MATCH_MULH, MASK_MULH, match_opcode, 0 },
{"mulhu",     "M",   "d,s,t",  MATCH_MULHU, MASK_MULHU, match_opcode, 0 },
{"mulhsu",    "M",   "d,s,t",  MATCH_MULHSU, MASK_MULHSU, match_opcode, 0 },
{"div",       "M",   "d,s,t",  MATCH_DIV, MASK_DIV, match_opcode, 0 },
{"divu",      "M",   "d,s,t",  MATCH_DIVU, MASK_DIVU, match_opcode, 0 },
{"rem",       "M",   "d,s,t",  MATCH_REM, MASK_REM, match_opcode, 0 },
{"remu",      "M",   "d,s,t",  MATCH_REMU, MASK_REMU, match_opcode, 0 },
{"mulw",      "64M", "d,s,t",  MATCH_MULW, MASK_MULW, match_opcode, 0 },
{"divw",      "64M", "d,s,t",  MATCH_DIVW, MASK_DIVW, match_opcode, 0 },
{"divuw",     "64M", "d,s,t",  MATCH_DIVUW, MASK_DIVUW, match_opcode, 0 },
{"remw",      "64M", "d,s,t",  MATCH_REMW, MASK_REMW, match_opcode, 0 },
{"remuw",     "64M", "d,s,t",  MATCH_REMUW, MASK_REMUW, match_opcode, 0 },

/* Single-precision floating-point instruction subset */
{"frsr",      "F",   "d",  MATCH_FRCSR, MASK_FRCSR, match_opcode, 0 },
{"fssr",      "F",   "s",  MATCH_FSCSR, MASK_FSCSR | MASK_RD, match_opcode, 0 },
{"fssr",      "F",   "d,s",  MATCH_FSCSR, MASK_FSCSR, match_opcode, 0 },
{"frcsr",     "F",   "d",  MATCH_FRCSR, MASK_FRCSR, match_opcode, 0 },
{"fscsr",     "F",   "s",  MATCH_FSCSR, MASK_FSCSR | MASK_RD, match_opcode, 0 },
{"fscsr",     "F",   "d,s",  MATCH_FSCSR, MASK_FSCSR, match_opcode, 0 },
{"frrm",      "F",   "d",  MATCH_FRRM, MASK_FRRM, match_opcode, 0 },
{"fsrm",      "F",   "s",  MATCH_FSRM, MASK_FSRM | MASK_RD, match_opcode, 0 },
{"fsrm",      "F",   "d,s",  MATCH_FSRM, MASK_FSRM, match_opcode, 0 },
{"frflags",   "F",   "d",  MATCH_FRFLAGS, MASK_FRFLAGS, match_opcode, 0 },
{"fsflags",   "F",   "s",  MATCH_FSFLAGS, MASK_FSFLAGS | MASK_RD, match_opcode, 0 },
{"fsflags",   "F",   "d,s",  MATCH_FSFLAGS, MASK_FSFLAGS, match_opcode, 0 },
{"flw",       "32C", "D,Cm(Cc)",  MATCH_C_FLWSP, MASK_C_FLWSP, match_opcode, INSN_ALIAS },
{"flw",       "32C", "CD,Ck(Cs)",  MATCH_C_FLW, MASK_C_FLW, match_opcode, INSN_ALIAS },
{"flw",       "F",   "D,o(s)",  MATCH_FLW, MASK_FLW, match_opcode, 0 },
{"flw",       "F",   "D,A,s",  0, (int) M_FLW, match_never, INSN_MACRO },
{"fsw",       "32C", "CT,CM(Cc)",  MATCH_C_FSWSP, MASK_C_FSWSP, match_opcode, INSN_ALIAS },
{"fsw",       "32C", "CD,Ck(Cs)",  MATCH_C_FSW, MASK_C_FSW, match_opcode, INSN_ALIAS },
{"fsw",       "F",   "T,q(s)",  MATCH_FSW, MASK_FSW, match_opcode, 0 },
{"fsw",       "F",   "T,A,s",  0, (int) M_FSW, match_never, INSN_MACRO },
{"fmv.x.s",   "F",   "d,S",  MATCH_FMV_X_S, MASK_FMV_X_S, match_opcode, 0 },
{"fmv.s.x",   "F",   "D,s",  MATCH_FMV_S_X, MASK_FMV_S_X, match_opcode, 0 },
{"fmv.s",     "F",   "D,U",  MATCH_FSGNJ_S, MASK_FSGNJ_S, match_rs1_eq_rs2, INSN_ALIAS },
{"fneg.s",    "F",   "D,U",  MATCH_FSGNJN_S, MASK_FSGNJN_S, match_rs1_eq_rs2, INSN_ALIAS },
{"fabs.s",    "F",   "D,U",  MATCH_FSGNJX_S, MASK_FSGNJX_S, match_rs1_eq_rs2, INSN_ALIAS },
{"fsgnj.s",   "F",   "D,S,T",  MATCH_FSGNJ_S, MASK_FSGNJ_S, match_opcode, 0 },
{"fsgnjn.s",  "F",   "D,S,T",  MATCH_FSGNJN_S, MASK_FSGNJN_S, match_opcode, 0 },
{"fsgnjx.s",  "F",   "D,S,T",  MATCH_FSGNJX_S, MASK_FSGNJX_S, match_opcode, 0 },
{"fadd.s",    "F",   "D,S,T",  MATCH_FADD_S | MASK_RM, MASK_FADD_S | MASK_RM, match_opcode, 0 },
{"fadd.s",    "F",   "D,S,T,m",  MATCH_FADD_S, MASK_FADD_S, match_opcode, 0 },
{"fsub.s",    "F",   "D,S,T",  MATCH_FSUB_S | MASK_RM, MASK_FSUB_S | MASK_RM, match_opcode, 0 },
{"fsub.s",    "F",   "D,S,T,m",  MATCH_FSUB_S, MASK_FSUB_S, match_opcode, 0 },
{"fmul.s",    "F",   "D,S,T",  MATCH_FMUL_S | MASK_RM, MASK_FMUL_S | MASK_RM, match_opcode, 0 },
{"fmul.s",    "F",   "D,S,T,m",  MATCH_FMUL_S, MASK_FMUL_S, match_opcode, 0 },
{"fdiv.s",    "F",   "D,S,T",  MATCH_FDIV_S | MASK_RM, MASK_FDIV_S | MASK_RM, match_opcode, 0 },
{"fdiv.s",    "F",   "D,S,T,m",  MATCH_FDIV_S, MASK_FDIV_S, match_opcode, 0 },
{"fsqrt.s",   "F",   "D,S",  MATCH_FSQRT_S | MASK_RM, MASK_FSQRT_S | MASK_RM, match_opcode, 0 },
{"fsqrt.s",   "F",   "D,S,m",  MATCH_FSQRT_S, MASK_FSQRT_S, match_opcode, 0 },
{"fmin.s",    "F",   "D,S,T",  MATCH_FMIN_S, MASK_FMIN_S, match_opcode, 0 },
{"fmax.s",    "F",   "D,S,T",  MATCH_FMAX_S, MASK_FMAX_S, match_opcode, 0 },
{"fmadd.s",   "F",   "D,S,T,R",  MATCH_FMADD_S | MASK_RM, MASK_FMADD_S | MASK_RM, match_opcode, 0 },
{"fmadd.s",   "F",   "D,S,T,R,m",  MATCH_FMADD_S, MASK_FMADD_S, match_opcode, 0 },
{"fnmadd.s",  "F",   "D,S,T,R",  MATCH_FNMADD_S | MASK_RM, MASK_FNMADD_S | MASK_RM, match_opcode, 0 },
{"fnmadd.s",  "F",   "D,S,T,R,m",  MATCH_FNMADD_S, MASK_FNMADD_S, match_opcode, 0 },
{"fmsub.s",   "F",   "D,S,T,R",  MATCH_FMSUB_S | MASK_RM, MASK_FMSUB_S | MASK_RM, match_opcode, 0 },
{"fmsub.s",   "F",   "D,S,T,R,m",  MATCH_FMSUB_S, MASK_FMSUB_S, match_opcode, 0 },
{"fnmsub.s",  "F",   "D,S,T,R",  MATCH_FNMSUB_S | MASK_RM, MASK_FNMSUB_S | MASK_RM, match_opcode, 0 },
{"fnmsub.s",  "F",   "D,S,T,R,m",  MATCH_FNMSUB_S, MASK_FNMSUB_S, match_opcode, 0 },
{"fcvt.w.s",  "F",   "d,S",  MATCH_FCVT_W_S | MASK_RM, MASK_FCVT_W_S | MASK_RM, match_opcode, 0 },
{"fcvt.w.s",  "F",   "d,S,m",  MATCH_FCVT_W_S, MASK_FCVT_W_S, match_opcode, 0 },
{"fcvt.wu.s", "F",   "d,S",  MATCH_FCVT_WU_S | MASK_RM, MASK_FCVT_WU_S | MASK_RM, match_opcode, 0 },
{"fcvt.wu.s", "F",   "d,S,m",  MATCH_FCVT_WU_S, MASK_FCVT_WU_S, match_opcode, 0 },
{"fcvt.s.w",  "F",   "D,s",  MATCH_FCVT_S_W | MASK_RM, MASK_FCVT_S_W | MASK_RM, match_opcode, 0 },
{"fcvt.s.w",  "F",   "D,s,m",  MATCH_FCVT_S_W, MASK_FCVT_S_W, match_opcode, 0 },
{"fcvt.s.wu", "F",   "D,s",  MATCH_FCVT_S_WU | MASK_RM, MASK_FCVT_S_W | MASK_RM, match_opcode, 0 },
{"fcvt.s.wu", "F",   "D,s,m",  MATCH_FCVT_S_WU, MASK_FCVT_S_WU, match_opcode, 0 },
{"fclass.s",  "F",   "d,S",  MATCH_FCLASS_S, MASK_FCLASS_S, match_opcode, 0 },
{"feq.s",     "F",   "d,S,T",    MATCH_FEQ_S, MASK_FEQ_S, match_opcode, 0 },
{"flt.s",     "F",   "d,S,T",    MATCH_FLT_S, MASK_FLT_S, match_opcode, 0 },
{"fle.s",     "F",   "d,S,T",    MATCH_FLE_S, MASK_FLE_S, match_opcode, 0 },
{"fgt.s",     "F",   "d,T,S",    MATCH_FLT_S, MASK_FLT_S, match_opcode, 0 },
{"fge.s",     "F",   "d,T,S",    MATCH_FLE_S, MASK_FLE_S, match_opcode, 0 },
{"fcvt.l.s",  "64F", "d,S",  MATCH_FCVT_L_S | MASK_RM, MASK_FCVT_L_S | MASK_RM, match_opcode, 0 },
{"fcvt.l.s",  "64F", "d,S,m",  MATCH_FCVT_L_S, MASK_FCVT_L_S, match_opcode, 0 },
{"fcvt.lu.s", "64F", "d,S",  MATCH_FCVT_LU_S | MASK_RM, MASK_FCVT_LU_S | MASK_RM, match_opcode, 0 },
{"fcvt.lu.s", "64F", "d,S,m",  MATCH_FCVT_LU_S, MASK_FCVT_LU_S, match_opcode, 0 },
{"fcvt.s.l",  "64F", "D,s",  MATCH_FCVT_S_L | MASK_RM, MASK_FCVT_S_L | MASK_RM, match_opcode, 0 },
{"fcvt.s.l",  "64F", "D,s,m",  MATCH_FCVT_S_L, MASK_FCVT_S_L, match_opcode, 0 },
{"fcvt.s.lu", "64F", "D,s",  MATCH_FCVT_S_LU | MASK_RM, MASK_FCVT_S_L | MASK_RM, match_opcode, 0 },
{"fcvt.s.lu", "64F", "D,s,m",  MATCH_FCVT_S_LU, MASK_FCVT_S_LU, match_opcode, 0 },

/* Double-precision floating-point instruction subset */
{"fld",       "C",   "D,Cn(Cc)",  MATCH_C_FLDSP, MASK_C_FLDSP, match_opcode, INSN_ALIAS },
{"fld",       "C",   "CD,Cl(Cs)",  MATCH_C_FLD, MASK_C_FLD, match_opcode, INSN_ALIAS },
{"fld",       "D",   "D,o(s)",  MATCH_FLD, MASK_FLD, match_opcode, 0 },
{"fld",       "D",   "D,A,s",  0, (int) M_FLD, match_never, INSN_MACRO },
{"fsd",       "C",   "CT,CN(Cc)",  MATCH_C_FSDSP, MASK_C_FSDSP, match_opcode, INSN_ALIAS },
{"fsd",       "C",   "CD,Cl(Cs)",  MATCH_C_FSD, MASK_C_FSD, match_opcode, INSN_ALIAS },
{"fsd",       "D",   "T,q(s)",  MATCH_FSD, MASK_FSD, match_opcode, 0 },
{"fsd",       "D",   "T,A,s",  0, (int) M_FSD, match_never, INSN_MACRO },
{"fmv.d",     "D",   "D,U",  MATCH_FSGNJ_D, MASK_FSGNJ_D, match_rs1_eq_rs2, INSN_ALIAS },
{"fneg.d",    "D",   "D,U",  MATCH_FSGNJN_D, MASK_FSGNJN_D, match_rs1_eq_rs2, INSN_ALIAS },
{"fabs.d",    "D",   "D,U",  MATCH_FSGNJX_D, MASK_FSGNJX_D, match_rs1_eq_rs2, INSN_ALIAS },
{"fsgnj.d",   "D",   "D,S,T",  MATCH_FSGNJ_D, MASK_FSGNJ_D, match_opcode, 0 },
{"fsgnjn.d",  "D",   "D,S,T",  MATCH_FSGNJN_D, MASK_FSGNJN_D, match_opcode, 0 },
{"fsgnjx.d",  "D",   "D,S,T",  MATCH_FSGNJX_D, MASK_FSGNJX_D, match_opcode, 0 },
{"fadd.d",    "D",   "D,S,T",  MATCH_FADD_D | MASK_RM, MASK_FADD_D | MASK_RM, match_opcode, 0 },
{"fadd.d",    "D",   "D,S,T,m",  MATCH_FADD_D, MASK_FADD_D, match_opcode, 0 },
{"fsub.d",    "D",   "D,S,T",  MATCH_FSUB_D | MASK_RM, MASK_FSUB_D | MASK_RM, match_opcode, 0 },
{"fsub.d",    "D",   "D,S,T,m",  MATCH_FSUB_D, MASK_FSUB_D, match_opcode, 0 },
{"fmul.d",    "D",   "D,S,T",  MATCH_FMUL_D | MASK_RM, MASK_FMUL_D | MASK_RM, match_opcode, 0 },
{"fmul.d",    "D",   "D,S,T,m",  MATCH_FMUL_D, MASK_FMUL_D, match_opcode, 0 },
{"fdiv.d",    "D",   "D,S,T",  MATCH_FDIV_D | MASK_RM, MASK_FDIV_D | MASK_RM, match_opcode, 0 },
{"fdiv.d",    "D",   "D,S,T,m",  MATCH_FDIV_D, MASK_FDIV_D, match_opcode, 0 },
{"fsqrt.d",   "D",   "D,S",  MATCH_FSQRT_D | MASK_RM, MASK_FSQRT_D | MASK_RM, match_opcode, 0 },
{"fsqrt.d",   "D",   "D,S,m",  MATCH_FSQRT_D, MASK_FSQRT_D, match_opcode, 0 },
{"fmin.d",    "D",   "D,S,T",  MATCH_FMIN_D, MASK_FMIN_D, match_opcode, 0 },
{"fmax.d",    "D",   "D,S,T",  MATCH_FMAX_D, MASK_FMAX_D, match_opcode, 0 },
{"fmadd.d",   "D",   "D,S,T,R",  MATCH_FMADD_D | MASK_RM, MASK_FMADD_D | MASK_RM, match_opcode, 0 },
{"fmadd.d",   "D",   "D,S,T,R,m",  MATCH_FMADD_D, MASK_FMADD_D, match_opcode, 0 },
{"fnmadd.d",  "D",   "D,S,T,R",  MATCH_FNMADD_D | MASK_RM, MASK_FNMADD_D | MASK_RM, match_opcode, 0 },
{"fnmadd.d",  "D",   "D,S,T,R,m",  MATCH_FNMADD_D, MASK_FNMADD_D, match_opcode, 0 },
{"fmsub.d",   "D",   "D,S,T,R",  MATCH_FMSUB_D | MASK_RM, MASK_FMSUB_D | MASK_RM, match_opcode, 0 },
{"fmsub.d",   "D",   "D,S,T,R,m",  MATCH_FMSUB_D, MASK_FMSUB_D, match_opcode, 0 },
{"fnmsub.d",  "D",   "D,S,T,R",  MATCH_FNMSUB_D | MASK_RM, MASK_FNMSUB_D | MASK_RM, match_opcode, 0 },
{"fnmsub.d",  "D",   "D,S,T,R,m",  MATCH_FNMSUB_D, MASK_FNMSUB_D, match_opcode, 0 },
{"fcvt.w.d",  "D",   "d,S",  MATCH_FCVT_W_D | MASK_RM, MASK_FCVT_W_D | MASK_RM, match_opcode, 0 },
{"fcvt.w.d",  "D",   "d,S,m",  MATCH_FCVT_W_D, MASK_FCVT_W_D, match_opcode, 0 },
{"fcvt.wu.d", "D",   "d,S",  MATCH_FCVT_WU_D | MASK_RM, MASK_FCVT_WU_D | MASK_RM, match_opcode, 0 },
{"fcvt.wu.d", "D",   "d,S,m",  MATCH_FCVT_WU_D, MASK_FCVT_WU_D, match_opcode, 0 },
{"fcvt.d.w",  "D",   "D,s",  MATCH_FCVT_D_W, MASK_FCVT_D_W | MASK_RM, match_opcode, 0 },
{"fcvt.d.wu", "D",   "D,s",  MATCH_FCVT_D_WU, MASK_FCVT_D_WU | MASK_RM, match_opcode, 0 },
{"fcvt.d.s",  "D",   "D,S",  MATCH_FCVT_D_S, MASK_FCVT_D_S | MASK_RM, match_opcode, 0 },
{"fcvt.s.d",  "D",   "D,S",  MATCH_FCVT_S_D | MASK_RM, MASK_FCVT_S_D | MASK_RM, match_opcode, 0 },
{"fcvt.s.d",  "D",   "D,S,m",  MATCH_FCVT_S_D, MASK_FCVT_S_D, match_opcode, 0 },
{"fclass.d",  "D",   "d,S",  MATCH_FCLASS_D, MASK_FCLASS_D, match_opcode, 0 },
{"feq.d",     "D",   "d,S,T",    MATCH_FEQ_D, MASK_FEQ_D, match_opcode, 0 },
{"flt.d",     "D",   "d,S,T",    MATCH_FLT_D, MASK_FLT_D, match_opcode, 0 },
{"fle.d",     "D",   "d,S,T",    MATCH_FLE_D, MASK_FLE_D, match_opcode, 0 },
{"fgt.d",     "D",   "d,T,S",    MATCH_FLT_D, MASK_FLT_D, match_opcode, 0 },
{"fge.d",     "D",   "d,T,S",    MATCH_FLE_D, MASK_FLE_D, match_opcode, 0 },
{"fmv.x.d",   "64D", "d,S",  MATCH_FMV_X_D, MASK_FMV_X_D, match_opcode, 0 },
{"fmv.d.x",   "64D", "D,s",  MATCH_FMV_D_X, MASK_FMV_D_X, match_opcode, 0 },
{"fcvt.l.d",  "64D", "d,S",  MATCH_FCVT_L_D | MASK_RM, MASK_FCVT_L_D | MASK_RM, match_opcode, 0 },
{"fcvt.l.d",  "64D", "d,S,m",  MATCH_FCVT_L_D, MASK_FCVT_L_D, match_opcode, 0 },
{"fcvt.lu.d", "64D", "d,S",  MATCH_FCVT_LU_D | MASK_RM, MASK_FCVT_LU_D | MASK_RM, match_opcode, 0 },
{"fcvt.lu.d", "64D", "d,S,m",  MATCH_FCVT_LU_D, MASK_FCVT_LU_D, match_opcode, 0 },
{"fcvt.d.l",  "64D", "D,s",  MATCH_FCVT_D_L | MASK_RM, MASK_FCVT_D_L | MASK_RM, match_opcode, 0 },
{"fcvt.d.l",  "64D", "D,s,m",  MATCH_FCVT_D_L, MASK_FCVT_D_L, match_opcode, 0 },
{"fcvt.d.lu", "64D", "D,s",  MATCH_FCVT_D_LU | MASK_RM, MASK_FCVT_D_L | MASK_RM, match_opcode, 0 },
{"fcvt.d.lu", "64D", "D,s,m",  MATCH_FCVT_D_LU, MASK_FCVT_D_LU, match_opcode, 0 },

/* Compressed instructions.  */
{"c.ebreak",  "C",   "",  MATCH_C_EBREAK, MASK_C_EBREAK, match_opcode, 0 },
{"c.jr",      "C",   "d",  MATCH_C_JR, MASK_C_JR, match_rd_nonzero, 0 },
{"c.jalr",    "C",   "d",  MATCH_C_JALR, MASK_C_JALR, match_rd_nonzero, 0 },
{"c.j",       "C",   "Ca",  MATCH_C_J, MASK_C_J, match_opcode, 0 },
{"c.jal",     "32C", "Ca",  MATCH_C_JAL, MASK_C_JAL, match_opcode, 0 },
{"c.beqz",    "C",   "Cs,Cp",  MATCH_C_BEQZ, MASK_C_BEQZ, match_opcode, 0 },
{"c.bnez",    "C",   "Cs,Cp",  MATCH_C_BNEZ, MASK_C_BNEZ, match_opcode, 0 },
{"c.lwsp",    "C",   "d,Cm(Cc)",  MATCH_C_LWSP, MASK_C_LWSP, match_rd_nonzero, 0 },
{"c.lw",      "C",   "Ct,Ck(Cs)",  MATCH_C_LW, MASK_C_LW, match_opcode, 0 },
{"c.swsp",    "C",   "CV,CM(Cc)",  MATCH_C_SWSP, MASK_C_SWSP, match_opcode, 0 },
{"c.sw",      "C",   "Ct,Ck(Cs)",  MATCH_C_SW, MASK_C_SW, match_opcode, 0 },
{"c.nop",     "C",   "",  MATCH_C_ADDI, 0xffff, match_opcode, 0 },
{"c.mv",      "C",   "d,CV",  MATCH_C_MV, MASK_C_MV, match_c_add, 0 },
{"c.lui",     "C",   "d,Cu",  MATCH_C_LUI, MASK_C_LUI, match_c_lui, 0 },
{"c.li",      "C",   "d,Co",  MATCH_C_LI, MASK_C_LI, match_rd_nonzero, 0 },
{"c.addi4spn","C",   "Ct,Cc,CK", MATCH_C_ADDI4SPN, MASK_C_ADDI4SPN, match_opcode, 0 },
{"c.addi16sp","C",   "Cc,CL", MATCH_C_ADDI16SP, MASK_C_ADDI16SP, match_opcode, 0 },
{"c.addi",    "C",   "d,Cj",  MATCH_C_ADDI, MASK_C_ADDI, match_opcode, 0 },
{"c.add",     "C",   "d,CV",  MATCH_C_ADD, MASK_C_ADD, match_c_add, 0 },
{"c.sub",     "C",   "Cs,Ct",  MATCH_C_SUB, MASK_C_SUB, match_opcode, 0 },
{"c.and",     "C",   "Cs,Ct",  MATCH_C_AND, MASK_C_AND, match_opcode, 0 },
{"c.or",      "C",   "Cs,Ct",  MATCH_C_OR, MASK_C_OR, match_opcode, 0 },
{"c.xor",     "C",   "Cs,Ct",  MATCH_C_XOR, MASK_C_XOR, match_opcode, 0 },
{"c.slli",    "C",   "d,C>",  MATCH_C_SLLI, MASK_C_SLLI, match_rd_nonzero, 0 },
{"c.srli",    "C",   "Cs,C>",  MATCH_C_SRLI, MASK_C_SRLI, match_opcode, 0 },
{"c.srai",    "C",   "Cs,C>",  MATCH_C_SRAI, MASK_C_SRAI, match_opcode, 0 },
{"c.andi",    "C",   "Cs,Co",  MATCH_C_ANDI, MASK_C_ANDI, match_opcode, 0 },
{"c.addiw",   "64C", "d,Co",  MATCH_C_ADDIW, MASK_C_ADDIW, match_rd_nonzero, 0 },
{"c.addw",    "64C", "Cs,Ct",  MATCH_C_ADDW, MASK_C_ADDW, match_opcode, 0 },
{"c.subw",    "64C", "Cs,Ct",  MATCH_C_SUBW, MASK_C_SUBW, match_opcode, 0 },
{"c.ldsp",    "64C", "d,Cn(Cc)",  MATCH_C_LDSP, MASK_C_LDSP, match_rd_nonzero, 0 },
{"c.ld",      "64C", "Ct,Cl(Cs)",  MATCH_C_LD, MASK_C_LD, match_opcode, 0 },
{"c.sdsp",    "64C", "CV,CN(Cc)",  MATCH_C_SDSP, MASK_C_SDSP, match_opcode, 0 },
{"c.sd",      "64C", "Ct,Cl(Cs)",  MATCH_C_SD, MASK_C_SD, match_opcode, 0 },
{"c.fldsp",   "C",   "D,Cn(Cc)",  MATCH_C_FLDSP, MASK_C_FLDSP, match_opcode, 0 },
{"c.fld",     "C",   "CD,Cl(Cs)",  MATCH_C_FLD, MASK_C_FLD, match_opcode, 0 },
{"c.fsdsp",   "C",   "CT,CN(Cc)",  MATCH_C_FSDSP, MASK_C_FSDSP, match_opcode, 0 },
{"c.fsd",     "C",   "CD,Cl(Cs)",  MATCH_C_FSD, MASK_C_FSD, match_opcode, 0 },
{"c.flwsp",   "32C", "D,Cm(Cc)",  MATCH_C_FLWSP, MASK_C_FLWSP, match_opcode, 0 },
{"c.flw",     "32C", "CD,Ck(Cs)",  MATCH_C_FLW, MASK_C_FLW, match_opcode, 0 },
{"c.fswsp",   "32C", "CT,CM(Cc)",  MATCH_C_FSWSP, MASK_C_FSWSP, match_opcode, 0 },
{"c.fsw",     "32C", "CD,Ck(Cs)",  MATCH_C_FSW, MASK_C_FSW, match_opcode, 0 },

/* Supervisor instructions */
{"csrr",      "I",   "d,E",  MATCH_CSRRS, MASK_CSRRS | MASK_RS1, match_opcode, INSN_ALIAS },
{"csrwi",     "I",   "E,Z",  MATCH_CSRRWI, MASK_CSRRWI | MASK_RD, match_opcode, INSN_ALIAS },
{"csrsi",     "I",   "E,Z",  MATCH_CSRRSI, MASK_CSRRSI | MASK_RD, match_opcode, INSN_ALIAS },
{"csrci",     "I",   "E,Z",  MATCH_CSRRCI, MASK_CSRRCI | MASK_RD, match_opcode, INSN_ALIAS },
{"csrw",      "I",   "E,s",  MATCH_CSRRW, MASK_CSRRW | MASK_RD, match_opcode, INSN_ALIAS },
{"csrw",      "I",   "E,Z",  MATCH_CSRRWI, MASK_CSRRWI | MASK_RD, match_opcode, INSN_ALIAS },
{"csrs",      "I",   "E,s",  MATCH_CSRRS, MASK_CSRRS | MASK_RD, match_opcode, INSN_ALIAS },
{"csrs",      "I",   "E,Z",  MATCH_CSRRSI, MASK_CSRRSI | MASK_RD, match_opcode, INSN_ALIAS },
{"csrc",      "I",   "E,s",  MATCH_CSRRC, MASK_CSRRC | MASK_RD, match_opcode, INSN_ALIAS },
{"csrc",      "I",   "E,Z",  MATCH_CSRRCI, MASK_CSRRCI | MASK_RD, match_opcode, INSN_ALIAS },
{"csrrwi",    "I",   "d,E,Z",  MATCH_CSRRWI, MASK_CSRRWI, match_opcode, 0 },
{"csrrsi",    "I",   "d,E,Z",  MATCH_CSRRSI, MASK_CSRRSI, match_opcode, 0 },
{"csrrci",    "I",   "d,E,Z",  MATCH_CSRRCI, MASK_CSRRCI, match_opcode, 0 },
{"csrrw",     "I",   "d,E,s",  MATCH_CSRRW, MASK_CSRRW, match_opcode, 0 },
{"csrrw",     "I",   "d,E,Z",  MATCH_CSRRWI, MASK_CSRRWI, match_opcode, INSN_ALIAS },
{"csrrs",     "I",   "d,E,s",  MATCH_CSRRS, MASK_CSRRS, match_opcode, 0 },
{"csrrs",     "I",   "d,E,Z",  MATCH_CSRRSI, MASK_CSRRSI, match_opcode, INSN_ALIAS },
{"csrrc",     "I",   "d,E,s",  MATCH_CSRRC, MASK_CSRRC, match_opcode, 0 },
{"csrrc",     "I",   "d,E,Z",  MATCH_CSRRCI, MASK_CSRRCI, match_opcode, INSN_ALIAS },
{"uret",      "I",   "",     MATCH_URET, MASK_URET, match_opcode, 0 },
{"sret",      "I",   "",     MATCH_SRET, MASK_SRET, match_opcode, 0 },
{"hret",      "I",   "",     MATCH_HRET, MASK_HRET, match_opcode, 0 },
{"mret",      "I",   "",     MATCH_MRET, MASK_MRET, match_opcode, 0 },
{"dret",      "I",   "",     MATCH_DRET, MASK_DRET, match_opcode, 0 },
{"sfence.vm", "I",   "",     MATCH_SFENCE_VM, MASK_SFENCE_VM | MASK_RS1, match_opcode, 0 },
{"sfence.vm", "I",   "s",    MATCH_SFENCE_VM, MASK_SFENCE_VM, match_opcode, 0 },
{"sfence.vma","I",   "",     MATCH_SFENCE_VMA, MASK_SFENCE_VMA | MASK_RS1 | MASK_RS2, match_opcode, INSN_ALIAS },
{"sfence.vma","I",   "s",    MATCH_SFENCE_VMA, MASK_SFENCE_VMA | MASK_RS2, match_opcode, INSN_ALIAS },
{"sfence.vma","I",   "s,t",  MATCH_SFENCE_VMA, MASK_SFENCE_VMA, match_opcode, 0 },
{"wfi",       "I",   "",     MATCH_WFI, MASK_WFI, match_opcode, 0 },

/* PULP specific opcodes */

/* Pulp slim */

{"p.mul",      		"Xpulpslim", "d,s,t",  	MATCH_MUL, 				MASK_MUL, 	match_opcode,  0},

/* 32x32 into 64 support */

{"p.mulh",      	"Xpulpslim", "d,s,t",  	MATCH_MULH, 				MASK_MULH, 	match_opcode,  0},
{"p.mulhu",     	"Xpulpslim", "d,s,t",  	MATCH_MULHU, 				MASK_MULHU, 	match_opcode,  0},
{"p.mulhsu",    	"Xpulpslim", "d,s,t",  	MATCH_MULHSU, 				MASK_MULHSU, 	match_opcode,  0},

/* 32 bit div and rem */

{"p.div",      		"Xpulpslim", "d,s,t", 	MATCH_DIV, 				MASK_DIV, 	match_opcode,  0},
{"p.divu",     		"Xpulpslim", "d,s,t", 	MATCH_DIVU, 				MASK_DIVU, 	match_opcode,  0},
{"p.rem",      		"Xpulpslim", "d,s,t", 	MATCH_REM, 				MASK_REM, 	match_opcode,  0},
{"p.remu",     		"Xpulpslim", "d,s,t", 	MATCH_REMU,				MASK_REMU, 	match_opcode,  0},

/* Load from event unit */

{"p.elw",		"Xpulpslim", "d,o(s)",	MATCH_LWU, 				MASK_LWU, 	match_opcode,  0},

/* Pulp slim end */

/* Pulp v0 => move to pulp v0, Sven version. Disable HW loop since hw is buggy */

/* post-increment and register-register loads */

{"p.lb",  		"Xpulpv0", "d,o(s)",  	MATCH_LB,        			MASK_LB,      	match_opcode,	0},
{"p.lb",  		"Xpulpv0", "d,o(s!)", 	MATCH_LBPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lb",  		"Xpulpv0", "d,t(s)",  	MATCH_LBRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lb",  		"Xpulpv0", "d,t(s!)", 	MATCH_LBRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

{"p.lbu", 		"Xpulpv0", "d,o(s)",  	MATCH_LBU,       			MASK_LBU,     	match_opcode,	0},
{"p.lbu", 		"Xpulpv0", "d,o(s!)", 	MATCH_LBUPOST,   			MASK_LPOST,   	match_opcode,	0},
{"p.lbu", 		"Xpulpv0", "d,t(s)",  	MATCH_LBURR,     			MASK_LRR,     	match_opcode,	0},
{"p.lbu", 		"Xpulpv0", "d,t(s!)", 	MATCH_LBURRPOST, 			MASK_LRRPOST, 	match_opcode,	0},

{"p.lh",  		"Xpulpv0", "d,o(s)",  	MATCH_LH,        			MASK_LH,      	match_opcode,	0},
{"p.lh",  		"Xpulpv0", "d,o(s!)", 	MATCH_LHPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lh",  		"Xpulpv0", "d,t(s)",  	MATCH_LHRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lh",  		"Xpulpv0", "d,t(s!)", 	MATCH_LHRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

{"p.lhu", 		"Xpulpv0", "d,o(s)",  	MATCH_LHU,       			MASK_LHU,     	match_opcode,	0},
{"p.lhu", 		"Xpulpv0", "d,o(s!)", 	MATCH_LHUPOST,   			MASK_LPOST,   	match_opcode,	0},
{"p.lhu", 		"Xpulpv0", "d,t(s)",  	MATCH_LHURR,     			MASK_LRR,     	match_opcode,	0},
{"p.lhu", 		"Xpulpv0", "d,t(s!)", 	MATCH_LHURRPOST, 			MASK_LRRPOST, 	match_opcode,	0},

{"p.lw",  		"Xpulpv0", "d,o(s)",  	MATCH_LW,        			MASK_LW,      	match_opcode,	0},
{"p.lw",  		"Xpulpv0", "d,o(s!)", 	MATCH_LWPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lw",  		"Xpulpv0", "d,t(s)",  	MATCH_LWRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lw",  		"Xpulpv0", "d,t(s!)", 	MATCH_LWRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

/* post-increment and reg-reg stores */

{"p.sb",  		"Xpulpv0", "t,q(s)",  	MATCH_SB,        			MASK_SB,      	match_opcode,	0},
{"p.sb",  		"Xpulpv0", "t,q(s!)", 	MATCH_SBPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sb",  		"Xpulpv0", "t,r(s)",  	MATCH_SBRR,      			MASK_SRR,     	match_opcode,	0},
{"p.sb",  		"Xpulpv0", "t,r(s!)", 	MATCH_SBRRPOST,  			MASK_SRRPOST, 	match_opcode,	0},

{"p.sh",  		"Xpulpv0", "t,q(s)",  	MATCH_SH,        			MASK_SH,      	match_opcode,	0},
{"p.sh",  		"Xpulpv0", "t,q(s!)", 	MATCH_SHPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sh",  		"Xpulpv0", "t,r(s)",  	MATCH_SHRR,      			MASK_SRR,     	match_opcode,	0},
{"p.sh",  		"Xpulpv0", "t,r(s!)", 	MATCH_SHRRPOST,  			MASK_SRRPOST, 	match_opcode,	0},

{"p.sw",  		"Xpulpv0", "t,q(s)",  	MATCH_SW,        			MASK_SW,      	match_opcode,	0},
{"p.sw",  		"Xpulpv0", "t,q(s!)", 	MATCH_SWPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sw",  		"Xpulpv0", "t,r(s)",  	MATCH_SWRR,      			MASK_SRR,     	match_opcode,	0},
{"p.sw",  		"Xpulpv0", "t,r(s!)", 	MATCH_SWRRPOST,  			MASK_SRRPOST, 	match_opcode,	0},

/* additional ALU operations */

{"p.avg",   		"Xpulpv0", "d,s,t", 	MATCH_AVG,   				MASK_PALU,  	match_opcode,	0},
{"p.avgu",  		"Xpulpv0", "d,s,t", 	MATCH_AVGU,  				MASK_PALU,  	match_opcode,	0},
{"p.slet",  		"Xpulpv0", "d,s,t", 	MATCH_SLET,  				MASK_PALU,  	match_opcode,	0},
{"p.sletu", 		"Xpulpv0", "d,s,t", 	MATCH_SLETU, 				MASK_PALU,  	match_opcode,	0},
{"p.min",   		"Xpulpv0", "d,s,t", 	MATCH_MIN,   				MASK_PALU,  	match_opcode,	0},
{"p.minu",  		"Xpulpv0", "d,s,t", 	MATCH_MINU,  				MASK_PALU,  	match_opcode,	0},
{"p.max",   		"Xpulpv0", "d,s,t", 	MATCH_MAX,   				MASK_PALU,  	match_opcode,	0},
{"p.maxu",  		"Xpulpv0", "d,s,t", 	MATCH_MAXU,  				MASK_PALU,  	match_opcode,	0},
{"p.ror",   		"Xpulpv0", "d,s,t", 	MATCH_ROR,   				MASK_PALU,  	match_opcode,	0},

/* PLACEHOLDER: add your own instruction as part of Xplulv0 ISA

/* additional ALU operations with only a single source operand */

{"p.ff1",   		"Xpulpv0", "d,s",   	MATCH_FF1,   				MASK_PALUS, 	match_opcode, 	0},
{"p.fl1",   		"Xpulpv0", "d,s",   	MATCH_FL1,   				MASK_PALUS, 	match_opcode, 	0},
{"p.clb",   		"Xpulpv0", "d,s",   	MATCH_CLB,   				MASK_PALUS, 	match_opcode, 	0},
{"p.cnt",   		"Xpulpv0", "d,s",   	MATCH_CNT,   				MASK_PALUS, 	match_opcode, 	0},
{"p.exths", 		"Xpulpv0", "d,s",   	MATCH_EXTHS, 				MASK_PALUS, 	match_opcode, 	0},
{"p.exthz", 		"Xpulpv0", "d,s",   	MATCH_EXTHZ, 				MASK_PALUS, 	match_opcode, 	0},
{"p.extbs", 		"Xpulpv0", "d,s",   	MATCH_EXTBS,		 		MASK_PALUS, 	match_opcode, 	0},
{"p.extbz", 		"Xpulpv0", "d,s",   	MATCH_EXTBZ, 				MASK_PALUS, 	match_opcode, 	0},
{"p.abs",   		"Xpulpv0", "d,s",   	MATCH_ABS,   				MASK_PALUS, 	match_opcode, 	0},

/* hardware loops */

{"lp.starti", 		"Xpulpv0", "di,b1",    	MATCH_HWLP_STARTI, 			MASK_HWLP_STARTI,match_opcode, 	0},
{"lp.endi",   		"Xpulpv0", "di,b1",    	MATCH_HWLP_ENDI,   			MASK_HWLP_ENDI,  match_opcode, 	0},
{"lp.count",  		"Xpulpv0", "di,s",     	MATCH_HWLP_COUNT,  			MASK_HWLP_COUNT, match_opcode, 	0},
{"lp.counti", 		"Xpulpv0", "di,ji",    	MATCH_HWLP_COUNTI, 			MASK_HWLP_COUNTI,match_opcode, 	0},
{"lp.setup",  		"Xpulpv0", "di,s,b1",  	MATCH_HWLP_SETUP,  			MASK_HWLP_SETUP, match_opcode, 	0},
{"lp.setupi", 		"Xpulpv0", "di,ji,b2", 	MATCH_HWLP_SETUPI, 			MASK_HWLP_SETUPI,match_opcode, 	0},

/* 32x32 into 32 multiplication */

{"p.mul",      		"Xpulpv0", "d,s,t",  	MATCH_MUL, 				MASK_MUL, 	match_opcode,  	0},

/* 32x32 into 32 MAC operation */

{"p.mac",       	"Xpulpv0", "d,s,t,r", 	MATCH_MAC,     				MASK_MAC, 	match_opcode, 	0},

/* 16x16 into 32 MAC operations */

{"p.mac.zl.zl", 	"Xpulpv0", "d,s,t,r", 	MATCH_MACZLZL, 				MASK_MAC, 	match_opcode, 	0},
{"p.macu",      	"Xpulpv0", "d,s,t,r", 	MATCH_MACZLZL, 				MASK_MAC, 	match_opcode, 	0},
{"p.mac.zl.zh", 	"Xpulpv0", "d,s,t,r", 	MATCH_MACZLZH, 				MASK_MAC, 	match_opcode, 	0},
{"p.mac.zh.zl", 	"Xpulpv0", "d,s,t,r", 	MATCH_MACZHZL, 				MASK_MAC, 	match_opcode, 	0},
{"p.machlu",    	"Xpulpv0", "d,s,t,r", 	MATCH_MACZHZL, 				MASK_MAC, 	match_opcode, 	0},
{"p.mac.zh.zh", 	"Xpulpv0", "d,s,t,r", 	MATCH_MACZHZH, 				MASK_MAC, 	match_opcode, 	0},
{"p.machhu",    	"Xpulpv0", "d,s,t,r", 	MATCH_MACZHZH, 				MASK_MAC, 	match_opcode, 	0},

{"p.mac.zl.sl", 	"Xpulpv0", "d,s,t,r", 	MATCH_MACZLSL, 				MASK_MAC, 	match_opcode, 	0},
{"p.mac.zl.sh", 	"Xpulpv0", "d,s,t,r", 	MATCH_MACZLSH, 				MASK_MAC, 	match_opcode, 	0},
{"p.mac.zh.sl", 	"Xpulpv0", "d,s,t,r", 	MATCH_MACZHSL, 				MASK_MAC, 	match_opcode, 	0},
{"p.mac.zh.sh", 	"Xpulpv0", "d,s,t,r", 	MATCH_MACZHSH, 				MASK_MAC, 	match_opcode, 	0},

{"p.mac.sl.zl", 	"Xpulpv0", "d,s,t,r", 	MATCH_MACSLZL, 				MASK_MAC, 	match_opcode, 	0},
{"p.mac.sl.zh", 	"Xpulpv0", "d,s,t,r", 	MATCH_MACSLZH, 				MASK_MAC, 	match_opcode, 	0},
{"p.mac.sh.zl", 	"Xpulpv0", "d,s,t,r", 	MATCH_MACSHZL, 				MASK_MAC, 	match_opcode, 	0},
{"p.machlsu",   	"Xpulpv0", "d,s,t,r", 	MATCH_MACSHZL, 				MASK_MAC, 	match_opcode, 	0},
{"p.mac.sh.zh", 	"Xpulpv0", "d,s,t,r", 	MATCH_MACSHZH, 				MASK_MAC, 	match_opcode, 	0},

{"p.mac.sl.sl", 	"Xpulpv0", "d,s,t,r", 	MATCH_MACSLSL, 				MASK_MAC, 	match_opcode, 	0},
{"p.macs",      	"Xpulpv0", "d,s,t,r", 	MATCH_MACSLSL, 				MASK_MAC, 	match_opcode, 	0},
{"p.mac.sl.sh", 	"Xpulpv0", "d,s,t,r", 	MATCH_MACSLSH, 				MASK_MAC, 	match_opcode, 	0},
{"p.mac.sh.sl", 	"Xpulpv0", "d,s,t,r", 	MATCH_MACSHSL, 				MASK_MAC, 	match_opcode, 	0},
{"p.machls",    	"Xpulpv0", "d,s,t,r", 	MATCH_MACSHSL, 				MASK_MAC, 	match_opcode, 	0},
{"p.mac.sh.sh", 	"Xpulpv0", "d,s,t,r", 	MATCH_MACSHSH, 				MASK_MAC, 	match_opcode, 	0},
{"p.machhs",    	"Xpulpv0", "d,s,t,r", 	MATCH_MACSHSH, 				MASK_MAC, 	match_opcode, 	0},

/* Pulp v1 */
/* post-increment and register-register loads */

{"p.lb",  		"Xpulpv1", "d,o(s)",  	MATCH_LB,        			MASK_LB,      	match_opcode,	0},
{"p.lb",  		"Xpulpv1", "d,o(s!)", 	MATCH_LBPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lb",  		"Xpulpv1", "d,t(s)",  	MATCH_LBRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lb",  		"Xpulpv1", "d,t(s!)", 	MATCH_LBRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

{"p.lbu", 		"Xpulpv1", "d,o(s)",  	MATCH_LBU,       			MASK_LBU,     	match_opcode,	0},
{"p.lbu", 		"Xpulpv1", "d,o(s!)", 	MATCH_LBUPOST,   			MASK_LPOST,   	match_opcode,	0},
{"p.lbu", 		"Xpulpv1", "d,t(s)",  	MATCH_LBURR,     			MASK_LRR,     	match_opcode,	0},
{"p.lbu", 		"Xpulpv1", "d,t(s!)", 	MATCH_LBURRPOST, 			MASK_LRRPOST, 	match_opcode,	0},

{"p.lh",  		"Xpulpv1", "d,o(s)",  	MATCH_LH,        			MASK_LH,      	match_opcode,	0},
{"p.lh",  		"Xpulpv1", "d,o(s!)", 	MATCH_LHPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lh",  		"Xpulpv1", "d,t(s)",  	MATCH_LHRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lh",  		"Xpulpv1", "d,t(s!)", 	MATCH_LHRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

{"p.lhu", 		"Xpulpv1", "d,o(s)",  	MATCH_LHU,       			MASK_LHU,     	match_opcode,	0},
{"p.lhu", 		"Xpulpv1", "d,o(s!)", 	MATCH_LHUPOST,   			MASK_LPOST,   	match_opcode,	0},
{"p.lhu", 		"Xpulpv1", "d,t(s)",  	MATCH_LHURR,     			MASK_LRR,     	match_opcode,	0},
{"p.lhu", 		"Xpulpv1", "d,t(s!)", 	MATCH_LHURRPOST, 			MASK_LRRPOST, 	match_opcode,	0},

{"p.lw",  		"Xpulpv1", "d,o(s)",  	MATCH_LW,        			MASK_LW,      	match_opcode,	0},
{"p.lw",  		"Xpulpv1", "d,o(s!)", 	MATCH_LWPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lw",  		"Xpulpv1", "d,t(s)",  	MATCH_LWRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lw",  		"Xpulpv1", "d,t(s!)", 	MATCH_LWRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

/* post-increment and reg-reg stores */

{"p.sb",  		"Xpulpv1", "t,q(s)",  	MATCH_SB,        			MASK_SB,      	match_opcode,	0},
{"p.sb",  		"Xpulpv1", "t,q(s!)", 	MATCH_SBPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sb",  		"Xpulpv1", "t,r(s)",  	MATCH_SBRR,      			MASK_SRR,     	match_opcode,	0},
{"p.sb",  		"Xpulpv1", "t,r(s!)", 	MATCH_SBRRPOST,  			MASK_SRRPOST, 	match_opcode,	0},

{"p.sh",  		"Xpulpv1", "t,q(s)",  	MATCH_SH,        			MASK_SH,      	match_opcode,	0},
{"p.sh",  		"Xpulpv1", "t,q(s!)", 	MATCH_SHPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sh",  		"Xpulpv1", "t,r(s)",  	MATCH_SHRR,      			MASK_SRR,     	match_opcode,	0},
{"p.sh",  		"Xpulpv1", "t,r(s!)", 	MATCH_SHRRPOST,  			MASK_SRRPOST, 	match_opcode,	0},

{"p.sw",  		"Xpulpv1", "t,q(s)",  	MATCH_SW,        			MASK_SW,      	match_opcode,	0},
{"p.sw",  		"Xpulpv1", "t,q(s!)", 	MATCH_SWPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sw",  		"Xpulpv1", "t,r(s)",  	MATCH_SWRR,      			MASK_SRR,     	match_opcode,	0},
{"p.sw",  		"Xpulpv1", "t,r(s!)", 	MATCH_SWRRPOST,  			MASK_SRRPOST, 	match_opcode,	0},

/* additional ALU operations */

{"p.avg",   		"Xpulpv1", "d,s,t", 	MATCH_AVG,   				MASK_PALU,  	match_opcode,	0},
{"p.avgu",  		"Xpulpv1", "d,s,t", 	MATCH_AVGU,  				MASK_PALU,  	match_opcode,	0},
{"p.slet",  		"Xpulpv1", "d,s,t", 	MATCH_SLET,  				MASK_PALU,  	match_opcode,	0},
{"p.sletu", 		"Xpulpv1", "d,s,t", 	MATCH_SLETU, 				MASK_PALU,  	match_opcode,	0},
{"p.min",   		"Xpulpv1", "d,s,t", 	MATCH_MIN,   				MASK_PALU,  	match_opcode,	0},
{"p.minu",  		"Xpulpv1", "d,s,t", 	MATCH_MINU,  				MASK_PALU,  	match_opcode,	0},
{"p.max",   		"Xpulpv1", "d,s,t", 	MATCH_MAX,   				MASK_PALU,  	match_opcode,	0},
{"p.maxu",  		"Xpulpv1", "d,s,t", 	MATCH_MAXU,  				MASK_PALU,  	match_opcode,	0},
{"p.ror",   		"Xpulpv1", "d,s,t", 	MATCH_ROR,   				MASK_PALU,  	match_opcode,	0},

/* PLACEHOLDER: add your own instruction as part of Xplulv0 ISA 
/* additional ALU operations with only a single source operand */

{"p.ff1",   		"Xpulpv1", "d,s",   	MATCH_FF1,   				MASK_PALUS, 	match_opcode, 	0},
{"p.fl1",   		"Xpulpv1", "d,s",   	MATCH_FL1,   				MASK_PALUS, 	match_opcode, 	0},
{"p.clb",   		"Xpulpv1", "d,s",   	MATCH_CLB,   				MASK_PALUS, 	match_opcode, 	0},
{"p.cnt",   		"Xpulpv1", "d,s",   	MATCH_CNT,   				MASK_PALUS, 	match_opcode, 	0},
{"p.exths", 		"Xpulpv1", "d,s",   	MATCH_EXTHS, 				MASK_PALUS, 	match_opcode, 	0},
{"p.exthz", 		"Xpulpv1", "d,s",   	MATCH_EXTHZ, 				MASK_PALUS, 	match_opcode, 	0},
{"p.extbs", 		"Xpulpv1", "d,s",   	MATCH_EXTBS,		 		MASK_PALUS, 	match_opcode, 	0},
{"p.extbz", 		"Xpulpv1", "d,s",   	MATCH_EXTBZ, 				MASK_PALUS, 	match_opcode, 	0},
{"p.abs",   		"Xpulpv1", "d,s",   	MATCH_ABS,   				MASK_PALUS, 	match_opcode, 	0},

/* hardware loops */

{"lp.starti", 		"Xpulpv1", "di,b1",    	MATCH_HWLP_STARTI, 			MASK_HWLP_STARTI,match_opcode, 	0},
{"lp.endi",   		"Xpulpv1", "di,b1",    	MATCH_HWLP_ENDI,   			MASK_HWLP_ENDI,  match_opcode, 	0},
{"lp.count",  		"Xpulpv1", "di,s",     	MATCH_HWLP_COUNT,  			MASK_HWLP_COUNT, match_opcode, 	0},
{"lp.counti", 		"Xpulpv1", "di,ji",    	MATCH_HWLP_COUNTI, 			MASK_HWLP_COUNTI,match_opcode, 	0},
{"lp.setup",  		"Xpulpv1", "di,s,b1",  	MATCH_HWLP_SETUP,  			MASK_HWLP_SETUP, match_opcode, 	0},
{"lp.setupi", 		"Xpulpv1", "di,ji,b2", 	MATCH_HWLP_SETUPI, 			MASK_HWLP_SETUPI,match_opcode, 	0},

/* 32x32 into 32 multiplication */

{"p.mul",      		"Xpulpv1", "d,s,t",  	MATCH_MUL, 				MASK_MUL, 	match_opcode,  	0},

/* 32x32 into 32 MAC operation */

{"p.mac",		"Xpulpv1", "d,s,t",  	MATCH_MULH, 				MASK_MULH, 	match_opcode,  	0},

/* 16x16 into 32 MAC operations */

{"p.macs",		"Xpulpv1", "d,s,t",  	MATCH_MACS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.machhs",		"Xpulpv1", "d,s,t",  	MATCH_MACHHS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.macu",		"Xpulpv1", "d,s,t",  	MATCH_MACU, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.machhu",		"Xpulpv1", "d,s,t",  	MATCH_MACHHU, 				MASK_MACMUL, 	match_opcode,  	0},

/* Pulp v2 */
/* post-increment and register-register loads */

{"p.lb",  		"Xpulpv2", "d,o(s)",  	MATCH_LB,        			MASK_LB,      	match_opcode,	0},
{"p.lb",  		"Xpulpv2", "d,o(s!)", 	MATCH_LBPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lb",  		"Xpulpv2", "d,t(s)",  	MATCH_LBRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lb",  		"Xpulpv2", "d,t(s!)", 	MATCH_LBRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

{"p.lbu", 		"Xpulpv2", "d,o(s)",  	MATCH_LBU,       			MASK_LBU,     	match_opcode,	0},
{"p.lbu", 		"Xpulpv2", "d,o(s!)", 	MATCH_LBUPOST,   			MASK_LPOST,   	match_opcode,	0},
{"p.lbu", 		"Xpulpv2", "d,t(s)",  	MATCH_LBURR,     			MASK_LRR,     	match_opcode,	0},
{"p.lbu", 		"Xpulpv2", "d,t(s!)", 	MATCH_LBURRPOST, 			MASK_LRRPOST, 	match_opcode,	0},

{"p.lh",  		"Xpulpv2", "d,o(s)",  	MATCH_LH,        			MASK_LH,      	match_opcode,	0},
{"p.lh",  		"Xpulpv2", "d,o(s!)", 	MATCH_LHPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lh",  		"Xpulpv2", "d,t(s)",  	MATCH_LHRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lh",  		"Xpulpv2", "d,t(s!)", 	MATCH_LHRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

{"p.lhu", 		"Xpulpv2", "d,o(s)",  	MATCH_LHU,       			MASK_LHU,     	match_opcode,	0},
{"p.lhu", 		"Xpulpv2", "d,o(s!)", 	MATCH_LHUPOST,   			MASK_LPOST,   	match_opcode,	0},
{"p.lhu", 		"Xpulpv2", "d,t(s)",  	MATCH_LHURR,     			MASK_LRR,     	match_opcode,	0},
{"p.lhu", 		"Xpulpv2", "d,t(s!)", 	MATCH_LHURRPOST, 			MASK_LRRPOST, 	match_opcode,	0},

{"p.lw",  		"Xpulpv2", "d,o(s)",  	MATCH_LW,        			MASK_LW,      	match_opcode,	0},
{"p.lw",  		"Xpulpv2", "d,o(s!)", 	MATCH_LWPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lw",  		"Xpulpv2", "d,t(s)",  	MATCH_LWRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lw",  		"Xpulpv2", "d,t(s!)", 	MATCH_LWRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

/* post-increment and reg-reg stores */

{"p.sb",  		"Xpulpv2", "t,q(s)",  	MATCH_SB,        			MASK_SB,      	match_opcode,	0},
{"p.sb",  		"Xpulpv2", "t,q(s!)", 	MATCH_SBPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sb",  		"Xpulpv2", "t,d(s)",  	MATCH_SBRR,      			MASK_PALU,     	match_opcode,	0},
{"p.sb",  		"Xpulpv2", "t,d(s!)", 	MATCH_SBRRPOST,  			MASK_PALU, 	match_opcode,	0},

{"p.sh",  		"Xpulpv2", "t,q(s)",  	MATCH_SH,        			MASK_SH,      	match_opcode,	0},
{"p.sh",  		"Xpulpv2", "t,q(s!)", 	MATCH_SHPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sh",  		"Xpulpv2", "t,d(s)",  	MATCH_SHRR,      			MASK_PALU,     	match_opcode,	0},
{"p.sh",  		"Xpulpv2", "t,d(s!)", 	MATCH_SHRRPOST,  			MASK_PALU, 	match_opcode,	0},

{"p.sw",  		"Xpulpv2", "t,q(s)",  	MATCH_SW,        			MASK_SW,      	match_opcode,	0},
{"p.sw",  		"Xpulpv2", "t,q(s!)", 	MATCH_SWPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sw",  		"Xpulpv2", "t,d(s)",  	MATCH_SWRR,      			MASK_PALU,     	match_opcode,	0},
{"p.sw",  		"Xpulpv2", "t,d(s!)", 	MATCH_SWRRPOST,  			MASK_PALU, 	match_opcode,	0},

/* additional ALU operations */

{"p.abs",   		"Xpulpv2", "d,s", 	MATCH_AVG,   				MASK_PALUS,  	match_opcode,	0},
{"p.slet",  		"Xpulpv2", "d,s,t", 	MATCH_SLET,  				MASK_PALU,  	match_opcode,	0},
{"p.sletu", 		"Xpulpv2", "d,s,t", 	MATCH_SLETU, 				MASK_PALU,  	match_opcode,	0},
{"p.min",   		"Xpulpv2", "d,s,t", 	MATCH_MIN,   				MASK_PALU,  	match_opcode,	0},
{"p.minu",  		"Xpulpv2", "d,s,t", 	MATCH_MINU,  				MASK_PALU,  	match_opcode,	0},
{"p.max",   		"Xpulpv2", "d,s,t", 	MATCH_MAX,   				MASK_PALU,  	match_opcode,	0},
{"p.maxu",  		"Xpulpv2", "d,s,t", 	MATCH_MAXU,  				MASK_PALU,  	match_opcode,	0},
{"p.ror",   		"Xpulpv2", "d,s,t", 	MATCH_ROR,   				MASK_PALU,  	match_opcode,	0},

/* PLACEHOLDER: add your own instruction as part of Xplulv0 ISA 
/* additional ALU operations with only a single source operand */

{"p.ff1",   		"Xpulpv2", "d,s",   	MATCH_FF1,   				MASK_PALUS, 	match_opcode, 	0},
{"p.fl1",   		"Xpulpv2", "d,s",   	MATCH_FL1,   				MASK_PALUS, 	match_opcode, 	0},
{"p.clb",   		"Xpulpv2", "d,s",   	MATCH_CLB,   				MASK_PALUS, 	match_opcode, 	0},
{"p.cnt",   		"Xpulpv2", "d,s",   	MATCH_CNT,   				MASK_PALUS, 	match_opcode, 	0},
{"p.exths", 		"Xpulpv2", "d,s",   	MATCH_EXTHS, 				MASK_PALUS, 	match_opcode, 	0},
{"p.exthz", 		"Xpulpv2", "d,s",   	MATCH_EXTHZ, 				MASK_PALUS, 	match_opcode, 	0},
{"p.extbs", 		"Xpulpv2", "d,s",   	MATCH_EXTBS,		 		MASK_PALUS, 	match_opcode, 	0},
{"p.extbz", 		"Xpulpv2", "d,s",   	MATCH_EXTBZ, 				MASK_PALUS, 	match_opcode, 	0},

/* clip and bit manipulation */


{"p.clip",   		"Xpulpv2", "d,s,bi", 	MATCH_CLIP,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipi",   		"Xpulpv2", "d,s,bi", 	MATCH_CLIP,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipr",   		"Xpulpv2", "d,s,t", 	MATCH_CLIPR,   				MASK_PALU,  	match_opcode,	0},
{"p.clipu",   		"Xpulpv2", "d,s,bi", 	MATCH_CLIPU,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipui",   		"Xpulpv2", "d,s,bi", 	MATCH_CLIPU,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipur",   		"Xpulpv2", "d,s,t", 	MATCH_CLIPUR,  				MASK_PALU,  	match_opcode,	0},

{"p.extract",  		"Xpulpv2", "d,s,b5,bi",	MATCH_EXTRACT, 				MASK_PALU2,  	match_opcode,	0},
{"p.extracti", 		"Xpulpv2", "d,s,b5,bi",	MATCH_EXTRACT, 				MASK_PALU2,  	match_opcode,	0},
{"p.extractr", 		"Xpulpv2", "d,s,t",	MATCH_EXTRACTR,				MASK_PALU,  	match_opcode,	0},
{"p.extractu", 		"Xpulpv2", "d,s,b5,bi",	MATCH_EXTRACTU, 			MASK_PALU2,  	match_opcode,	0},
{"p.extractui", 	"Xpulpv2", "d,s,b5,bi",	MATCH_EXTRACTU, 			MASK_PALU2,  	match_opcode,	0},
{"p.extractur", 	"Xpulpv2", "d,s,t",	MATCH_EXTRACTUR, 			MASK_PALU,  	match_opcode,	0},
{"p.insert",  		"Xpulpv2", "d,s,b5,bi",	MATCH_INSERT, 				MASK_PALU2,  	match_opcode,	0},
{"p.inserti",  		"Xpulpv2", "d,s,b5,bi",	MATCH_INSERT, 				MASK_PALU2,  	match_opcode,	0},
{"p.insertr",  		"Xpulpv2", "d,s,t",	MATCH_INSERTR, 				MASK_PALU,  	match_opcode,	0},
{"p.bset",  		"Xpulpv2", "d,s,b5,bi",	MATCH_BSET, 				MASK_PALU2,  	match_opcode,	0},
{"p.bseti",  		"Xpulpv2", "d,s,b5,bi",	MATCH_BSET, 				MASK_PALU2,  	match_opcode,	0},
{"p.bsetr",  		"Xpulpv2", "d,s,t",	MATCH_BSETR, 				MASK_PALU,  	match_opcode,	0},
{"p.bclr",  		"Xpulpv2", "d,s,b5,bi",	MATCH_BCLR, 				MASK_PALU2,  	match_opcode,	0},
{"p.bclri",  		"Xpulpv2", "d,s,b5,bi",	MATCH_BCLR, 				MASK_PALU2,  	match_opcode,	0},
{"p.bclrr",  		"Xpulpv2", "d,s,t",	MATCH_BCLRR, 				MASK_PALU,  	match_opcode,	0},

/* hardware loops */

{"lp.starti", 		"Xpulpv2", "di,b1",    	MATCH_HWLP_STARTI, 			MASK_HWLP_STARTI,match_opcode, 	0},
{"lp.endi",   		"Xpulpv2", "di,b1",    	MATCH_HWLP_ENDI,   			MASK_HWLP_ENDI,  match_opcode, 	0},
{"lp.count",  		"Xpulpv2", "di,s",     	MATCH_HWLP_COUNT,  			MASK_HWLP_COUNT, match_opcode, 	0},
{"lp.counti", 		"Xpulpv2", "di,ji",    	MATCH_HWLP_COUNTI, 			MASK_HWLP_COUNTI,match_opcode, 	0},
{"lp.setup",  		"Xpulpv2", "di,s,b1",  	MATCH_HWLP_SETUP,  			MASK_HWLP_SETUP, match_opcode, 	0},
{"lp.setupi", 		"Xpulpv2", "di,ji,b2", 	MATCH_HWLP_SETUPI, 			MASK_HWLP_SETUPI,match_opcode, 	0},

/* 32x32 into 32 multiplication */

{"p.mul",      		"Xpulpv2", "d,s,t",  	MATCH_MUL, 				MASK_MUL, 	match_opcode,  	0},

/* 32x32 into 64 support */

{"p.mulh",      	"Xpulpv2", "d,s,t",  	MATCH_MULH, 				MASK_MULH, 	match_opcode,  0},
{"p.mulhu",     	"Xpulpv2", "d,s,t",  	MATCH_MULHU, 				MASK_MULHU, 	match_opcode,  0},
{"p.mulhsu",    	"Xpulpv2", "d,s,t",  	MATCH_MULHSU, 				MASK_MULHSU, 	match_opcode,  0},

/* 32 bit div and rem */

{"p.div",      		"Xpulpv2",   "d,s,t",  	MATCH_DIV, 				MASK_DIV, 	match_opcode,  0},
{"p.divu",     		"Xpulpv2",   "d,s,t",  	MATCH_DIVU, 				MASK_DIVU, 	match_opcode,  0},
{"p.rem",      		"Xpulpv2",   "d,s,t",  	MATCH_REM, 				MASK_REM, 	match_opcode,  0},
{"p.remu",     		"Xpulpv2",   "d,s,t",  	MATCH_REMU,				MASK_REMU, 	match_opcode,  0},

/* 32x32 into 32 Mac/Msu */

{"p.mac",		"Xpulpv2", "d,s,t",  	MATCH_MAC32, 				MASK_MACMSU32, 	match_opcode,  	0},
{"p.msu",		"Xpulpv2", "d,s,t",  	MATCH_MSU32, 				MASK_MACMSU32, 	match_opcode,  	0},

/*  16x16 into 32 Mult/Mac with optional norm and rounding */

{"p.muls",		"Xpulpv2", "d,s,t",  	MATCH_MULS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.mulhhs",		"Xpulpv2", "d,s,t",  	MATCH_MULHHS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.mulu",		"Xpulpv2", "d,s,t",  	MATCH_MULU, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.mulhhu",		"Xpulpv2", "d,s,t",  	MATCH_MULHHU, 				MASK_MACMUL, 	match_opcode,  	0},

{"p.macs",		"Xpulpv2", "d,s,t",  	MATCH_MACS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.machhs",		"Xpulpv2", "d,s,t",  	MATCH_MACHHS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.macu",		"Xpulpv2", "d,s,t",  	MATCH_MACU, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.machhu",		"Xpulpv2", "d,s,t",  	MATCH_MACHHU, 				MASK_MACMUL, 	match_opcode,  	0},

{"p.mulsn",		"Xpulpv2", "d,s,t,b5", 	MATCH_MULSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhsn",		"Xpulpv2", "d,s,t,b5", 	MATCH_MULHHSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulsrn",		"Xpulpv2", "d,s,t,b5", 	MATCH_MULSRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhsrn",		"Xpulpv2", "d,s,t,b5", 	MATCH_MULHHSRN,				MASK_MACMULNR, 	match_opcode,  	0},

{"p.mulun",		"Xpulpv2", "d,s,t,b5", 	MATCH_MULUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhun",		"Xpulpv2", "d,s,t,b5", 	MATCH_MULHHUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulurn",		"Xpulpv2", "d,s,t,b5", 	MATCH_MULURN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhurn",		"Xpulpv2", "d,s,t,b5", 	MATCH_MULHHURN,				MASK_MACMULNR, 	match_opcode,  	0},

{"p.macsn",		"Xpulpv2", "d,s,t,b5", 	MATCH_MACSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhsn",		"Xpulpv2", "d,s,t,b5", 	MATCH_MACHHSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.macsrn",		"Xpulpv2", "d,s,t,b5", 	MATCH_MACSRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhsrn",		"Xpulpv2", "d,s,t,b5", 	MATCH_MACHHSRN,				MASK_MACMULNR, 	match_opcode,  	0},

{"p.macun",		"Xpulpv2", "d,s,t,b5", 	MATCH_MACUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhun",		"Xpulpv2", "d,s,t,b5", 	MATCH_MACHHUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.macurn",		"Xpulpv2", "d,s,t,b5", 	MATCH_MACURN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhurn",		"Xpulpv2", "d,s,t,b5", 	MATCH_MACHHURN,				MASK_MACMULNR, 	match_opcode,  	0},

/*  Add/Sub with norm and rounding */

{"p.addn",		"Xpulpv2", "d,s,t,b5", 	MATCH_ADDN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addni",		"Xpulpv2", "d,s,t,b5", 	MATCH_ADDN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addnr",		"Xpulpv2", "d,s,t", 	MATCH_ADDNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.addun",		"Xpulpv2", "d,s,t,b5", 	MATCH_ADDUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.adduni",		"Xpulpv2", "d,s,t,b5", 	MATCH_ADDUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addunr",		"Xpulpv2", "d,s,t", 	MATCH_ADDUNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.addrn",		"Xpulpv2", "d,s,t,b5", 	MATCH_ADDRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addrni",		"Xpulpv2", "d,s,t,b5", 	MATCH_ADDRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addrnr",		"Xpulpv2", "d,s,t", 	MATCH_ADDRNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.addurn",		"Xpulpv2", "d,s,t,b5", 	MATCH_ADDURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addurni",		"Xpulpv2", "d,s,t,b5", 	MATCH_ADDURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addurnr",		"Xpulpv2", "d,s,t", 	MATCH_ADDURNR,				MASK_PALU, 	match_opcode,  	0},

{"p.subn",		"Xpulpv2", "d,s,t,b5", 	MATCH_SUBN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subni",		"Xpulpv2", "d,s,t,b5", 	MATCH_SUBN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subnr",		"Xpulpv2", "d,s,t", 	MATCH_SUBNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.subun",		"Xpulpv2", "d,s,t,b5", 	MATCH_SUBUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subuni",		"Xpulpv2", "d,s,t,b5", 	MATCH_SUBUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subunr",		"Xpulpv2", "d,s,t", 	MATCH_SUBUNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.subrn",		"Xpulpv2", "d,s,t,b5", 	MATCH_SUBRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subrni",		"Xpulpv2", "d,s,t,b5", 	MATCH_SUBRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subrnr",		"Xpulpv2", "d,s,t", 	MATCH_SUBRNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.suburn",		"Xpulpv2", "d,s,t,b5", 	MATCH_SUBURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.suburni",		"Xpulpv2", "d,s,t,b5", 	MATCH_SUBURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.suburnr",		"Xpulpv2", "d,s,t", 	MATCH_SUBURNR,				MASK_PALU, 	match_opcode,  	0},

/* Vector Operations */

{"pv.add.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sc.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_ADD|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.add.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sc.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_ADD|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.sub.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sc.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_SUB|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.sub.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sc.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_SUB|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.avg.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sc.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_AVG|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.avg.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sc.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_AVG|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.avgu.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sci.h",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_AVGU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.avgu.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sci.b",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_AVGU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.min.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sc.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_MIN|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.min.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sc.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_MIN|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.minu.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sci.h",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_MINU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.minu.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sci.b",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_MINU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.max.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sc.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_MAX|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.max.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sc.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_MAX|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.maxu.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sci.h",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_MAXU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.maxu.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sci.b",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_MAXU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.srl.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sc.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sci.h",	"Xpulpv2", "d,s,bU",	MATCH_V_OP_SRL|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.srl.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sc.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sci.b",	"Xpulpv2", "d,s,bU",	MATCH_V_OP_SRL|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.sra.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sc.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sci.h",	"Xpulpv2", "d,s,bU",	MATCH_V_OP_SRA|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.sra.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sc.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sci.b",	"Xpulpv2", "d,s,bU",	MATCH_V_OP_SRA|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.sll.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sc.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sci.h",	"Xpulpv2", "d,s,bU",	MATCH_V_OP_SLL|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.sll.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sc.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sci.b",	"Xpulpv2", "d,s,bU",	MATCH_V_OP_SLL|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.or.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sc.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sci.h",		"Xpulpv2", "d,s,bs",	MATCH_V_OP_OR|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.or.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sc.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sci.b",		"Xpulpv2", "d,s,bs",	MATCH_V_OP_OR|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.xor.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sc.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_XOR|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.xor.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sc.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_XOR|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.and.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sc.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_AND|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.and.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sc.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_AND|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.abs.h",		"Xpulpv2", "d,s",	MATCH_V_OP_ABS|MATCH_V_OP_H_VV,		MASK_V_OP2,	match_opcode,	0},
{"pv.abs.b",		"Xpulpv2", "d,s",	MATCH_V_OP_ABS|MATCH_V_OP_B_VV,		MASK_V_OP2,	match_opcode,	0},

{"pv.extract.h",	"Xpulpv2", "d,s,bf",	MATCH_V_OP_EXTRACT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.extract.b",	"Xpulpv2", "d,s,bF",	MATCH_V_OP_EXTRACT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.extractu.h",	"Xpulpv2", "d,s,bf",	MATCH_V_OP_DOTSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.extractu.b",	"Xpulpv2", "d,s,bF",	MATCH_V_OP_DOTSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.insert.h",		"Xpulpv2", "d,s,bf",	MATCH_V_OP_SDOTUP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.insert.b",		"Xpulpv2", "d,s,bF",	MATCH_V_OP_SDOTUP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.dotsp.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_DOTUP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.dotsp.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_DOTUP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.dotup.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sci.h",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.dotup.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sci.b",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.dotusp.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_INSERT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.dotusp.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_INSERT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.sdotsp.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.sdotsp.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.sdotup.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sci.h",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_DOTUSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.sdotup.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sci.b",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_DOTUSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.sdotusp.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_SDOTSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.sdotusp.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_SDOTSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.shuffle.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shuffle.sci.h",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shuffle.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shufflei0.sci.b",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei1.sci.b",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_SHUFFLEI1|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei2.sci.b",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_SHUFFLEI2|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei3.sci.b",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_SHUFFLEI3|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.shuffle2.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_SHUFFLE2|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shuffle2.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_SHUFFLE2|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.pack.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_PACK|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.packhi.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_PACKHI|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.packlo.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_PACKLO|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.cmpeq.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_CMPEQ|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpeq.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_CMPEQ|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpne.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_CMPNE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpne.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_CMPNE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpgt.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_CMPGT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpgt.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_CMPGT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpge.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_CMPGE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpge.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_CMPGE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmplt.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_CMPLT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmplt.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_CMPLT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmple.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sci.h",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_CMPLE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmple.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sci.b",	"Xpulpv2", "d,s,bs",	MATCH_V_OP_CMPLE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpgtu.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sci.h",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_CMPGTU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpgtu.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sci.b",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_CMPGTU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpgeu.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sci.h",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_CMPGEU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpgeu.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sci.b",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_CMPGEU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpltu.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sci.h",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_CMPLTU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpltu.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sci.b",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_CMPLTU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpleu.h",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sc.h",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sci.h",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_CMPLEU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpleu.b",		"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sc.b",	"Xpulpv2", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sci.b",	"Xpulpv2", "d,s,bu",	MATCH_V_OP_CMPLEU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"p.beqimm",       	"Xpulpv2",  "s,bI,p",  	MATCH_BEQM1, 				MASK_BEQM1, 	match_opcode,   0},
{"p.bneimm",       	"Xpulpv2",  "s,bI,p",  	MATCH_BNEM1, 				MASK_BNEM1, 	match_opcode,   0},

/* Load from event unit */

{"p.elw",		"Xpulpv2", "d,o(s)",	MATCH_LWU, 				MASK_LWU, 	match_opcode,   0},

/* __GAP8 Start */

/* Gap8 */
/* post-increment and register-register loads */

{"p.lb",  		"Xgap8", "d,o(s)",  	MATCH_LB,        			MASK_LB,      	match_opcode,	0},
{"p.lb",  		"Xgap8", "d,o(s!)", 	MATCH_LBPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lb",  		"Xgap8", "d,t(s)",  	MATCH_LBRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lb",  		"Xgap8", "d,t(s!)", 	MATCH_LBRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

{"p.lbu", 		"Xgap8", "d,o(s)",  	MATCH_LBU,       			MASK_LBU,     	match_opcode,	0},
{"p.lbu", 		"Xgap8", "d,o(s!)", 	MATCH_LBUPOST,   			MASK_LPOST,   	match_opcode,	0},
{"p.lbu", 		"Xgap8", "d,t(s)",  	MATCH_LBURR,     			MASK_LRR,     	match_opcode,	0},
{"p.lbu", 		"Xgap8", "d,t(s!)", 	MATCH_LBURRPOST, 			MASK_LRRPOST, 	match_opcode,	0},

{"p.lh",  		"Xgap8", "d,o(s)",  	MATCH_LH,        			MASK_LH,      	match_opcode,	0},
{"p.lh",  		"Xgap8", "d,o(s!)", 	MATCH_LHPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lh",  		"Xgap8", "d,t(s)",  	MATCH_LHRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lh",  		"Xgap8", "d,t(s!)", 	MATCH_LHRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

{"p.lhu", 		"Xgap8", "d,o(s)",  	MATCH_LHU,       			MASK_LHU,     	match_opcode,	0},
{"p.lhu", 		"Xgap8", "d,o(s!)", 	MATCH_LHUPOST,   			MASK_LPOST,   	match_opcode,	0},
{"p.lhu", 		"Xgap8", "d,t(s)",  	MATCH_LHURR,     			MASK_LRR,     	match_opcode,	0},
{"p.lhu", 		"Xgap8", "d,t(s!)", 	MATCH_LHURRPOST, 			MASK_LRRPOST, 	match_opcode,	0},

{"p.lw",  		"Xgap8", "d,o(s)",  	MATCH_LW,        			MASK_LW,      	match_opcode,	0},
{"p.lw",  		"Xgap8", "d,o(s!)", 	MATCH_LWPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lw",  		"Xgap8", "d,t(s)",  	MATCH_LWRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lw",  		"Xgap8", "d,t(s!)", 	MATCH_LWRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

/* post-increment and reg-reg stores */

{"p.sb",  		"Xgap8", "t,q(s)",  	MATCH_SB,        			MASK_SB,      	match_opcode,	0},
{"p.sb",  		"Xgap8", "t,q(s!)", 	MATCH_SBPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sb",  		"Xgap8", "t,d(s)",  	MATCH_SBRR,      			MASK_PALU,     	match_opcode,	0},
{"p.sb",  		"Xgap8", "t,d(s!)", 	MATCH_SBRRPOST,  			MASK_PALU, 	match_opcode,	0},

{"p.sh",  		"Xgap8", "t,q(s)",  	MATCH_SH,        			MASK_SH,      	match_opcode,	0},
{"p.sh",  		"Xgap8", "t,q(s!)", 	MATCH_SHPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sh",  		"Xgap8", "t,d(s)",  	MATCH_SHRR,      			MASK_PALU,     	match_opcode,	0},
{"p.sh",  		"Xgap8", "t,d(s!)", 	MATCH_SHRRPOST,  			MASK_PALU, 	match_opcode,	0},

{"p.sw",  		"Xgap8", "t,q(s)",  	MATCH_SW,        			MASK_SW,      	match_opcode,	0},
{"p.sw",  		"Xgap8", "t,q(s!)", 	MATCH_SWPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sw",  		"Xgap8", "t,d(s)",  	MATCH_SWRR,      			MASK_PALU,     	match_opcode,	0},
{"p.sw",  		"Xgap8", "t,d(s!)", 	MATCH_SWRRPOST,  			MASK_PALU, 	match_opcode,	0},

/* additional ALU operations */

{"p.abs",   		"Xgap8", "d,s", 	MATCH_AVG,   				MASK_PALUS,  	match_opcode,	0},
{"p.slet",  		"Xgap8", "d,s,t", 	MATCH_SLET,  				MASK_PALU,  	match_opcode,	0},
{"p.sletu", 		"Xgap8", "d,s,t", 	MATCH_SLETU, 				MASK_PALU,  	match_opcode,	0},
{"p.min",   		"Xgap8", "d,s,t", 	MATCH_MIN,   				MASK_PALU,  	match_opcode,	0},
{"p.minu",  		"Xgap8", "d,s,t", 	MATCH_MINU,  				MASK_PALU,  	match_opcode,	0},
{"p.max",   		"Xgap8", "d,s,t", 	MATCH_MAX,   				MASK_PALU,  	match_opcode,	0},
{"p.maxu",  		"Xgap8", "d,s,t", 	MATCH_MAXU,  				MASK_PALU,  	match_opcode,	0},
{"p.ror",   		"Xgap8", "d,s,t", 	MATCH_ROR,   				MASK_PALU,  	match_opcode,	0},

/* PLACEHOLDER: add your own instruction as part of Xplulv0 ISA 
{"andl",        "Xgap8", "d,s,t",   MATCH_ANDL,         MASK_ANDL,    match_opcode, 0},

/* additional ALU operations with only a single source operand */

{"p.ff1",   		"Xgap8", "d,s",   	MATCH_FF1,   				MASK_PALUS, 	match_opcode, 	0},
{"p.fl1",   		"Xgap8", "d,s",   	MATCH_FL1,   				MASK_PALUS, 	match_opcode, 	0},
{"p.clb",   		"Xgap8", "d,s",   	MATCH_CLB,   				MASK_PALUS, 	match_opcode, 	0},
{"p.cnt",   		"Xgap8", "d,s",   	MATCH_CNT,   				MASK_PALUS, 	match_opcode, 	0},
{"p.exths", 		"Xgap8", "d,s",   	MATCH_EXTHS, 				MASK_PALUS, 	match_opcode, 	0},
{"p.exthz", 		"Xgap8", "d,s",   	MATCH_EXTHZ, 				MASK_PALUS, 	match_opcode, 	0},
{"p.extbs", 		"Xgap8", "d,s",   	MATCH_EXTBS,		 		MASK_PALUS, 	match_opcode, 	0},
{"p.extbz", 		"Xgap8", "d,s",   	MATCH_EXTBZ, 				MASK_PALUS, 	match_opcode, 	0},

/* clip and bit manipulation */

{"p.clip",   		"Xgap8", "d,s,bi", 	MATCH_CLIP,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipi",   		"Xgap8", "d,s,bi", 	MATCH_CLIP,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipr",   		"Xgap8", "d,s,t", 	MATCH_CLIPR,   				MASK_PALU,  	match_opcode,	0},
{"p.clipu",   		"Xgap8", "d,s,bi", 	MATCH_CLIPU,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipui",   		"Xgap8", "d,s,bi", 	MATCH_CLIPU,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipur",   		"Xgap8", "d,s,t", 	MATCH_CLIPUR,  				MASK_PALU,  	match_opcode,	0},

{"p.extract",  		"Xgap8", "d,s,b5,bi",	MATCH_EXTRACT, 				MASK_PALU2,  	match_opcode,	0},
{"p.extracti", 		"Xgap8", "d,s,b5,bi",	MATCH_EXTRACT, 				MASK_PALU2,  	match_opcode,	0},
{"p.extractr", 		"Xgap8", "d,s,t",	MATCH_EXTRACTR,				MASK_PALU,  	match_opcode,	0},
{"p.extractu", 		"Xgap8", "d,s,b5,bi",	MATCH_EXTRACTU, 			MASK_PALU2,  	match_opcode,	0},
{"p.extractui", 	"Xgap8", "d,s,b5,bi",	MATCH_EXTRACTU, 			MASK_PALU2,  	match_opcode,	0},
{"p.extractur", 	"Xgap8", "d,s,t",	MATCH_EXTRACTUR, 			MASK_PALU,  	match_opcode,	0},
{"p.insert",  		"Xgap8", "d,s,b5,bi",	MATCH_INSERT, 				MASK_PALU2,  	match_opcode,	0},
{"p.inserti",  		"Xgap8", "d,s,b5,bi",	MATCH_INSERT, 				MASK_PALU2,  	match_opcode,	0},
{"p.insertr",  		"Xgap8", "d,s,t",	MATCH_INSERTR, 				MASK_PALU,  	match_opcode,	0},
{"p.bset",  		"Xgap8", "d,s,b5,bi",	MATCH_BSET, 				MASK_PALU2,  	match_opcode,	0},
{"p.bseti",  		"Xgap8", "d,s,b5,bi",	MATCH_BSET, 				MASK_PALU2,  	match_opcode,	0},
{"p.bsetr",  		"Xgap8", "d,s,t",	MATCH_BSETR, 				MASK_PALU,  	match_opcode,	0},
{"p.bclr",  		"Xgap8", "d,s,b5,bi",	MATCH_BCLR, 				MASK_PALU2,  	match_opcode,	0},
{"p.bclri",  		"Xgap8", "d,s,b5,bi",	MATCH_BCLR, 				MASK_PALU2,  	match_opcode,	0},
{"p.bclrr",  		"Xgap8", "d,s,t",	MATCH_BCLRR, 				MASK_PALU,  	match_opcode,	0},

/* hardware loops */

{"lp.starti", 		"Xgap8", "di,b1",    	MATCH_HWLP_STARTI, 			MASK_HWLP_STARTI,match_opcode, 	0},
{"lp.endi",   		"Xgap8", "di,b1",    	MATCH_HWLP_ENDI,   			MASK_HWLP_ENDI,  match_opcode, 	0},
{"lp.count",  		"Xgap8", "di,s",     	MATCH_HWLP_COUNT,  			MASK_HWLP_COUNT, match_opcode, 	0},
{"lp.counti", 		"Xgap8", "di,ji",    	MATCH_HWLP_COUNTI, 			MASK_HWLP_COUNTI,match_opcode, 	0},
{"lp.setup",  		"Xgap8", "di,s,b1",  	MATCH_HWLP_SETUP,  			MASK_HWLP_SETUP, match_opcode, 	0},
{"lp.setupi", 		"Xgap8", "di,ji,b2", 	MATCH_HWLP_SETUPI, 			MASK_HWLP_SETUPI,match_opcode, 	0},

/* 32x32 into 32 multiplication */

{"p.mul",      		"Xgap8",   "d,s,t",  	MATCH_MUL, 				MASK_MUL, 	match_opcode,  	0},

/* 32x32 into 64 support */

{"p.mulh",      	"Xgap8", "d,s,t",  	MATCH_MULH, 				MASK_MULH, 	match_opcode,  0},
{"p.mulhu",     	"Xgap8", "d,s,t",  	MATCH_MULHU, 				MASK_MULHU, 	match_opcode,  0},
{"p.mulhsu",    	"Xgap8", "d,s,t",  	MATCH_MULHSU, 				MASK_MULHSU, 	match_opcode,  0},

/* 32 bit div and rem */

{"p.div",       	"Xgap8",   "d,s,t",  	MATCH_DIV, 				MASK_DIV, 	match_opcode,  0},
{"p.divu",      	"Xgap8",   "d,s,t",  	MATCH_DIVU, 				MASK_DIVU, 	match_opcode,  0},
{"p.rem",       	"Xgap8",   "d,s,t",  	MATCH_REM, 				MASK_REM, 	match_opcode,  0},
{"p.remu",      	"Xgap8",   "d,s,t",  	MATCH_REMU,				MASK_REMU, 	match_opcode,  0},

/* 32x32 into 32 Mac/Msu */

{"p.mac",		"Xgap8",   "d,s,t",  	MATCH_MAC32, 				MASK_MACMSU32, 	match_opcode,  	0},
{"p.msu",		"Xgap8",   "d,s,t",  	MATCH_MSU32, 				MASK_MACMSU32, 	match_opcode,  	0},

/*  16x16 into 32 Mult/Mac with optional norm and rounding */

{"p.muls",		"Xgap8", "d,s,t",  	MATCH_MULS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.mulhhs",		"Xgap8", "d,s,t",  	MATCH_MULHHS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.mulu",		"Xgap8", "d,s,t",  	MATCH_MULU, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.mulhhu",		"Xgap8", "d,s,t",  	MATCH_MULHHU, 				MASK_MACMUL, 	match_opcode,  	0},

{"p.macs",		"Xgap8", "d,s,t",  	MATCH_MACS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.machhs",		"Xgap8", "d,s,t",  	MATCH_MACHHS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.macu",		"Xgap8", "d,s,t",  	MATCH_MACU, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.machhu",		"Xgap8", "d,s,t",  	MATCH_MACHHU, 				MASK_MACMUL, 	match_opcode,  	0},

{"p.mulsn",		"Xgap8", "d,s,t,b5", 	MATCH_MULSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhsn",		"Xgap8", "d,s,t,b5", 	MATCH_MULHHSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulsrn",		"Xgap8", "d,s,t,b5", 	MATCH_MULSRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhsrn",		"Xgap8", "d,s,t,b5", 	MATCH_MULHHSRN,				MASK_MACMULNR, 	match_opcode,  	0},

{"p.mulun",		"Xgap8", "d,s,t,b5", 	MATCH_MULUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhun",		"Xgap8", "d,s,t,b5", 	MATCH_MULHHUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulurn",		"Xgap8", "d,s,t,b5", 	MATCH_MULURN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhurn",		"Xgap8", "d,s,t,b5", 	MATCH_MULHHURN,				MASK_MACMULNR, 	match_opcode,  	0},

{"p.macsn",		"Xgap8", "d,s,t,b5", 	MATCH_MACSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhsn",		"Xgap8", "d,s,t,b5", 	MATCH_MACHHSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.macsrn",		"Xgap8", "d,s,t,b5", 	MATCH_MACSRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhsrn",		"Xgap8", "d,s,t,b5", 	MATCH_MACHHSRN,				MASK_MACMULNR, 	match_opcode,  	0},

{"p.macun",		"Xgap8", "d,s,t,b5", 	MATCH_MACUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhun",		"Xgap8", "d,s,t,b5", 	MATCH_MACHHUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.macurn",		"Xgap8", "d,s,t,b5", 	MATCH_MACURN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhurn",		"Xgap8", "d,s,t,b5", 	MATCH_MACHHURN,				MASK_MACMULNR, 	match_opcode,  	0},

/*  Add/Sub with norm and rounding */

{"p.addn",		"Xgap8", "d,s,t,b5", 	MATCH_ADDN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addni",		"Xgap8", "d,s,t,b5", 	MATCH_ADDN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addnr",		"Xgap8", "d,s,t", 	MATCH_ADDNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.addun",		"Xgap8", "d,s,t,b5", 	MATCH_ADDUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.adduni",		"Xgap8", "d,s,t,b5", 	MATCH_ADDUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addunr",		"Xgap8", "d,s,t", 	MATCH_ADDUNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.addrn",		"Xgap8", "d,s,t,b5", 	MATCH_ADDRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addrni",		"Xgap8", "d,s,t,b5", 	MATCH_ADDRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addrnr",		"Xgap8", "d,s,t", 	MATCH_ADDRNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.addurn",		"Xgap8", "d,s,t,b5", 	MATCH_ADDURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addurni",		"Xgap8", "d,s,t,b5", 	MATCH_ADDURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addurnr",		"Xgap8", "d,s,t", 	MATCH_ADDURNR,				MASK_PALU, 	match_opcode,  	0},

{"p.subn",		"Xgap8", "d,s,t,b5", 	MATCH_SUBN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subni",		"Xgap8", "d,s,t,b5", 	MATCH_SUBN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subnr",		"Xgap8", "d,s,t", 	MATCH_SUBNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.subun",		"Xgap8", "d,s,t,b5", 	MATCH_SUBUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subuni",		"Xgap8", "d,s,t,b5", 	MATCH_SUBUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subunr",		"Xgap8", "d,s,t", 	MATCH_SUBUNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.subrn",		"Xgap8", "d,s,t,b5", 	MATCH_SUBRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subrni",		"Xgap8", "d,s,t,b5", 	MATCH_SUBRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subrnr",		"Xgap8", "d,s,t", 	MATCH_SUBRNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.suburn",		"Xgap8", "d,s,t,b5", 	MATCH_SUBURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.suburni",		"Xgap8", "d,s,t,b5", 	MATCH_SUBURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.suburnr",		"Xgap8", "d,s,t", 	MATCH_SUBURNR,				MASK_PALU, 	match_opcode,  	0},

/* Vector Operations */

{"pv.add.h",		"Xgap8", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sc.h",		"Xgap8", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_ADD|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.add.b",		"Xgap8", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sc.b",		"Xgap8", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_ADD|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.sub.h",		"Xgap8", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sc.h",		"Xgap8", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_SUB|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.sub.b",		"Xgap8", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sc.b",		"Xgap8", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_SUB|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.avg.h",		"Xgap8", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sc.h",		"Xgap8", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_AVG|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.avg.b",		"Xgap8", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sc.b",		"Xgap8", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_AVG|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.avgu.h",		"Xgap8", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sci.h",	"Xgap8", "d,s,bu",	MATCH_V_OP_AVGU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.avgu.b",		"Xgap8", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sci.b",	"Xgap8", "d,s,bu",	MATCH_V_OP_AVGU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.min.h",		"Xgap8", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sc.h",		"Xgap8", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_MIN|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.min.b",		"Xgap8", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sc.b",		"Xgap8", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_MIN|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.minu.h",		"Xgap8", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sci.h",	"Xgap8", "d,s,bu",	MATCH_V_OP_MINU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.minu.b",		"Xgap8", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sci.b",	"Xgap8", "d,s,bu",	MATCH_V_OP_MINU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.max.h",		"Xgap8", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sc.h",		"Xgap8", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_MAX|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.max.b",		"Xgap8", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sc.b",		"Xgap8", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_MAX|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.maxu.h",		"Xgap8", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sci.h",	"Xgap8", "d,s,bu",	MATCH_V_OP_MAXU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.maxu.b",		"Xgap8", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sci.b",	"Xgap8", "d,s,bu",	MATCH_V_OP_MAXU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.srl.h",		"Xgap8", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sc.h",		"Xgap8", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sci.h",	"Xgap8", "d,s,bU",	MATCH_V_OP_SRL|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.srl.b",		"Xgap8", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sc.b",		"Xgap8", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sci.b",	"Xgap8", "d,s,bU",	MATCH_V_OP_SRL|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.sra.h",		"Xgap8", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sc.h",		"Xgap8", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sci.h",	"Xgap8", "d,s,bU",	MATCH_V_OP_SRA|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.sra.b",		"Xgap8", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sc.b",		"Xgap8", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sci.b",	"Xgap8", "d,s,bU",	MATCH_V_OP_SRA|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.sll.h",		"Xgap8", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sc.h",		"Xgap8", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sci.h",	"Xgap8", "d,s,bU",	MATCH_V_OP_SLL|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.sll.b",		"Xgap8", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sc.b",		"Xgap8", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sci.b",	"Xgap8", "d,s,bU",	MATCH_V_OP_SLL|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.or.h",		"Xgap8", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sc.h",		"Xgap8", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sci.h",		"Xgap8", "d,s,bs",	MATCH_V_OP_OR|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.or.b",		"Xgap8", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sc.b",		"Xgap8", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sci.b",		"Xgap8", "d,s,bs",	MATCH_V_OP_OR|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.xor.h",		"Xgap8", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sc.h",		"Xgap8", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_XOR|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.xor.b",		"Xgap8", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sc.b",		"Xgap8", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_XOR|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.and.h",		"Xgap8", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sc.h",		"Xgap8", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_AND|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.and.b",		"Xgap8", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sc.b",		"Xgap8", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_AND|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.abs.h",		"Xgap8", "d,s",		MATCH_V_OP_ABS|MATCH_V_OP_H_VV,		MASK_V_OP2,	match_opcode,	0},
{"pv.abs.b",		"Xgap8", "d,s",		MATCH_V_OP_ABS|MATCH_V_OP_B_VV,		MASK_V_OP2,	match_opcode,	0},

{"pv.extract.h",	"Xgap8", "d,s,bf",	MATCH_V_OP_EXTRACT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.extract.b",	"Xgap8", "d,s,bF",	MATCH_V_OP_EXTRACT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.extractu.h",	"Xgap8", "d,s,bf",	MATCH_V_OP_DOTSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.extractu.b",	"Xgap8", "d,s,bF",	MATCH_V_OP_DOTSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.insert.h",		"Xgap8", "d,s,bf",	MATCH_V_OP_SDOTUP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.insert.b",		"Xgap8", "d,s,bF",	MATCH_V_OP_SDOTUP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.dotsp.h",		"Xgap8", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_DOTUP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.dotsp.b",		"Xgap8", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_DOTUP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.dotup.h",		"Xgap8", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sci.h",	"Xgap8", "d,s,bu",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.dotup.b",		"Xgap8", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sci.b",	"Xgap8", "d,s,bu",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.dotusp.h",		"Xgap8", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_INSERT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.dotusp.b",		"Xgap8", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_INSERT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.sdotsp.h",		"Xgap8", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.sdotsp.b",		"Xgap8", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.sdotup.h",		"Xgap8", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sci.h",	"Xgap8", "d,s,bu",	MATCH_V_OP_DOTUSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.sdotup.b",		"Xgap8", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sci.b",	"Xgap8", "d,s,bu",	MATCH_V_OP_DOTUSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.sdotusp.h",	"Xgap8", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_SDOTSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.sdotusp.b",	"Xgap8", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_SDOTSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.shuffle.h",	"Xgap8", "d,s,t",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shuffle.sci.h",	"Xgap8", "d,s,bu",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shuffle.b",	"Xgap8", "d,s,t",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shufflei0.sci.b",	"Xgap8", "d,s,bu",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei1.sci.b",	"Xgap8", "d,s,bu",	MATCH_V_OP_SHUFFLEI1|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei2.sci.b",	"Xgap8", "d,s,bu",	MATCH_V_OP_SHUFFLEI2|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei3.sci.b",	"Xgap8", "d,s,bu",	MATCH_V_OP_SHUFFLEI3|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.shuffle2.h",	"Xgap8", "d,s,t",	MATCH_V_OP_SHUFFLE2|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shuffle2.b",	"Xgap8", "d,s,t",	MATCH_V_OP_SHUFFLE2|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.pack.h",		"Xgap8", "d,s,t",	MATCH_V_OP_PACK|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.packhi.b",		"Xgap8", "d,s,t",	MATCH_V_OP_PACKHI|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.packlo.b",		"Xgap8", "d,s,t",	MATCH_V_OP_PACKLO|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.cmpeq.h",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_CMPEQ|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpeq.b",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_CMPEQ|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpne.h",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_CMPNE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpne.b",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_CMPNE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpgt.h",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_CMPGT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpgt.b",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_CMPGT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpge.h",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_CMPGE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpge.b",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_CMPGE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmplt.h",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_CMPLT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmplt.b",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_CMPLT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmple.h",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sci.h",	"Xgap8", "d,s,bs",	MATCH_V_OP_CMPLE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmple.b",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sci.b",	"Xgap8", "d,s,bs",	MATCH_V_OP_CMPLE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpgtu.h",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sci.h",	"Xgap8", "d,s,bu",	MATCH_V_OP_CMPGTU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpgtu.b",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sci.b",	"Xgap8", "d,s,bu",	MATCH_V_OP_CMPGTU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpgeu.h",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sci.h",	"Xgap8", "d,s,bu",	MATCH_V_OP_CMPGEU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpgeu.b",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sci.b",	"Xgap8", "d,s,bu",	MATCH_V_OP_CMPGEU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpltu.h",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sci.h",	"Xgap8", "d,s,bu",	MATCH_V_OP_CMPLTU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpltu.b",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sci.b",	"Xgap8", "d,s,bu",	MATCH_V_OP_CMPLTU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpleu.h",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sc.h",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sci.h",	"Xgap8", "d,s,bu",	MATCH_V_OP_CMPLEU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpleu.b",		"Xgap8", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sc.b",	"Xgap8", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sci.b",	"Xgap8", "d,s,bu",	MATCH_V_OP_CMPLEU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"p.beqimm",       	"Xgap8",  "s,bI,p",  	MATCH_BEQM1, 				MASK_BEQM1, 	match_opcode,   0},
{"p.bneimm",       	"Xgap8",  "s,bI,p",  	MATCH_BNEM1, 				MASK_BNEM1, 	match_opcode,   0},

/* Load from event unit */

{"p.elw",		"Xgap8",  "d,o(s)",	MATCH_LWU, 				MASK_LWU, 	match_opcode,   0},

/* Vector, Gap8 only */

{"pv.pack.h.h",		"Xgap8",   "d,s,t",	MATCH_V_OP_PACK|MATCH_V_OP_H_VI,	MASK_V_OP,	match_opcode,	0},
{"pv.pack.l.h",		"Xgap8",   "d,s,t",	MATCH_V_OP_PACK|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},

{"pv.cplxmul.s",	"Xgap8",   "d,s,t",	MATCH_V_OP_CPLXMUL|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cplxmul.sc.s",	"Xgap8",   "d,s,t",	MATCH_V_OP_CPLXMUL|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cplxmul.sci.s",	"Xgap8",   "d,s,bs",	MATCH_V_OP_CPLXMUL|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cplxmul.s.div2",	"Xgap8",   "d,s,t",	MATCH_V_OP_CPLXMUL|MATCH_V_OP_H_VV_PS1,	MASK_V_OP,	match_opcode,	0},
{"pv.cplxmul.s.div4",	"Xgap8",   "d,s,t",	MATCH_V_OP_CPLXMUL|MATCH_V_OP_H_VV_PS2,	MASK_V_OP,	match_opcode,	0},

{"pv.subrotmj.h",	"Xgap8",   "d,s,t",	MATCH_V_OP_SUBROTMJ|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.subrotmj.h.div2",	"Xgap8",   "d,s,t",	MATCH_V_OP_SUBROTMJ|MATCH_V_OP_H_VV_PS1,MASK_V_OP,	match_opcode,	0},
{"pv.subrotmj.h.div4",	"Xgap8",   "d,s,t",	MATCH_V_OP_SUBROTMJ|MATCH_V_OP_H_VV_PS2,MASK_V_OP,	match_opcode,	0},

{"pv.add.b.div2",	"Xgap8",   "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_B_VV_PS1,	MASK_V_OP,	match_opcode,	0},
{"pv.add.h.div2",	"Xgap8",   "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_H_VV_PS1,	MASK_V_OP,	match_opcode,	0},
{"pv.add.b.div4",	"Xgap8",   "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_B_VV_PS2,	MASK_V_OP,	match_opcode,	0},
{"pv.add.h.div4",	"Xgap8",   "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_H_VV_PS2,	MASK_V_OP,	match_opcode,	0},

{"pv.sub.b.div2",	"Xgap8",   "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_B_VV_PS1,	MASK_V_OP,	match_opcode,	0},
{"pv.sub.h.div2",	"Xgap8",   "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_H_VV_PS1,	MASK_V_OP,	match_opcode,	0},
{"pv.sub.b.div4",	"Xgap8",   "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_B_VV_PS2,	MASK_V_OP,	match_opcode,	0},
{"pv.sub.h.div4",	"Xgap8",   "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_H_VV_PS2,	MASK_V_OP,	match_opcode,	0},

{"pv.vitop.max",	"Xgap8",   "d,s,t",	MATCH_V_OP_VITOP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.vitop.sel",	"Xgap8",   "d,s,t",	MATCH_V_OP_VITOP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.cplxconj.h",	"Xgap8",   "d,s",	MATCH_V_OP_CPLXCONJ|MATCH_V_OP_H_VV,	MASK_V_OP2,	match_opcode,	0},

/* __GAP8 Stop */

/* Pulp v3 */
/* post-increment and register-register loads */

{"p.lb",  		"Xpulpv3", "d,o(s)",  	MATCH_LB,        			MASK_LB,      	match_opcode,	0},
{"p.lb",  		"Xpulpv3", "d,o(s!)", 	MATCH_LBPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lb",  		"Xpulpv3", "d,t(s)",  	MATCH_LBRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lb",  		"Xpulpv3", "d,t(s!)", 	MATCH_LBRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

{"p.lbu", 		"Xpulpv3", "d,o(s)",  	MATCH_LBU,       			MASK_LBU,     	match_opcode,	0},
{"p.lbu", 		"Xpulpv3", "d,o(s!)", 	MATCH_LBUPOST,   			MASK_LPOST,   	match_opcode,	0},
{"p.lbu", 		"Xpulpv3", "d,t(s)",  	MATCH_LBURR,     			MASK_LRR,     	match_opcode,	0},
{"p.lbu", 		"Xpulpv3", "d,t(s!)", 	MATCH_LBURRPOST, 			MASK_LRRPOST, 	match_opcode,	0},

{"p.lh",  		"Xpulpv3", "d,o(s)",  	MATCH_LH,        			MASK_LH,      	match_opcode,	0},
{"p.lh",  		"Xpulpv3", "d,o(s!)", 	MATCH_LHPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lh",  		"Xpulpv3", "d,t(s)",  	MATCH_LHRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lh",  		"Xpulpv3", "d,t(s!)", 	MATCH_LHRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

{"p.lhu", 		"Xpulpv3", "d,o(s)",  	MATCH_LHU,       			MASK_LHU,     	match_opcode,	0},
{"p.lhu", 		"Xpulpv3", "d,o(s!)", 	MATCH_LHUPOST,   			MASK_LPOST,   	match_opcode,	0},
{"p.lhu", 		"Xpulpv3", "d,t(s)",  	MATCH_LHURR,     			MASK_LRR,     	match_opcode,	0},
{"p.lhu", 		"Xpulpv3", "d,t(s!)", 	MATCH_LHURRPOST, 			MASK_LRRPOST, 	match_opcode,	0},

{"p.lw",  		"Xpulpv3", "d,o(s)",  	MATCH_LW,        			MASK_LW,      	match_opcode,	0},
{"p.lw",  		"Xpulpv3", "d,o(s!)", 	MATCH_LWPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lw",  		"Xpulpv3", "d,t(s)",  	MATCH_LWRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lw",  		"Xpulpv3", "d,t(s!)", 	MATCH_LWRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

/* post-increment and reg-reg stores */

{"p.sb",  		"Xpulpv3", "t,q(s)",  	MATCH_SB,        			MASK_SB,      	match_opcode,	0},
{"p.sb",  		"Xpulpv3", "t,q(s!)", 	MATCH_SBPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sb",  		"Xpulpv3", "t,d(s)",  	MATCH_SBRR,      			MASK_PALU,     	match_opcode,	0},
{"p.sb",  		"Xpulpv3", "t,d(s!)", 	MATCH_SBRRPOST,  			MASK_PALU, 	match_opcode,	0},

{"p.sh",  		"Xpulpv3", "t,q(s)",  	MATCH_SH,        			MASK_SH,      	match_opcode,	0},
{"p.sh",  		"Xpulpv3", "t,q(s!)", 	MATCH_SHPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sh",  		"Xpulpv3", "t,d(s)",  	MATCH_SHRR,      			MASK_PALU,     	match_opcode,	0},
{"p.sh",  		"Xpulpv3", "t,d(s!)", 	MATCH_SHRRPOST,  			MASK_PALU, 	match_opcode,	0},

{"p.sw",  		"Xpulpv3", "t,q(s)",  	MATCH_SW,        			MASK_SW,      	match_opcode,	0},
{"p.sw",  		"Xpulpv3", "t,q(s!)", 	MATCH_SWPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sw",  		"Xpulpv3", "t,d(s)",  	MATCH_SWRR,      			MASK_PALU,     	match_opcode,	0},
{"p.sw",  		"Xpulpv3", "t,d(s!)", 	MATCH_SWRRPOST,  			MASK_PALU, 	match_opcode,	0},

/* additional ALU operations */

{"p.abs",   		"Xpulpv3", "d,s", 	MATCH_AVG,   				MASK_PALUS,  	match_opcode,	0},
// {"p.avgu",  		"Xpulpv3", "d,s,t", 	MATCH_AVGU,  				MASK_PALU,  	match_opcode,	0},
{"p.slet",  		"Xpulpv3", "d,s,t", 	MATCH_SLET,  				MASK_PALU,  	match_opcode,	0},
{"p.sletu", 		"Xpulpv3", "d,s,t", 	MATCH_SLETU, 				MASK_PALU,  	match_opcode,	0},
{"p.min",   		"Xpulpv3", "d,s,t", 	MATCH_MIN,   				MASK_PALU,  	match_opcode,	0},
{"p.minu",  		"Xpulpv3", "d,s,t", 	MATCH_MINU,  				MASK_PALU,  	match_opcode,	0},
{"p.max",   		"Xpulpv3", "d,s,t", 	MATCH_MAX,   				MASK_PALU,  	match_opcode,	0},
{"p.maxu",  		"Xpulpv3", "d,s,t", 	MATCH_MAXU,  				MASK_PALU,  	match_opcode,	0},
{"p.ror",   		"Xpulpv3", "d,s,t", 	MATCH_ROR,   				MASK_PALU,  	match_opcode,	0},

/* PLACEHOLDER: add your own instruction as part of Xplulv0 ISA */

/* additional ALU operations with only a single source operand */

{"p.ff1",   		"Xpulpv3", "d,s",   	MATCH_FF1,   				MASK_PALUS, 	match_opcode, 	0},
{"p.fl1",   		"Xpulpv3", "d,s",   	MATCH_FL1,   				MASK_PALUS, 	match_opcode, 	0},
{"p.clb",   		"Xpulpv3", "d,s",   	MATCH_CLB,   				MASK_PALUS, 	match_opcode, 	0},
{"p.cnt",   		"Xpulpv3", "d,s",   	MATCH_CNT,   				MASK_PALUS, 	match_opcode, 	0},
{"p.exths", 		"Xpulpv3", "d,s",   	MATCH_EXTHS, 				MASK_PALUS, 	match_opcode, 	0},
{"p.exthz", 		"Xpulpv3", "d,s",   	MATCH_EXTHZ, 				MASK_PALUS, 	match_opcode, 	0},
{"p.extbs", 		"Xpulpv3", "d,s",   	MATCH_EXTBS,		 		MASK_PALUS, 	match_opcode, 	0},
{"p.extbz", 		"Xpulpv3", "d,s",   	MATCH_EXTBZ, 				MASK_PALUS, 	match_opcode, 	0},

/* clip and bit manipulation */

{"p.clip",   		"Xpulpv3", "d,s,bi", 	MATCH_CLIP,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipi",   		"Xpulpv3", "d,s,bi", 	MATCH_CLIP,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipr",   		"Xpulpv3", "d,s,t", 	MATCH_CLIPR,   				MASK_PALU,  	match_opcode,	0},
{"p.clipu",   		"Xpulpv3", "d,s,bi", 	MATCH_CLIPU,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipui",   		"Xpulpv3", "d,s,bi", 	MATCH_CLIPU,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipur",   		"Xpulpv3", "d,s,t", 	MATCH_CLIPUR,  				MASK_PALU,  	match_opcode,	0},

{"p.extract",  		"Xpulpv3", "d,s,b5,bi",	MATCH_EXTRACT, 				MASK_PALU2,  	match_opcode,	0},
{"p.extracti", 		"Xpulpv3", "d,s,b5,bi",	MATCH_EXTRACT, 				MASK_PALU2,  	match_opcode,	0},
{"p.extractr", 		"Xpulpv3", "d,s,t",	MATCH_EXTRACTR,				MASK_PALU,  	match_opcode,	0},
{"p.extractu", 		"Xpulpv3", "d,s,b5,bi",	MATCH_EXTRACTU, 			MASK_PALU2,  	match_opcode,	0},
{"p.extractui", 	"Xpulpv3", "d,s,b5,bi",	MATCH_EXTRACTU, 			MASK_PALU2,  	match_opcode,	0},
{"p.extractur", 	"Xpulpv3", "d,s,t",	MATCH_EXTRACTUR, 			MASK_PALU,  	match_opcode,	0},
{"p.insert",  		"Xpulpv3", "d,s,b5,bi",	MATCH_INSERT, 				MASK_PALU2,  	match_opcode,	0},
{"p.inserti",  		"Xpulpv3", "d,s,b5,bi",	MATCH_INSERT, 				MASK_PALU2,  	match_opcode,	0},
{"p.insertr",  		"Xpulpv3", "d,s,t",	MATCH_INSERTR, 				MASK_PALU,  	match_opcode,	0},
{"p.bset",  		"Xpulpv3", "d,s,b5,bi",	MATCH_BSET, 				MASK_PALU2,  	match_opcode,	0},
{"p.bseti",  		"Xpulpv3", "d,s,b5,bi",	MATCH_BSET, 				MASK_PALU2,  	match_opcode,	0},
{"p.bsetr",  		"Xpulpv3", "d,s,t",	MATCH_BSETR, 				MASK_PALU,  	match_opcode,	0},
{"p.bclr",  		"Xpulpv3", "d,s,b5,bi",	MATCH_BCLR, 				MASK_PALU2,  	match_opcode,	0},
{"p.bclri",  		"Xpulpv3", "d,s,b5,bi",	MATCH_BCLR, 				MASK_PALU2,  	match_opcode,	0},
{"p.bclrr",  		"Xpulpv3", "d,s,t",	MATCH_BCLRR, 				MASK_PALU,  	match_opcode,	0},

/* hardware loops */

{"lp.starti", 		"Xpulpv3", "di,b1",    	MATCH_HWLP_STARTI, 			MASK_HWLP_STARTI,match_opcode, 	0},
{"lp.endi",   		"Xpulpv3", "di,b1",    	MATCH_HWLP_ENDI,   			MASK_HWLP_ENDI,  match_opcode, 	0},
{"lp.count",  		"Xpulpv3", "di,s",     	MATCH_HWLP_COUNT,  			MASK_HWLP_COUNT, match_opcode, 	0},
{"lp.counti", 		"Xpulpv3", "di,ji",    	MATCH_HWLP_COUNTI, 			MASK_HWLP_COUNTI,match_opcode, 	0},
{"lp.setup",  		"Xpulpv3", "di,s,b1",  	MATCH_HWLP_SETUP,  			MASK_HWLP_SETUP, match_opcode, 	0},
{"lp.setupi", 		"Xpulpv3", "di,ji,b2", 	MATCH_HWLP_SETUPI, 			MASK_HWLP_SETUPI,match_opcode, 	0},

/* 32x32 into 32 multiplication */

{"p.mul",      		"Xpulpv3", "d,s,t",  	MATCH_MUL, 				MASK_MUL, 	match_opcode,  	0},

/* 32x32 into 64 support */

{"p.mulh",      	"Xpulpv3", "d,s,t",  	MATCH_MULH, 				MASK_MULH, 	match_opcode,  0},
{"p.mulhu",     	"Xpulpv3", "d,s,t",  	MATCH_MULHU, 				MASK_MULHU, 	match_opcode,  0},
{"p.mulhsu",    	"Xpulpv3", "d,s,t",  	MATCH_MULHSU, 				MASK_MULHSU, 	match_opcode,  0},

/* 32 bit div and rem */

{"p.div",      		"Xpulpv3",   "d,s,t",  	MATCH_DIV, 				MASK_DIV, 	match_opcode,  0},
{"p.divu",     		"Xpulpv3",   "d,s,t",  	MATCH_DIVU, 				MASK_DIVU, 	match_opcode,  0},
{"p.rem",      		"Xpulpv3",   "d,s,t",  	MATCH_REM, 				MASK_REM, 	match_opcode,  0},
{"p.remu",     		"Xpulpv3",   "d,s,t",  	MATCH_REMU,				MASK_REMU, 	match_opcode,  0},

/* 32x32 into 32 Mac/Msu */

{"p.mac",		"Xpulpv3", "d,s,t",  	MATCH_MAC32, 				MASK_MACMSU32, 	match_opcode,  	0},
{"p.msu",		"Xpulpv3", "d,s,t",  	MATCH_MSU32, 				MASK_MACMSU32, 	match_opcode,  	0},

/*  16x16 into 32 Mult/Mac with optional norm and rounding */

{"p.muls",		"Xpulpv3", "d,s,t",  	MATCH_MULS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.mulhhs",		"Xpulpv3", "d,s,t",  	MATCH_MULHHS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.mulu",		"Xpulpv3", "d,s,t",  	MATCH_MULU, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.mulhhu",		"Xpulpv3", "d,s,t",  	MATCH_MULHHU, 				MASK_MACMUL, 	match_opcode,  	0},

{"p.macs",		"Xpulpv3", "d,s,t",  	MATCH_MACS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.machhs",		"Xpulpv3", "d,s,t",  	MATCH_MACHHS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.macu",		"Xpulpv3", "d,s,t",  	MATCH_MACU, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.machhu",		"Xpulpv3", "d,s,t",  	MATCH_MACHHU, 				MASK_MACMUL, 	match_opcode,  	0},

{"p.mulsn",		"Xpulpv3", "d,s,t,b5", 	MATCH_MULSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhsn",		"Xpulpv3", "d,s,t,b5", 	MATCH_MULHHSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulsrn",		"Xpulpv3", "d,s,t,b5", 	MATCH_MULSRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhsrn",		"Xpulpv3", "d,s,t,b5", 	MATCH_MULHHSRN,				MASK_MACMULNR, 	match_opcode,  	0},

{"p.mulun",		"Xpulpv3", "d,s,t,b5", 	MATCH_MULUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhun",		"Xpulpv3", "d,s,t,b5", 	MATCH_MULHHUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulurn",		"Xpulpv3", "d,s,t,b5", 	MATCH_MULURN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhurn",		"Xpulpv3", "d,s,t,b5", 	MATCH_MULHHURN,				MASK_MACMULNR, 	match_opcode,  	0},

{"p.macsn",		"Xpulpv3", "d,s,t,b5", 	MATCH_MACSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhsn",		"Xpulpv3", "d,s,t,b5", 	MATCH_MACHHSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.macsrn",		"Xpulpv3", "d,s,t,b5", 	MATCH_MACSRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhsrn",		"Xpulpv3", "d,s,t,b5", 	MATCH_MACHHSRN,				MASK_MACMULNR, 	match_opcode,  	0},

{"p.macun",		"Xpulpv3", "d,s,t,b5", 	MATCH_MACUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhun",		"Xpulpv3", "d,s,t,b5", 	MATCH_MACHHUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.macurn",		"Xpulpv3", "d,s,t,b5", 	MATCH_MACURN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhurn",		"Xpulpv3", "d,s,t,b5", 	MATCH_MACHHURN,				MASK_MACMULNR, 	match_opcode,  	0},

/*  Add/Sub with norm and rounding */

{"p.addn",		"Xpulpv3", "d,s,t,b5", 	MATCH_ADDN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addni",		"Xpulpv3", "d,s,t,b5", 	MATCH_ADDN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addnr",		"Xpulpv3", "d,s,t", 	MATCH_ADDNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.addun",		"Xpulpv3", "d,s,t,b5", 	MATCH_ADDUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.adduni",		"Xpulpv3", "d,s,t,b5", 	MATCH_ADDUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addunr",		"Xpulpv3", "d,s,t", 	MATCH_ADDUNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.addrn",		"Xpulpv3", "d,s,t,b5", 	MATCH_ADDRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addrni",		"Xpulpv3", "d,s,t,b5", 	MATCH_ADDRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addrnr",		"Xpulpv3", "d,s,t", 	MATCH_ADDRNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.addurn",		"Xpulpv3", "d,s,t,b5", 	MATCH_ADDURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addurni",		"Xpulpv3", "d,s,t,b5", 	MATCH_ADDURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addurnr",		"Xpulpv3", "d,s,t", 	MATCH_ADDURNR,				MASK_PALU, 	match_opcode,  	0},

{"p.subn",		"Xpulpv3", "d,s,t,b5", 	MATCH_SUBN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subni",		"Xpulpv3", "d,s,t,b5", 	MATCH_SUBN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subnr",		"Xpulpv3", "d,s,t", 	MATCH_SUBNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.subun",		"Xpulpv3", "d,s,t,b5", 	MATCH_SUBUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subuni",		"Xpulpv3", "d,s,t,b5", 	MATCH_SUBUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subunr",		"Xpulpv3", "d,s,t", 	MATCH_SUBUNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.subrn",		"Xpulpv3", "d,s,t,b5", 	MATCH_SUBRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subrni",		"Xpulpv3", "d,s,t,b5", 	MATCH_SUBRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subrnr",		"Xpulpv3", "d,s,t", 	MATCH_SUBRNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.suburn",		"Xpulpv3", "d,s,t,b5", 	MATCH_SUBURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.suburni",		"Xpulpv3", "d,s,t,b5", 	MATCH_SUBURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.suburnr",		"Xpulpv3", "d,s,t", 	MATCH_SUBURNR,				MASK_PALU, 	match_opcode,  	0},

/* Vector Operations */

{"pv.add.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sc.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_ADD|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.add.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sc.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_ADD|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.sub.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sc.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_SUB|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.sub.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sc.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_SUB|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.avg.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sc.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_AVG|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.avg.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sc.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_AVG|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.avgu.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sci.h",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_AVGU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.avgu.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_AVGU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.min.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sc.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_MIN|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.min.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sc.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_MIN|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.minu.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sci.h",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_MINU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.minu.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_MINU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.max.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sc.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_MAX|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.max.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sc.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_MAX|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.maxu.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sci.h",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_MAXU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.maxu.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_MAXU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.srl.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sc.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sci.h",	"Xpulpv3", "d,s,bU",	MATCH_V_OP_SRL|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.srl.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sc.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sci.b",	"Xpulpv3", "d,s,bU",	MATCH_V_OP_SRL|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.sra.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sc.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sci.h",	"Xpulpv3", "d,s,bU",	MATCH_V_OP_SRA|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.sra.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sc.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sci.b",	"Xpulpv3", "d,s,bU",	MATCH_V_OP_SRA|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.sll.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sc.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sci.h",	"Xpulpv3", "d,s,bU",	MATCH_V_OP_SLL|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.sll.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sc.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sci.b",	"Xpulpv3", "d,s,bU",	MATCH_V_OP_SLL|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.or.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sc.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sci.h",		"Xpulpv3", "d,s,bs",	MATCH_V_OP_OR|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.or.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sc.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sci.b",		"Xpulpv3", "d,s,bs",	MATCH_V_OP_OR|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.xor.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sc.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_XOR|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.xor.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sc.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_XOR|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.and.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sc.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_AND|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.and.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sc.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_AND|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.abs.h",		"Xpulpv3", "d,s",	MATCH_V_OP_ABS|MATCH_V_OP_H_VV,		MASK_V_OP2,	match_opcode,	0},
{"pv.abs.b",		"Xpulpv3", "d,s",	MATCH_V_OP_ABS|MATCH_V_OP_B_VV,		MASK_V_OP2,	match_opcode,	0},

{"pv.extract.h",	"Xpulpv3", "d,s,bf",	MATCH_V_OP_EXTRACT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.extract.b",	"Xpulpv3", "d,s,bF",	MATCH_V_OP_EXTRACT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.extractu.h",	"Xpulpv3", "d,s,bf",	MATCH_V_OP_DOTSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.extractu.b",	"Xpulpv3", "d,s,bF",	MATCH_V_OP_DOTSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.insert.h",		"Xpulpv3", "d,s,bf",	MATCH_V_OP_SDOTUP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.insert.b",		"Xpulpv3", "d,s,bF",	MATCH_V_OP_SDOTUP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.dotsp.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_DOTUP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.dotsp.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_DOTUP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.dotup.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sci.h",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.dotup.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.dotusp.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_INSERT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.dotusp.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_INSERT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.sdotsp.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.sdotsp.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.sdotup.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sci.h",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_DOTUSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.sdotup.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_DOTUSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.sdotusp.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_SDOTSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.sdotusp.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_SDOTSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.shuffle.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shuffle.sci.h",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shuffle.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shufflei0.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei1.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_SHUFFLEI1|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei2.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_SHUFFLEI2|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei3.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_SHUFFLEI3|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.shuffle2.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_SHUFFLE2|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shuffle2.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_SHUFFLE2|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.pack.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_PACK|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.packhi.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_PACKHI|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.packlo.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_PACKLO|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.cmpeq.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_CMPEQ|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpeq.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_CMPEQ|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpne.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_CMPNE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpne.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_CMPNE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpgt.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_CMPGT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpgt.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_CMPGT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpge.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_CMPGE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpge.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_CMPGE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmplt.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_CMPLT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmplt.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_CMPLT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmple.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sci.h",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_CMPLE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmple.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sci.b",	"Xpulpv3", "d,s,bs",	MATCH_V_OP_CMPLE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpgtu.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sci.h",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_CMPGTU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpgtu.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_CMPGTU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpgeu.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sci.h",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_CMPGEU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpgeu.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_CMPGEU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpltu.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sci.h",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_CMPLTU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpltu.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_CMPLTU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpleu.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sc.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sci.h",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_CMPLEU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpleu.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sc.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_CMPLEU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
/*
{"pv.ball",       	"Xpulpv3",  "s,p",  	MATCH_BEQM1, 			MASK_BEQM1|MASK_RS2, 	match_opcode,   },
{"pv.bnall",       	"Xpulpv3",  "s,p",  	MATCH_BNEM1, 			MASK_BNEM1|MASK_RS2, 	match_opcode,   },
*/
{"p.beqimm",       	"Xpulpv3",  "s,bI,p",  	MATCH_BEQM1, 				MASK_BEQM1, 	match_opcode,    0},
{"p.bneimm",       	"Xpulpv3",  "s,bI,p",  	MATCH_BNEM1, 				MASK_BNEM1, 	match_opcode,    0},

/* Load from event unit */

{"p.elw",		"Xpulpv3", "d,o(s)",	MATCH_LWU, 				MASK_LWU, 	match_opcode,   0},


/* Xpulpnn extensions */
/***********************************************************************************************************************/
/**************************************             Xpulpnn           ***************************************************/
/***********************************************************************************************************************/

{"p.lb",  		"Xpulpnn", "d,o(s)",  	MATCH_LB,        			MASK_LB,      	match_opcode,	0},
{"p.lb",  		"Xpulpnn", "d,o(s!)", 	MATCH_LBPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lb",  		"Xpulpnn", "d,t(s)",  	MATCH_LBRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lb",  		"Xpulpnn", "d,t(s!)", 	MATCH_LBRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

{"p.lbu", 		"Xpulpnn", "d,o(s)",  	MATCH_LBU,       			MASK_LBU,     	match_opcode,	0},
{"p.lbu", 		"Xpulpnn", "d,o(s!)", 	MATCH_LBUPOST,   			MASK_LPOST,   	match_opcode,	0},
{"p.lbu", 		"Xpulpnn", "d,t(s)",  	MATCH_LBURR,     			MASK_LRR,     	match_opcode,	0},
{"p.lbu", 		"Xpulpnn", "d,t(s!)", 	MATCH_LBURRPOST, 			MASK_LRRPOST, 	match_opcode,	0},

{"p.lh",  		"Xpulpnn", "d,o(s)",  	MATCH_LH,        			MASK_LH,      	match_opcode,	0},
{"p.lh",  		"Xpulpnn", "d,o(s!)", 	MATCH_LHPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lh",  		"Xpulpnn", "d,t(s)",  	MATCH_LHRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lh",  		"Xpulpnn", "d,t(s!)", 	MATCH_LHRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

{"p.lhu", 		"Xpulpnn", "d,o(s)",  	MATCH_LHU,       			MASK_LHU,     	match_opcode,	0},
{"p.lhu", 		"Xpulpnn", "d,o(s!)", 	MATCH_LHUPOST,   			MASK_LPOST,   	match_opcode,	0},
{"p.lhu", 		"Xpulpnn", "d,t(s)",  	MATCH_LHURR,     			MASK_LRR,     	match_opcode,	0},
{"p.lhu", 		"Xpulpnn", "d,t(s!)", 	MATCH_LHURRPOST, 			MASK_LRRPOST, 	match_opcode,	0},

{"p.lw",  		"Xpulpnn", "d,o(s)",  	MATCH_LW,        			MASK_LW,      	match_opcode,	0},
{"p.lw",  		"Xpulpnn", "d,o(s!)", 	MATCH_LWPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lw",  		"Xpulpnn", "d,t(s)",  	MATCH_LWRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lw",  		"Xpulpnn", "d,t(s!)", 	MATCH_LWRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

/* post-increment and reg-reg stores */

{"p.sb",  		"Xpulpnn", "t,q(s)",  	MATCH_SB,        			MASK_SB,      	match_opcode,	0},
{"p.sb",  		"Xpulpnn", "t,q(s!)", 	MATCH_SBPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sb",  		"Xpulpnn", "t,d(s)",  	MATCH_SBRR,      			MASK_PALU,     	match_opcode,	0},
{"p.sb",  		"Xpulpnn", "t,d(s!)", 	MATCH_SBRRPOST,  			MASK_PALU, 	match_opcode,	0},

{"p.sh",  		"Xpulpnn", "t,q(s)",  	MATCH_SH,        			MASK_SH,      	match_opcode,	0},
{"p.sh",  		"Xpulpnn", "t,q(s!)", 	MATCH_SHPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sh",  		"Xpulpnn", "t,d(s)",  	MATCH_SHRR,      			MASK_PALU,     	match_opcode,	0},
{"p.sh",  		"Xpulpnn", "t,d(s!)", 	MATCH_SHRRPOST,  			MASK_PALU, 	match_opcode,	0},

{"p.sw",  		"Xpulpnn", "t,q(s)",  	MATCH_SW,        			MASK_SW,      	match_opcode,	0},
{"p.sw",  		"Xpulpnn", "t,q(s!)", 	MATCH_SWPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sw",  		"Xpulpnn", "t,d(s)",  	MATCH_SWRR,      			MASK_PALU,     	match_opcode,	0},
{"p.sw",  		"Xpulpnn", "t,d(s!)", 	MATCH_SWRRPOST,  			MASK_PALU, 	match_opcode,	0},

/* additional ALU operations */

{"p.abs",   		"Xpulpnn", "d,s", 	MATCH_AVG,   				MASK_PALUS,  	match_opcode,	0},
{"p.slet",  		"Xpulpnn", "d,s,t", 	MATCH_SLET,  				MASK_PALU,  	match_opcode,	0},
{"p.sletu", 		"Xpulpnn", "d,s,t", 	MATCH_SLETU, 				MASK_PALU,  	match_opcode,	0},
{"p.min",   		"Xpulpnn", "d,s,t", 	MATCH_MIN,   				MASK_PALU,  	match_opcode,	0},
{"p.minu",  		"Xpulpnn", "d,s,t", 	MATCH_MINU,  				MASK_PALU,  	match_opcode,	0},
{"p.max",   		"Xpulpnn", "d,s,t", 	MATCH_MAX,   				MASK_PALU,  	match_opcode,	0},
{"p.maxu",  		"Xpulpnn", "d,s,t", 	MATCH_MAXU,  				MASK_PALU,  	match_opcode,	0},
{"p.ror",   		"Xpulpnn", "d,s,t", 	MATCH_ROR,   				MASK_PALU,  	match_opcode,	0},

/* PLACEHOLDER: add your own instruction as part of Xplulv0 ISA*/
/* additional ALU operations with only a single source operand */

{"p.ff1",   		"Xpulpnn", "d,s",   	MATCH_FF1,   				MASK_PALUS, 	match_opcode, 	0},
{"p.fl1",   		"Xpulpnn", "d,s",   	MATCH_FL1,   				MASK_PALUS, 	match_opcode, 	0},
{"p.clb",   		"Xpulpnn", "d,s",   	MATCH_CLB,   				MASK_PALUS, 	match_opcode, 	0},
{"p.cnt",   		"Xpulpnn", "d,s",   	MATCH_CNT,   				MASK_PALUS, 	match_opcode, 	0},
{"p.exths", 		"Xpulpnn", "d,s",   	MATCH_EXTHS, 				MASK_PALUS, 	match_opcode, 	0},
{"p.exthz", 		"Xpulpnn", "d,s",   	MATCH_EXTHZ, 				MASK_PALUS, 	match_opcode, 	0},
{"p.extbs", 		"Xpulpnn", "d,s",   	MATCH_EXTBS,		 		MASK_PALUS, 	match_opcode, 	0},
{"p.extbz", 		"Xpulpnn", "d,s",   	MATCH_EXTBZ, 				MASK_PALUS, 	match_opcode, 	0},

/* clip and bit manipulation */


{"p.clip",   		"Xpulpnn", "d,s,bi", 	MATCH_CLIP,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipi",   		"Xpulpnn", "d,s,bi", 	MATCH_CLIP,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipr",   		"Xpulpnn", "d,s,t", 	MATCH_CLIPR,   				MASK_PALU,  	match_opcode,	0},
{"p.clipu",   		"Xpulpnn", "d,s,bi", 	MATCH_CLIPU,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipui",   		"Xpulpnn", "d,s,bi", 	MATCH_CLIPU,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipur",   		"Xpulpnn", "d,s,t", 	MATCH_CLIPUR,  				MASK_PALU,  	match_opcode,	0},

{"p.extract",  		"Xpulpnn", "d,s,b5,bi",	MATCH_EXTRACT, 				MASK_PALU2,  	match_opcode,	0},
{"p.extracti", 		"Xpulpnn", "d,s,b5,bi",	MATCH_EXTRACT, 				MASK_PALU2,  	match_opcode,	0},
{"p.extractr", 		"Xpulpnn", "d,s,t",	MATCH_EXTRACTR,				MASK_PALU,  	match_opcode,	0},
{"p.extractu", 		"Xpulpnn", "d,s,b5,bi",	MATCH_EXTRACTU, 			MASK_PALU2,  	match_opcode,	0},
{"p.extractui", 	"Xpulpnn", "d,s,b5,bi",	MATCH_EXTRACTU, 			MASK_PALU2,  	match_opcode,	0},
{"p.extractur", 	"Xpulpnn", "d,s,t",	MATCH_EXTRACTUR, 			MASK_PALU,  	match_opcode,	0},
{"p.insert",  		"Xpulpnn", "d,s,b5,bi",	MATCH_INSERT, 				MASK_PALU2,  	match_opcode,	0},
{"p.inserti",  		"Xpulpnn", "d,s,b5,bi",	MATCH_INSERT, 				MASK_PALU2,  	match_opcode,	0},
{"p.insertr",  		"Xpulpnn", "d,s,t",	MATCH_INSERTR, 				MASK_PALU,  	match_opcode,	0},
{"p.bset",  		"Xpulpnn", "d,s,b5,bi",	MATCH_BSET, 				MASK_PALU2,  	match_opcode,	0},
{"p.bseti",  		"Xpulpnn", "d,s,b5,bi",	MATCH_BSET, 				MASK_PALU2,  	match_opcode,	0},
{"p.bsetr",  		"Xpulpnn", "d,s,t",	MATCH_BSETR, 				MASK_PALU,  	match_opcode,	0},
{"p.bclr",  		"Xpulpnn", "d,s,b5,bi",	MATCH_BCLR, 				MASK_PALU2,  	match_opcode,	0},
{"p.bclri",  		"Xpulpnn", "d,s,b5,bi",	MATCH_BCLR, 				MASK_PALU2,  	match_opcode,	0},
{"p.bclrr",  		"Xpulpnn", "d,s,t",	MATCH_BCLRR, 				MASK_PALU,  	match_opcode,	0},

/* hardware loops */

{"lp.starti", 		"Xpulpnn", "di,b1",    	MATCH_HWLP_STARTI, 			MASK_HWLP_STARTI,match_opcode, 	0},
{"lp.endi",   		"Xpulpnn", "di,b1",    	MATCH_HWLP_ENDI,   			MASK_HWLP_ENDI,  match_opcode, 	0},
{"lp.count",  		"Xpulpnn", "di,s",     	MATCH_HWLP_COUNT,  			MASK_HWLP_COUNT, match_opcode, 	0},
{"lp.counti", 		"Xpulpnn", "di,ji",    	MATCH_HWLP_COUNTI, 			MASK_HWLP_COUNTI,match_opcode, 	0},
{"lp.setup",  		"Xpulpnn", "di,s,b1",  	MATCH_HWLP_SETUP,  			MASK_HWLP_SETUP, match_opcode, 	0},
{"lp.setupi", 		"Xpulpnn", "di,ji,b2", 	MATCH_HWLP_SETUPI, 			MASK_HWLP_SETUPI,match_opcode, 	0},

/* 32x32 into 32 multiplication */

{"p.mul",      		"Xpulpnn", "d,s,t",  	MATCH_MUL, 				MASK_MUL, 	match_opcode,  	0},

/* 32x32 into 64 support */

{"p.mulh",      	"Xpulpnn", "d,s,t",  	MATCH_MULH, 				MASK_MULH, 	match_opcode,  0},
{"p.mulhu",     	"Xpulpnn", "d,s,t",  	MATCH_MULHU, 				MASK_MULHU, 	match_opcode,  0},
{"p.mulhsu",    	"Xpulpnn", "d,s,t",  	MATCH_MULHSU, 				MASK_MULHSU, 	match_opcode,  0},

/* 32 bit div and rem */

{"p.div",      		"Xpulpnn",   "d,s,t",  	MATCH_DIV, 				MASK_DIV, 	match_opcode,  0},
{"p.divu",     		"Xpulpnn",   "d,s,t",  	MATCH_DIVU, 				MASK_DIVU, 	match_opcode,  0},
{"p.rem",      		"Xpulpnn",   "d,s,t",  	MATCH_REM, 				MASK_REM, 	match_opcode,  0},
{"p.remu",     		"Xpulpnn",   "d,s,t",  	MATCH_REMU,				MASK_REMU, 	match_opcode,  0},

/* 32x32 into 32 Mac/Msu */

{"p.mac",		"Xpulpnn", "d,s,t",  	MATCH_MAC32, 				MASK_MACMSU32, 	match_opcode,  	0},
{"p.msu",		"Xpulpnn", "d,s,t",  	MATCH_MSU32, 				MASK_MACMSU32, 	match_opcode,  	0},

/*  16x16 into 32 Mult/Mac with optional norm and rounding */

{"p.muls",		"Xpulpnn", "d,s,t",  	MATCH_MULS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.mulhhs",		"Xpulpnn", "d,s,t",  	MATCH_MULHHS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.mulu",		"Xpulpnn", "d,s,t",  	MATCH_MULU, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.mulhhu",		"Xpulpnn", "d,s,t",  	MATCH_MULHHU, 				MASK_MACMUL, 	match_opcode,  	0},

{"p.macs",		"Xpulpnn", "d,s,t",  	MATCH_MACS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.machhs",		"Xpulpnn", "d,s,t",  	MATCH_MACHHS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.macu",		"Xpulpnn", "d,s,t",  	MATCH_MACU, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.machhu",		"Xpulpnn", "d,s,t",  	MATCH_MACHHU, 				MASK_MACMUL, 	match_opcode,  	0},

{"p.mulsn",		"Xpulpnn", "d,s,t,b5", 	MATCH_MULSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhsn",		"Xpulpnn", "d,s,t,b5", 	MATCH_MULHHSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulsrn",		"Xpulpnn", "d,s,t,b5", 	MATCH_MULSRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhsrn",		"Xpulpnn", "d,s,t,b5", 	MATCH_MULHHSRN,				MASK_MACMULNR, 	match_opcode,  	0},

{"p.mulun",		"Xpulpnn", "d,s,t,b5", 	MATCH_MULUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhun",		"Xpulpnn", "d,s,t,b5", 	MATCH_MULHHUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulurn",		"Xpulpnn", "d,s,t,b5", 	MATCH_MULURN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhurn",		"Xpulpnn", "d,s,t,b5", 	MATCH_MULHHURN,				MASK_MACMULNR, 	match_opcode,  	0},

{"p.macsn",		"Xpulpnn", "d,s,t,b5", 	MATCH_MACSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhsn",		"Xpulpnn", "d,s,t,b5", 	MATCH_MACHHSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.macsrn",		"Xpulpnn", "d,s,t,b5", 	MATCH_MACSRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhsrn",		"Xpulpnn", "d,s,t,b5", 	MATCH_MACHHSRN,				MASK_MACMULNR, 	match_opcode,  	0},

{"p.macun",		"Xpulpnn", "d,s,t,b5", 	MATCH_MACUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhun",		"Xpulpnn", "d,s,t,b5", 	MATCH_MACHHUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.macurn",		"Xpulpnn", "d,s,t,b5", 	MATCH_MACURN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhurn",		"Xpulpnn", "d,s,t,b5", 	MATCH_MACHHURN,				MASK_MACMULNR, 	match_opcode,  	0},

/*  Add/Sub with norm and rounding */

{"p.addn",		"Xpulpnn", "d,s,t,b5", 	MATCH_ADDN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addni",		"Xpulpnn", "d,s,t,b5", 	MATCH_ADDN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addnr",		"Xpulpnn", "d,s,t", 	MATCH_ADDNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.addun",		"Xpulpnn", "d,s,t,b5", 	MATCH_ADDUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.adduni",		"Xpulpnn", "d,s,t,b5", 	MATCH_ADDUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addunr",		"Xpulpnn", "d,s,t", 	MATCH_ADDUNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.addrn",		"Xpulpnn", "d,s,t,b5", 	MATCH_ADDRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addrni",		"Xpulpnn", "d,s,t,b5", 	MATCH_ADDRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addrnr",		"Xpulpnn", "d,s,t", 	MATCH_ADDRNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.addurn",		"Xpulpnn", "d,s,t,b5", 	MATCH_ADDURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addurni",		"Xpulpnn", "d,s,t,b5", 	MATCH_ADDURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addurnr",		"Xpulpnn", "d,s,t", 	MATCH_ADDURNR,				MASK_PALU, 	match_opcode,  	0},

{"p.subn",		"Xpulpnn", "d,s,t,b5", 	MATCH_SUBN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subni",		"Xpulpnn", "d,s,t,b5", 	MATCH_SUBN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subnr",		"Xpulpnn", "d,s,t", 	MATCH_SUBNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.subun",		"Xpulpnn", "d,s,t,b5", 	MATCH_SUBUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subuni",		"Xpulpnn", "d,s,t,b5", 	MATCH_SUBUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subunr",		"Xpulpnn", "d,s,t", 	MATCH_SUBUNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.subrn",		"Xpulpnn", "d,s,t,b5", 	MATCH_SUBRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subrni",		"Xpulpnn", "d,s,t,b5", 	MATCH_SUBRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subrnr",		"Xpulpnn", "d,s,t", 	MATCH_SUBRNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.suburn",		"Xpulpnn", "d,s,t,b5", 	MATCH_SUBURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.suburni",		"Xpulpnn", "d,s,t,b5", 	MATCH_SUBURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.suburnr",		"Xpulpnn", "d,s,t", 	MATCH_SUBURNR,				MASK_PALU, 	match_opcode,  	0},

/* Vector Operations */

{"pv.add.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sc.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_ADD|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.add.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sc.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_ADD|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.sub.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sc.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_SUB|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.sub.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sc.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_SUB|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.avg.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sc.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_AVG|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.avg.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sc.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_AVG|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.avgu.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sci.h",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_AVGU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.avgu.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sci.b",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_AVGU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.min.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sc.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_MIN|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.min.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sc.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_MIN|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.minu.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sci.h",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_MINU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.minu.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sci.b",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_MINU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.max.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sc.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_MAX|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.max.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sc.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_MAX|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.maxu.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sci.h",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_MAXU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.maxu.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sci.b",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_MAXU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.srl.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sc.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sci.h",	"Xpulpnn", "d,s,bU",	MATCH_V_OP_SRL|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.srl.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sc.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sci.b",	"Xpulpnn", "d,s,bU",	MATCH_V_OP_SRL|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.sra.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sc.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sci.h",	"Xpulpnn", "d,s,bU",	MATCH_V_OP_SRA|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.sra.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sc.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sci.b",	"Xpulpnn", "d,s,bU",	MATCH_V_OP_SRA|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.sll.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sc.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sci.h",	"Xpulpnn", "d,s,bU",	MATCH_V_OP_SLL|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.sll.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sc.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sci.b",	"Xpulpnn", "d,s,bU",	MATCH_V_OP_SLL|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.or.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sc.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sci.h",		"Xpulpnn", "d,s,bs",	MATCH_V_OP_OR|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.or.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sc.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sci.b",		"Xpulpnn", "d,s,bs",	MATCH_V_OP_OR|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.xor.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sc.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_XOR|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.xor.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sc.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_XOR|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.and.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sc.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_AND|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.and.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sc.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_AND|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.abs.h",		"Xpulpnn", "d,s",	MATCH_V_OP_ABS|MATCH_V_OP_H_VV,		MASK_V_OP2,	match_opcode,	0},
{"pv.abs.b",		"Xpulpnn", "d,s",	MATCH_V_OP_ABS|MATCH_V_OP_B_VV,		MASK_V_OP2,	match_opcode,	0},

{"pv.extract.h",	"Xpulpnn", "d,s,bf",	MATCH_V_OP_EXTRACT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.extract.b",	"Xpulpnn", "d,s,bF",	MATCH_V_OP_EXTRACT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.extractu.h",	"Xpulpnn", "d,s,bf",	MATCH_V_OP_DOTSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.extractu.b",	"Xpulpnn", "d,s,bF",	MATCH_V_OP_DOTSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.insert.h",		"Xpulpnn", "d,s,bf",	MATCH_V_OP_SDOTUP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.insert.b",		"Xpulpnn", "d,s,bF",	MATCH_V_OP_SDOTUP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.dotsp.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_DOTUP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.dotsp.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_DOTUP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.dotup.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sci.h",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.dotup.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sci.b",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.dotusp.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_INSERT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.dotusp.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_INSERT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.sdotsp.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.sdotsp.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.sdotup.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sci.h",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_DOTUSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.sdotup.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sci.b",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_DOTUSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.sdotusp.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_SDOTSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.sdotusp.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_SDOTSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.shuffle.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shuffle.sci.h",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shuffle.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shufflei0.sci.b",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei1.sci.b",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_SHUFFLEI1|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei2.sci.b",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_SHUFFLEI2|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei3.sci.b",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_SHUFFLEI3|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.shuffle2.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SHUFFLE2|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shuffle2.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SHUFFLE2|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.pack.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_PACK|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.packhi.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_PACKHI|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.packlo.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_PACKLO|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.cmpeq.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_CMPEQ|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpeq.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_CMPEQ|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpne.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_CMPNE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpne.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_CMPNE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpgt.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_CMPGT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpgt.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_CMPGT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpge.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_CMPGE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpge.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_CMPGE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmplt.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_CMPLT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmplt.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_CMPLT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmple.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sci.h",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_CMPLE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmple.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sci.b",	"Xpulpnn", "d,s,bs",	MATCH_V_OP_CMPLE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpgtu.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sci.h",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_CMPGTU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpgtu.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sci.b",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_CMPGTU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpgeu.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sci.h",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_CMPGEU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpgeu.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sci.b",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_CMPGEU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpltu.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sci.h",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_CMPLTU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpltu.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sci.b",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_CMPLTU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpleu.h",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sc.h",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sci.h",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_CMPLEU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpleu.b",		"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sc.b",	"Xpulpnn", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sci.b",	"Xpulpnn", "d,s,bu",	MATCH_V_OP_CMPLEU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"p.beqimm",       	"Xpulpnn",  "s,bI,p",  	MATCH_BEQM1, 				MASK_BEQM1, 	match_opcode,   0},
{"p.bneimm",       	"Xpulpnn",  "s,bI,p",  	MATCH_BNEM1, 				MASK_BNEM1, 	match_opcode,   0},

/* Load from event unit */

{"p.elw",		"Xpulpnn", "d,o(s)",	MATCH_LWU, 				MASK_LWU, 	match_opcode,   0},
/* Vector instructions */

/* Xpulpnn vectorial add: nibble, crumb */
{"pv.add.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_N_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sc.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_N_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.add.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_C_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sc.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_C_VR,		MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial sub: nibble, crumb */
{"pv.sub.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_N_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sc.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_N_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_C_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sc.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_C_VR,		MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial avg: nibble, crumb */
{"pv.avg.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_N_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sc.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_N_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_C_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sc.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_C_VR,		MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial avgu: nibble, crumb */
{"pv.avgu.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_N_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sc.n",	"Xpulpnn", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_N_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_C_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sc.c",	"Xpulpnn", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_C_VR,	MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial min: nibble, crumb */
{"pv.min.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_N_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sc.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_N_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.min.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_C_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sc.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_C_VR,		MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial minu: nibble, crumb */
{"pv.minu.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_N_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sc.n",	"Xpulpnn", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_N_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_C_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sc.c",	"Xpulpnn", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_C_VR,	MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial max: nibble, crumb */
{"pv.max.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_N_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sc.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_N_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.max.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_C_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sc.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_C_VR,		MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial maxu: nibble, crumb */
{"pv.maxu.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_N_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sc.n",	"Xpulpnn", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_N_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_C_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sc.c",	"Xpulpnn", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_C_VR,	MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial srl: nibble, crumb */
{"pv.srl.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_N_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sc.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_N_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_C_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sc.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_C_VR,		MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial sra: nibble, crumb */
{"pv.sra.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_N_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sc.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_N_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_C_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sc.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_C_VR,		MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial sll: nibble, crumb */
{"pv.sll.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_N_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sc.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_N_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_C_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sc.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_C_VR,		MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial or: nibble, crumb */
{"pv.or.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_N_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sc.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_N_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.or.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_C_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sc.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_C_VR,		MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial xor: nibble, crumb */
{"pv.xor.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_N_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sc.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_N_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_C_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sc.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_C_VR,		MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial and: nibble, crumb */
{"pv.and.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_N_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sc.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_N_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.and.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_C_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sc.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_C_VR,		MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial abs: nibble, crumb */
{"pv.abs.n",		"Xpulpnn", "d,s",	MATCH_V_OP_ABS|MATCH_V_OP_N_VV,		MASK_V_OP2,	match_opcode,	0},
{"pv.abs.c",		"Xpulpnn", "d,s",	MATCH_V_OP_ABS|MATCH_V_OP_C_VV,		MASK_V_OP2,	match_opcode,	0},
/* Xpulpnn vectorial dotup: nibble, crumb */
{"pv.dotup.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_N_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sc.n",	"Xpulpnn", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_N_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_C_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sc.c",	"Xpulpnn", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_C_VR,	MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial dotusp: nibble, crumb */
{"pv.dotusp.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_N_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sc.n",	"Xpulpnn", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_N_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_C_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sc.c",	"Xpulpnn", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_C_VR,	MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial dotsp: nibble, crumb */
{"pv.dotsp.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_N_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sc.n",	"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_N_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_C_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sc.c",	"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_C_VR,	MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial sdotup: nibble, crumb */
{"pv.sdotup.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_N_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sc.n",	"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_N_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_C_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sc.c",	"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_C_VR,	MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial sdotusp: nibble, crumb */
{"pv.sdotusp.n",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_N_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sc.n",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_N_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.c",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_C_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sc.c",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_C_VR,	MASK_V_OP,	match_opcode,	0},
/* Xpulpnn vectorial sdotsp: nibble, crumb */
{"pv.sdotsp.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_N_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sc.n",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_N_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_C_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sc.c",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_C_VR,	MASK_V_OP,	match_opcode,	0},

/* Xpulpnn quantization: nibble, crumble */
{"pv.qnt.n",		"Xpulpnn", "d,s,t",	MATCH_V_OP_PACKLO|MATCH_V_OP_N_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.qnt.c",		"Xpulpnn", "d,s,t",	MATCH_V_OP_PACKLO|MATCH_V_OP_C_VV,	MASK_V_OP,	match_opcode,	0},

/* To be integrated for nibble&crumb somhow -- needed more encoding space -- talk to Giuseppe about it

{"pv.shuffle.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shuffle.sci.h",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shuffle.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shufflei0.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei1.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_SHUFFLEI1|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei2.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_SHUFFLEI2|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei3.sci.b",	"Xpulpv3", "d,s,bu",	MATCH_V_OP_SHUFFLEI3|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.shuffle2.h",	"Xpulpv3", "d,s,t",	MATCH_V_OP_SHUFFLE2|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shuffle2.b",	"Xpulpv3", "d,s,t",	MATCH_V_OP_SHUFFLE2|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.pack.h",		"Xpulpv3", "d,s,t",	MATCH_V_OP_PACK|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.packhi.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_PACKHI|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.packlo.b",		"Xpulpv3", "d,s,t",	MATCH_V_OP_PACKLO|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0}, */

/* Xpulpnn extensions v2 */
/***********************************************************************************************************************/
/**************************************             Xpulpnn           ***************************************************/

{"pv.mlsdotup.h.0",		"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_ML_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotup.h.1",		"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUSP_1|MATCH_V_OP_ML_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotup.h.2",   "Xpulpnn", "d,s,t", MATCH_V_OP_DOTUSP_2|MATCH_V_OP_ML_H_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotup.h.3",   "Xpulpnn", "d,s,t", MATCH_V_OP_DOTUSP_3|MATCH_V_OP_ML_H_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotup.b.0",   "Xpulpnn", "d,s,t", MATCH_V_OP_DOTUSP|MATCH_V_OP_ML_B_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotup.b.1",   "Xpulpnn", "d,s,t", MATCH_V_OP_DOTUSP_1|MATCH_V_OP_ML_B_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotup.b.2",   "Xpulpnn", "d,s,t", MATCH_V_OP_DOTUSP_2|MATCH_V_OP_ML_B_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotup.b.3",   "Xpulpnn", "d,s,t", MATCH_V_OP_DOTUSP_3|MATCH_V_OP_ML_B_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotup.n.0",		"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_ML_N_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotup.n.1",		"Xpulpnn", "d,s,t",	MATCH_V_OP_DOTUSP_1|MATCH_V_OP_ML_N_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotup.n.2",   "Xpulpnn", "d,s,t", MATCH_V_OP_DOTUSP_2|MATCH_V_OP_ML_N_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotup.n.3",   "Xpulpnn", "d,s,t", MATCH_V_OP_DOTUSP_3|MATCH_V_OP_ML_N_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotup.c.0",   "Xpulpnn", "d,s,t", MATCH_V_OP_DOTUSP|MATCH_V_OP_ML_C_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotup.c.1",   "Xpulpnn", "d,s,t", MATCH_V_OP_DOTUSP_1|MATCH_V_OP_ML_C_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotup.c.2",   "Xpulpnn", "d,s,t", MATCH_V_OP_DOTUSP_2|MATCH_V_OP_ML_C_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotup.c.3",   "Xpulpnn", "d,s,t", MATCH_V_OP_DOTUSP_3|MATCH_V_OP_ML_C_VV, MASK_V_OP,  match_opcode, 0},

{"pv.mlsdotusp.h.0",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_ML_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotusp.h.1",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTSP_1|MATCH_V_OP_ML_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotusp.h.2",  "Xpulpnn", "d,s,t", MATCH_V_OP_SDOTSP_2|MATCH_V_OP_ML_H_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotusp.h.3",  "Xpulpnn", "d,s,t", MATCH_V_OP_SDOTSP_3|MATCH_V_OP_ML_H_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotusp.b.0",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_ML_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotusp.b.1",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTSP_1|MATCH_V_OP_ML_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotusp.b.2",  "Xpulpnn", "d,s,t", MATCH_V_OP_SDOTSP_2|MATCH_V_OP_ML_B_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotusp.b.3",  "Xpulpnn", "d,s,t", MATCH_V_OP_SDOTSP_3|MATCH_V_OP_ML_B_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotusp.n.0",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_ML_N_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotusp.n.1",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTSP_1|MATCH_V_OP_ML_N_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotusp.n.2",  "Xpulpnn", "d,s,t", MATCH_V_OP_SDOTSP_2|MATCH_V_OP_ML_N_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotusp.n.3",  "Xpulpnn", "d,s,t", MATCH_V_OP_SDOTSP_3|MATCH_V_OP_ML_N_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotusp.c.0",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_ML_C_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotusp.c.1",	"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTSP_1|MATCH_V_OP_ML_C_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotusp.c.2",  "Xpulpnn", "d,s,t", MATCH_V_OP_SDOTSP_2|MATCH_V_OP_ML_C_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotusp.c.3",  "Xpulpnn", "d,s,t", MATCH_V_OP_SDOTSP_3|MATCH_V_OP_ML_C_VV, MASK_V_OP,  match_opcode, 0},

{"pv.mlsdotsup.h.0",  "Xpulpnn", "d,s,t", MATCH_V_OP_ML_SDOTSUP|MATCH_V_OP_ML_H_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsup.h.1",  "Xpulpnn", "d,s,t", MATCH_V_OP_ML_SDOTSUP_1|MATCH_V_OP_ML_H_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsup.h.2",  "Xpulpnn", "d,s,t", MATCH_V_OP_ML_SDOTSUP_2|MATCH_V_OP_ML_H_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsup.h.3",  "Xpulpnn", "d,s,t", MATCH_V_OP_ML_SDOTSUP_3|MATCH_V_OP_ML_H_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsup.b.0",  "Xpulpnn", "d,s,t", MATCH_V_OP_ML_SDOTSUP|MATCH_V_OP_ML_B_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsup.b.1",  "Xpulpnn", "d,s,t", MATCH_V_OP_ML_SDOTSUP_1|MATCH_V_OP_ML_B_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsup.b.2",  "Xpulpnn", "d,s,t", MATCH_V_OP_ML_SDOTSUP_2|MATCH_V_OP_ML_B_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsup.b.3",  "Xpulpnn", "d,s,t", MATCH_V_OP_ML_SDOTSUP_3|MATCH_V_OP_ML_B_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsup.n.0",  "Xpulpnn", "d,s,t", MATCH_V_OP_ML_SDOTSUP|MATCH_V_OP_ML_N_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsup.n.1",  "Xpulpnn", "d,s,t", MATCH_V_OP_ML_SDOTSUP_1|MATCH_V_OP_ML_N_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsup.n.2",  "Xpulpnn", "d,s,t", MATCH_V_OP_ML_SDOTSUP_2|MATCH_V_OP_ML_N_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsup.n.3",  "Xpulpnn", "d,s,t", MATCH_V_OP_ML_SDOTSUP_3|MATCH_V_OP_ML_N_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsup.c.0",  "Xpulpnn", "d,s,t", MATCH_V_OP_ML_SDOTSUP|MATCH_V_OP_ML_C_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsup.c.1",  "Xpulpnn", "d,s,t", MATCH_V_OP_ML_SDOTSUP_1|MATCH_V_OP_ML_C_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsup.c.2",  "Xpulpnn", "d,s,t", MATCH_V_OP_ML_SDOTSUP_2|MATCH_V_OP_ML_C_VV, MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsup.c.3",  "Xpulpnn", "d,s,t", MATCH_V_OP_ML_SDOTSUP_3|MATCH_V_OP_ML_C_VV, MASK_V_OP,  match_opcode, 0},

{"pv.mlsdotsp.h.0",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_ML_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotsp.h.1",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTUSP_1|MATCH_V_OP_ML_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotsp.h.2",   "Xpulpnn", "d,s,t", MATCH_V_OP_SDOTUSP_2|MATCH_V_OP_ML_H_VV,  MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsp.h.3",   "Xpulpnn", "d,s,t", MATCH_V_OP_SDOTUSP_3|MATCH_V_OP_ML_H_VV,  MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsp.b.0",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_ML_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotsp.b.1",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTUSP_1|MATCH_V_OP_ML_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotsp.b.2",   "Xpulpnn", "d,s,t", MATCH_V_OP_SDOTUSP_2|MATCH_V_OP_ML_B_VV,  MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsp.b.3",   "Xpulpnn", "d,s,t", MATCH_V_OP_SDOTUSP_3|MATCH_V_OP_ML_B_VV,  MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsp.n.0",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_ML_N_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotsp.n.1",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTUSP_1|MATCH_V_OP_ML_N_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotsp.n.2",   "Xpulpnn", "d,s,t", MATCH_V_OP_SDOTUSP_2|MATCH_V_OP_ML_N_VV,  MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsp.n.3",   "Xpulpnn", "d,s,t", MATCH_V_OP_SDOTUSP_3|MATCH_V_OP_ML_N_VV,  MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsp.c.0",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_ML_C_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotsp.c.1",		"Xpulpnn", "d,s,t",	MATCH_V_OP_SDOTUSP_1|MATCH_V_OP_ML_C_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.mlsdotsp.c.2",   "Xpulpnn", "d,s,t", MATCH_V_OP_SDOTUSP_2|MATCH_V_OP_ML_C_VV,  MASK_V_OP,  match_opcode, 0},
{"pv.mlsdotsp.c.3",   "Xpulpnn", "d,s,t", MATCH_V_OP_SDOTUSP_3|MATCH_V_OP_ML_C_VV,  MASK_V_OP,  match_opcode, 0},

/***********************************************************************************************************************/
/**************************************             Gap9             ***************************************************/
/***********************************************************************************************************************/
/* post-increment and register-register loads */

{"p.lb",  		"Xgap9", "d,o(s)",  	MATCH_LB,        			MASK_LB,      	match_opcode,	0},
{"p.lb",  		"Xgap9", "d,o(s!)", 	MATCH_LBPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lb",  		"Xgap9", "d,t(s)",  	MATCH_LBRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lb",  		"Xgap9", "d,t(s!)", 	MATCH_LBRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

{"p.lbu", 		"Xgap9", "d,o(s)",  	MATCH_LBU,       			MASK_LBU,     	match_opcode,	0},
{"p.lbu", 		"Xgap9", "d,o(s!)", 	MATCH_LBUPOST,   			MASK_LPOST,   	match_opcode,	0},
{"p.lbu", 		"Xgap9", "d,t(s)",  	MATCH_LBURR,     			MASK_LRR,     	match_opcode,	0},
{"p.lbu", 		"Xgap9", "d,t(s!)", 	MATCH_LBURRPOST, 			MASK_LRRPOST, 	match_opcode,	0},

{"p.lh",  		"Xgap9", "d,o(s)",  	MATCH_LH,        			MASK_LH,      	match_opcode,	0},
{"p.lh",  		"Xgap9", "d,o(s!)", 	MATCH_LHPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lh",  		"Xgap9", "d,t(s)",  	MATCH_LHRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lh",  		"Xgap9", "d,t(s!)", 	MATCH_LHRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

{"p.lhu", 		"Xgap9", "d,o(s)",  	MATCH_LHU,       			MASK_LHU,     	match_opcode,	0},
{"p.lhu", 		"Xgap9", "d,o(s!)", 	MATCH_LHUPOST,   			MASK_LPOST,   	match_opcode,	0},
{"p.lhu", 		"Xgap9", "d,t(s)",  	MATCH_LHURR,     			MASK_LRR,     	match_opcode,	0},
{"p.lhu", 		"Xgap9", "d,t(s!)", 	MATCH_LHURRPOST, 			MASK_LRRPOST, 	match_opcode,	0},

{"p.lw",  		"Xgap9", "d,o(s)",  	MATCH_LW,        			MASK_LW,      	match_opcode,	0},
{"p.lw",  		"Xgap9", "d,o(s!)", 	MATCH_LWPOST,    			MASK_LPOST,   	match_opcode,	0},
{"p.lw",  		"Xgap9", "d,t(s)",  	MATCH_LWRR,      			MASK_LRR,     	match_opcode,	0},
{"p.lw",  		"Xgap9", "d,t(s!)", 	MATCH_LWRRPOST,  			MASK_LRRPOST, 	match_opcode,	0},

/* post-increment and reg-reg stores */

{"p.sb",  		"Xgap9", "t,q(s)",  	MATCH_SB,        			MASK_SB,      	match_opcode,	0},
{"p.sb",  		"Xgap9", "t,q(s!)", 	MATCH_SBPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sb",  		"Xgap9", "t,d(s)",  	MATCH_SBRR,      			MASK_PALU,     	match_opcode,	0},
{"p.sb",  		"Xgap9", "t,d(s!)", 	MATCH_SBRRPOST,  			MASK_PALU, 	match_opcode,	0},

{"p.sh",  		"Xgap9", "t,q(s)",  	MATCH_SH,        			MASK_SH,      	match_opcode,	0},
{"p.sh",  		"Xgap9", "t,q(s!)", 	MATCH_SHPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sh",  		"Xgap9", "t,d(s)",  	MATCH_SHRR,      			MASK_PALU,     	match_opcode,	0},
{"p.sh",  		"Xgap9", "t,d(s!)", 	MATCH_SHRRPOST,  			MASK_PALU, 	match_opcode,	0},

{"p.sw",  		"Xgap9", "t,q(s)",  	MATCH_SW,        			MASK_SW,      	match_opcode,	0},
{"p.sw",  		"Xgap9", "t,q(s!)", 	MATCH_SWPOST,    			MASK_SPOST,   	match_opcode,	0},
{"p.sw",  		"Xgap9", "t,d(s)",  	MATCH_SWRR,      			MASK_PALU,     	match_opcode,	0},
{"p.sw",  		"Xgap9", "t,d(s!)", 	MATCH_SWRRPOST,  			MASK_PALU, 	match_opcode,	0},

/* additional ALU operations */

{"p.abs",   		"Xgap9", "d,s", 	MATCH_AVG,   				MASK_PALUS,  	match_opcode,	0},
// {"p.avgu",  		"Xgap9", "d,s,t", 	MATCH_AVGU,  				MASK_PALU,  	match_opcode,	0},
{"p.slet",  		"Xgap9", "d,s,t", 	MATCH_SLET,  				MASK_PALU,  	match_opcode,	0},
{"p.sletu", 		"Xgap9", "d,s,t", 	MATCH_SLETU, 				MASK_PALU,  	match_opcode,	0},
{"p.min",   		"Xgap9", "d,s,t", 	MATCH_MIN,   				MASK_PALU,  	match_opcode,	0},
{"p.minu",  		"Xgap9", "d,s,t", 	MATCH_MINU,  				MASK_PALU,  	match_opcode,	0},
{"p.max",   		"Xgap9", "d,s,t", 	MATCH_MAX,   				MASK_PALU,  	match_opcode,	0},
{"p.maxu",  		"Xgap9", "d,s,t", 	MATCH_MAXU,  				MASK_PALU,  	match_opcode,	0},
{"p.ror",   		"Xgap9", "d,s,t", 	MATCH_ROR,   				MASK_PALU,  	match_opcode,	0},


/* PLACEHOLDER: add your own instruction as part of Xplulv0 ISA*/

/* additional ALU operations with only a single source operand */

{"p.ff1",   		"Xgap9", "d,s",   	MATCH_FF1,   				MASK_PALUS, 	match_opcode, 	0},
{"p.fl1",   		"Xgap9", "d,s",   	MATCH_FL1,   				MASK_PALUS, 	match_opcode, 	0},
{"p.clb",   		"Xgap9", "d,s",   	MATCH_CLB,   				MASK_PALUS, 	match_opcode, 	0},
{"p.cnt",   		"Xgap9", "d,s",   	MATCH_CNT,   				MASK_PALUS, 	match_opcode, 	0},
{"p.exths", 		"Xgap9", "d,s",   	MATCH_EXTHS, 				MASK_PALUS, 	match_opcode, 	0},
{"p.exthz", 		"Xgap9", "d,s",   	MATCH_EXTHZ, 				MASK_PALUS, 	match_opcode, 	0},
{"p.extbs", 		"Xgap9", "d,s",   	MATCH_EXTBS,		 		MASK_PALUS, 	match_opcode, 	0},
{"p.extbz", 		"Xgap9", "d,s",   	MATCH_EXTBZ, 				MASK_PALUS, 	match_opcode, 	0},

/* clip and bit manipulation */

{"p.clip",   		"Xgap9", "d,s,bi", 	MATCH_CLIP,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipi",   		"Xgap9", "d,s,bi", 	MATCH_CLIP,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipr",   		"Xgap9", "d,s,t", 	MATCH_CLIPR,   				MASK_PALU,  	match_opcode,	0},
{"p.clipu",   		"Xgap9", "d,s,bi", 	MATCH_CLIPU,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipui",   		"Xgap9", "d,s,bi", 	MATCH_CLIPU,   				MASK_PALU1,  	match_opcode,	0},
{"p.clipur",   		"Xgap9", "d,s,t", 	MATCH_CLIPUR,  				MASK_PALU,  	match_opcode,	0},

{"p.extract",  		"Xgap9", "d,s,b5,bi",	MATCH_EXTRACT, 				MASK_PALU2,  	match_opcode,	0},
{"p.extracti", 		"Xgap9", "d,s,b5,bi",	MATCH_EXTRACT, 				MASK_PALU2,  	match_opcode,	0},
{"p.extractr", 		"Xgap9", "d,s,t",	MATCH_EXTRACTR,				MASK_PALU,  	match_opcode,	0},
{"p.extractu", 		"Xgap9", "d,s,b5,bi",	MATCH_EXTRACTU, 			MASK_PALU2,  	match_opcode,	0},
{"p.extractui", 	"Xgap9", "d,s,b5,bi",	MATCH_EXTRACTU, 			MASK_PALU2,  	match_opcode,	0},
{"p.extractur", 	"Xgap9", "d,s,t",	MATCH_EXTRACTUR, 			MASK_PALU,  	match_opcode,	0},
{"p.insert",  		"Xgap9", "d,s,b5,bi",	MATCH_INSERT, 				MASK_PALU2,  	match_opcode,	0},
{"p.inserti",  		"Xgap9", "d,s,b5,bi",	MATCH_INSERT, 				MASK_PALU2,  	match_opcode,	0},
{"p.insertr",  		"Xgap9", "d,s,t",	MATCH_INSERTR, 				MASK_PALU,  	match_opcode,	0},
{"p.bset",  		"Xgap9", "d,s,b5,bi",	MATCH_BSET, 				MASK_PALU2,  	match_opcode,	0},
{"p.bseti",  		"Xgap9", "d,s,b5,bi",	MATCH_BSET, 				MASK_PALU2,  	match_opcode,	0},
{"p.bsetr",  		"Xgap9", "d,s,t",	MATCH_BSETR, 				MASK_PALU,  	match_opcode,	0},
{"p.bclr",  		"Xgap9", "d,s,b5,bi",	MATCH_BCLR, 				MASK_PALU2,  	match_opcode,	0},
{"p.bclri",  		"Xgap9", "d,s,b5,bi",	MATCH_BCLR, 				MASK_PALU2,  	match_opcode,	0},
{"p.bclrr",  		"Xgap9", "d,s,t",	MATCH_BCLRR, 				MASK_PALU,  	match_opcode,	0},

/* hardware loops */

{"lp.starti", 		"Xgap9", "di,b1",    	MATCH_HWLP_STARTI, 			MASK_HWLP_STARTI,match_opcode, 	0},
{"lp.endi",   		"Xgap9", "di,b1",    	MATCH_HWLP_ENDI,   			MASK_HWLP_ENDI,  match_opcode, 	0},
{"lp.count",  		"Xgap9", "di,s",     	MATCH_HWLP_COUNT,  			MASK_HWLP_COUNT, match_opcode, 	0},
{"lp.counti", 		"Xgap9", "di,ji",    	MATCH_HWLP_COUNTI, 			MASK_HWLP_COUNTI,match_opcode, 	0},
{"lp.setup",  		"Xgap9", "di,s,b1",  	MATCH_HWLP_SETUP,  			MASK_HWLP_SETUP, match_opcode, 	0},
{"lp.setupi", 		"Xgap9", "di,ji,b2", 	MATCH_HWLP_SETUPI, 			MASK_HWLP_SETUPI,match_opcode, 	0},

/* 32x32 into 32 multiplication */

{"p.mul",      		"Xgap9", "d,s,t",  	MATCH_MUL, 				MASK_MUL, 	match_opcode,  	0},

/* 32x32 into 64 support */

{"p.mulh",      	"Xgap9", "d,s,t",  	MATCH_MULH, 				MASK_MULH, 	match_opcode,  0},
{"p.mulhu",     	"Xgap9", "d,s,t",  	MATCH_MULHU, 				MASK_MULHU, 	match_opcode,  0},
{"p.mulhsu",    	"Xgap9", "d,s,t",  	MATCH_MULHSU, 				MASK_MULHSU, 	match_opcode,  0},

/* 32 bit div and rem */

{"p.div",      		"Xgap9",   "d,s,t",  	MATCH_DIV, 				MASK_DIV, 	match_opcode,  0},
{"p.divu",     		"Xgap9",   "d,s,t",  	MATCH_DIVU, 				MASK_DIVU, 	match_opcode,  0},
{"p.rem",      		"Xgap9",   "d,s,t",  	MATCH_REM, 				MASK_REM, 	match_opcode,  0},
{"p.remu",     		"Xgap9",   "d,s,t",  	MATCH_REMU,				MASK_REMU, 	match_opcode,  0},

/* 32x32 into 32 Mac/Msu */

{"p.mac",		"Xgap9", "d,s,t",  	MATCH_MAC32, 				MASK_MACMSU32, 	match_opcode,  	0},
{"p.msu",		"Xgap9", "d,s,t",  	MATCH_MSU32, 				MASK_MACMSU32, 	match_opcode,  	0},

/*  16x16 into 32 Mult/Mac with optional norm and rounding */

{"p.muls",		"Xgap9", "d,s,t",  	MATCH_MULS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.mulhhs",		"Xgap9", "d,s,t",  	MATCH_MULHHS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.mulu",		"Xgap9", "d,s,t",  	MATCH_MULU, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.mulhhu",		"Xgap9", "d,s,t",  	MATCH_MULHHU, 				MASK_MACMUL, 	match_opcode,  	0},

{"p.macs",		"Xgap9", "d,s,t",  	MATCH_MACS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.machhs",		"Xgap9", "d,s,t",  	MATCH_MACHHS, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.macu",		"Xgap9", "d,s,t",  	MATCH_MACU, 				MASK_MACMUL, 	match_opcode,  	0},
{"p.machhu",		"Xgap9", "d,s,t",  	MATCH_MACHHU, 				MASK_MACMUL, 	match_opcode,  	0},

{"p.mulsn",		"Xgap9", "d,s,t,b5", 	MATCH_MULSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhsn",		"Xgap9", "d,s,t,b5", 	MATCH_MULHHSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulsrn",		"Xgap9", "d,s,t,b5", 	MATCH_MULSRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhsrn",		"Xgap9", "d,s,t,b5", 	MATCH_MULHHSRN,				MASK_MACMULNR, 	match_opcode,  	0},

{"p.mulun",		"Xgap9", "d,s,t,b5", 	MATCH_MULUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhun",		"Xgap9", "d,s,t,b5", 	MATCH_MULHHUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulurn",		"Xgap9", "d,s,t,b5", 	MATCH_MULURN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.mulhhurn",		"Xgap9", "d,s,t,b5", 	MATCH_MULHHURN,				MASK_MACMULNR, 	match_opcode,  	0},

{"p.macsn",		"Xgap9", "d,s,t,b5", 	MATCH_MACSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhsn",		"Xgap9", "d,s,t,b5", 	MATCH_MACHHSN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.macsrn",		"Xgap9", "d,s,t,b5", 	MATCH_MACSRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhsrn",		"Xgap9", "d,s,t,b5", 	MATCH_MACHHSRN,				MASK_MACMULNR, 	match_opcode,  	0},

{"p.macun",		"Xgap9", "d,s,t,b5", 	MATCH_MACUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhun",		"Xgap9", "d,s,t,b5", 	MATCH_MACHHUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.macurn",		"Xgap9", "d,s,t,b5", 	MATCH_MACURN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.machhurn",		"Xgap9", "d,s,t,b5", 	MATCH_MACHHURN,				MASK_MACMULNR, 	match_opcode,  	0},

/*  Add/Sub with norm and rounding */

{"p.addn",		"Xgap9", "d,s,t,b5", 	MATCH_ADDN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addni",		"Xgap9", "d,s,t,b5", 	MATCH_ADDN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addnr",		"Xgap9", "d,s,t", 	MATCH_ADDNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.addun",		"Xgap9", "d,s,t,b5", 	MATCH_ADDUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.adduni",		"Xgap9", "d,s,t,b5", 	MATCH_ADDUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addunr",		"Xgap9", "d,s,t", 	MATCH_ADDUNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.addrn",		"Xgap9", "d,s,t,b5", 	MATCH_ADDRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addrni",		"Xgap9", "d,s,t,b5", 	MATCH_ADDRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addrnr",		"Xgap9", "d,s,t", 	MATCH_ADDRNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.addurn",		"Xgap9", "d,s,t,b5", 	MATCH_ADDURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addurni",		"Xgap9", "d,s,t,b5", 	MATCH_ADDURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.addurnr",		"Xgap9", "d,s,t", 	MATCH_ADDURNR,				MASK_PALU, 	match_opcode,  	0},

{"p.subn",		"Xgap9", "d,s,t,b5", 	MATCH_SUBN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subni",		"Xgap9", "d,s,t,b5", 	MATCH_SUBN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subnr",		"Xgap9", "d,s,t", 	MATCH_SUBNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.subun",		"Xgap9", "d,s,t,b5", 	MATCH_SUBUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subuni",		"Xgap9", "d,s,t,b5", 	MATCH_SUBUN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subunr",		"Xgap9", "d,s,t", 	MATCH_SUBUNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.subrn",		"Xgap9", "d,s,t,b5", 	MATCH_SUBRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subrni",		"Xgap9", "d,s,t,b5", 	MATCH_SUBRN, 				MASK_MACMULNR, 	match_opcode,  	0},
{"p.subrnr",		"Xgap9", "d,s,t", 	MATCH_SUBRNR, 				MASK_PALU, 	match_opcode,  	0},
{"p.suburn",		"Xgap9", "d,s,t,b5", 	MATCH_SUBURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.suburni",		"Xgap9", "d,s,t,b5", 	MATCH_SUBURN,				MASK_MACMULNR, 	match_opcode,  	0},
{"p.suburnr",		"Xgap9", "d,s,t", 	MATCH_SUBURNR,				MASK_PALU, 	match_opcode,  	0},

/* Vector Operations */

{"pv.add.h",		"Xgap9", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sc.h",		"Xgap9", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_ADD|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.add.b",		"Xgap9", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sc.b",		"Xgap9", "d,s,t",	MATCH_V_OP_ADD|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.add.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_ADD|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.sub.h",		"Xgap9", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sc.h",		"Xgap9", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_SUB|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.sub.b",		"Xgap9", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sc.b",		"Xgap9", "d,s,t",	MATCH_V_OP_SUB|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sub.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_SUB|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.avg.h",		"Xgap9", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sc.h",		"Xgap9", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_AVG|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.avg.b",		"Xgap9", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sc.b",		"Xgap9", "d,s,t",	MATCH_V_OP_AVG|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.avg.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_AVG|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.avgu.h",		"Xgap9", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sci.h",	"Xgap9", "d,s,bu",	MATCH_V_OP_AVGU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.avgu.b",		"Xgap9", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_AVGU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.avgu.sci.b",	"Xgap9", "d,s,bu",	MATCH_V_OP_AVGU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.min.h",		"Xgap9", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sc.h",		"Xgap9", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_MIN|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.min.b",		"Xgap9", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sc.b",		"Xgap9", "d,s,t",	MATCH_V_OP_MIN|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.min.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_MIN|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.minu.h",		"Xgap9", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sci.h",	"Xgap9", "d,s,bu",	MATCH_V_OP_MINU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.minu.b",		"Xgap9", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_MINU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.minu.sci.b",	"Xgap9", "d,s,bu",	MATCH_V_OP_MINU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.max.h",		"Xgap9", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sc.h",		"Xgap9", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_MAX|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.max.b",		"Xgap9", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sc.b",		"Xgap9", "d,s,t",	MATCH_V_OP_MAX|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.max.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_MAX|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.maxu.h",		"Xgap9", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sci.h",	"Xgap9", "d,s,bu",	MATCH_V_OP_MAXU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.maxu.b",		"Xgap9", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_MAXU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.maxu.sci.b",	"Xgap9", "d,s,bu",	MATCH_V_OP_MAXU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.srl.h",		"Xgap9", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sc.h",		"Xgap9", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sci.h",	"Xgap9", "d,s,bU",	MATCH_V_OP_SRL|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.srl.b",		"Xgap9", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sc.b",		"Xgap9", "d,s,t",	MATCH_V_OP_SRL|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.srl.sci.b",	"Xgap9", "d,s,bU",	MATCH_V_OP_SRL|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.sra.h",		"Xgap9", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sc.h",		"Xgap9", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sci.h",	"Xgap9", "d,s,bU",	MATCH_V_OP_SRA|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.sra.b",		"Xgap9", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sc.b",		"Xgap9", "d,s,t",	MATCH_V_OP_SRA|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sra.sci.b",	"Xgap9", "d,s,bU",	MATCH_V_OP_SRA|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.sll.h",		"Xgap9", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sc.h",		"Xgap9", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sci.h",	"Xgap9", "d,s,bU",	MATCH_V_OP_SLL|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.sll.b",		"Xgap9", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sc.b",		"Xgap9", "d,s,t",	MATCH_V_OP_SLL|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.sll.sci.b",	"Xgap9", "d,s,bU",	MATCH_V_OP_SLL|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.or.h",		"Xgap9", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sc.h",		"Xgap9", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sci.h",		"Xgap9", "d,s,bs",	MATCH_V_OP_OR|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.or.b",		"Xgap9", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sc.b",		"Xgap9", "d,s,t",	MATCH_V_OP_OR|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.or.sci.b",		"Xgap9", "d,s,bs",	MATCH_V_OP_OR|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.xor.h",		"Xgap9", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sc.h",		"Xgap9", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_XOR|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.xor.b",		"Xgap9", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sc.b",		"Xgap9", "d,s,t",	MATCH_V_OP_XOR|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.xor.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_XOR|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.and.h",		"Xgap9", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_H_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sc.h",		"Xgap9", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_H_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_AND|MATCH_V_OP_H_VI,		MASK_V_OP1,	match_opcode,	0},
{"pv.and.b",		"Xgap9", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_B_VV,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sc.b",		"Xgap9", "d,s,t",	MATCH_V_OP_AND|MATCH_V_OP_B_VR,		MASK_V_OP,	match_opcode,	0},
{"pv.and.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_AND|MATCH_V_OP_B_VI,		MASK_V_OP1,	match_opcode,	0},

{"pv.abs.h",		"Xgap9", "d,s",	MATCH_V_OP_ABS|MATCH_V_OP_H_VV,		MASK_V_OP2,	match_opcode,	0},
{"pv.abs.b",		"Xgap9", "d,s",	MATCH_V_OP_ABS|MATCH_V_OP_B_VV,		MASK_V_OP2,	match_opcode,	0},

{"pv.extract.h",	"Xgap9", "d,s,bf",	MATCH_V_OP_EXTRACT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.extract.b",	"Xgap9", "d,s,bF",	MATCH_V_OP_EXTRACT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.extractu.h",	"Xgap9", "d,s,bf",	MATCH_V_OP_DOTSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.extractu.b",	"Xgap9", "d,s,bF",	MATCH_V_OP_DOTSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.insert.h",		"Xgap9", "d,s,bf",	MATCH_V_OP_SDOTUP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.insert.b",		"Xgap9", "d,s,bF",	MATCH_V_OP_SDOTUP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.dotsp.h",		"Xgap9", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_DOTUP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.dotsp.b",		"Xgap9", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_DOTUP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotsp.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_DOTUP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.dotup.h",		"Xgap9", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sci.h",	"Xgap9", "d,s,bu",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.dotup.b",		"Xgap9", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotup.sci.b",	"Xgap9", "d,s,bu",	MATCH_V_OP_EXTRACTU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.dotusp.h",		"Xgap9", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_INSERT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.dotusp.b",		"Xgap9", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_INSERT|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.dotusp.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_INSERT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.sdotsp.h",		"Xgap9", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.sdotsp.b",		"Xgap9", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotsp.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_SDOTUSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.sdotup.h",		"Xgap9", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sci.h",	"Xgap9", "d,s,bu",	MATCH_V_OP_DOTUSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.sdotup.b",		"Xgap9", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_DOTUSP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotup.sci.b",	"Xgap9", "d,s,bu",	MATCH_V_OP_DOTUSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.sdotusp.h",	"Xgap9", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_SDOTSP|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.sdotusp.b",	"Xgap9", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_SDOTSP|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.sdotusp.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_SDOTSP|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.shuffle.h",	"Xgap9", "d,s,t",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shuffle.sci.h",	"Xgap9", "d,s,bu",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shuffle.b",	"Xgap9", "d,s,t",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shufflei0.sci.b",	"Xgap9", "d,s,bu",	MATCH_V_OP_SHUFFLE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei1.sci.b",	"Xgap9", "d,s,bu",	MATCH_V_OP_SHUFFLEI1|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei2.sci.b",	"Xgap9", "d,s,bu",	MATCH_V_OP_SHUFFLEI2|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.shufflei3.sci.b",	"Xgap9", "d,s,bu",	MATCH_V_OP_SHUFFLEI3|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.shuffle2.h",	"Xgap9", "d,s,t",	MATCH_V_OP_SHUFFLE2|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.shuffle2.b",	"Xgap9", "d,s,t",	MATCH_V_OP_SHUFFLE2|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.pack.h",		"Xgap9", "d,s,t",	MATCH_V_OP_PACK|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.packhi.b",		"Xgap9", "d,s,t",	MATCH_V_OP_PACKHI|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.packlo.b",		"Xgap9", "d,s,t",	MATCH_V_OP_PACKLO|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},

{"pv.cmpeq.h",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_CMPEQ|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpeq.b",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPEQ|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpeq.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_CMPEQ|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpne.h",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_CMPNE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpne.b",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPNE|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpne.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_CMPNE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpgt.h",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_CMPGT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpgt.b",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPGT|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgt.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_CMPGT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpge.h",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_CMPGE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpge.b",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPGE|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpge.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_CMPGE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmplt.h",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_CMPLT|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmplt.b",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPLT|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmplt.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_CMPLT|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmple.h",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sci.h",	"Xgap9", "d,s,bs",	MATCH_V_OP_CMPLE|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmple.b",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPLE|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmple.sci.b",	"Xgap9", "d,s,bs",	MATCH_V_OP_CMPLE|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpgtu.h",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sci.h",	"Xgap9", "d,s,bu",	MATCH_V_OP_CMPGTU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpgtu.b",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPGTU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgtu.sci.b",	"Xgap9", "d,s,bu",	MATCH_V_OP_CMPGTU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpgeu.h",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sci.h",	"Xgap9", "d,s,bu",	MATCH_V_OP_CMPGEU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpgeu.b",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPGEU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpgeu.sci.b",	"Xgap9", "d,s,bu",	MATCH_V_OP_CMPGEU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpltu.h",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sci.h",	"Xgap9", "d,s,bu",	MATCH_V_OP_CMPLTU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpltu.b",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPLTU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpltu.sci.b",	"Xgap9", "d,s,bu",	MATCH_V_OP_CMPLTU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},

{"pv.cmpleu.h",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sc.h",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_H_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sci.h",	"Xgap9", "d,s,bu",	MATCH_V_OP_CMPLEU|MATCH_V_OP_H_VI,	MASK_V_OP1,	match_opcode,	0},
{"pv.cmpleu.b",		"Xgap9", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_B_VV,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sc.b",	"Xgap9", "d,s,t",	MATCH_V_OP_CMPLEU|MATCH_V_OP_B_VR,	MASK_V_OP,	match_opcode,	0},
{"pv.cmpleu.sci.b",	"Xgap9", "d,s,bu",	MATCH_V_OP_CMPLEU|MATCH_V_OP_B_VI,	MASK_V_OP1,	match_opcode,	0},
/*
{"pv.ball",       	"Xgap9",  "s,p",  	MATCH_BEQM1, 			MASK_BEQM1|MASK_RS2, 	match_opcode,   },
{"pv.bnall",       	"Xgap9",  "s,p",  	MATCH_BNEM1, 			MASK_BNEM1|MASK_RS2, 	match_opcode,   },
*/
{"p.beqimm",       	"Xgap9",  "s,bI,p",  	MATCH_BEQM1, 				MASK_BEQM1, 	match_opcode,    0},
{"p.bneimm",       	"Xgap9",  "s,bI,p",  	MATCH_BNEM1, 				MASK_BNEM1, 	match_opcode,    0},

/* Complex numbers */
{"pv.cplxmul.h.r",	"Xgap9",   "d,s,t",	MATCH_V_OP_CPLXMULR|MATCH_V_OP_H_VV_S0,	MASK_V_OP11,	match_opcode,	0},
{"pv.cplxmul.h.r.div2",	"Xgap9",   "d,s,t",	MATCH_V_OP_CPLXMULR|MATCH_V_OP_H_VV_S1,	MASK_V_OP11,	match_opcode,	0},
{"pv.cplxmul.h.r.div4",	"Xgap9",   "d,s,t",	MATCH_V_OP_CPLXMULR|MATCH_V_OP_H_VV_S2,	MASK_V_OP11,	match_opcode,	0},
{"pv.cplxmul.h.r.div8",	"Xgap9",   "d,s,t",	MATCH_V_OP_CPLXMULR|MATCH_V_OP_H_VV_S3,	MASK_V_OP11,	match_opcode,	0},

{"pv.cplxmul.h.i",	"Xgap9",   "d,s,t",	MATCH_V_OP_CPLXMULI|MATCH_V_OP_H_VV_S0,	MASK_V_OP11,	match_opcode,	0},
{"pv.cplxmul.h.i.div2",	"Xgap9",   "d,s,t",	MATCH_V_OP_CPLXMULI|MATCH_V_OP_H_VV_S1,	MASK_V_OP11,	match_opcode,	0},
{"pv.cplxmul.h.i.div4",	"Xgap9",   "d,s,t",	MATCH_V_OP_CPLXMULI|MATCH_V_OP_H_VV_S2,	MASK_V_OP11,	match_opcode,	0},
{"pv.cplxmul.h.i.div8",	"Xgap9",   "d,s,t",	MATCH_V_OP_CPLXMULI|MATCH_V_OP_H_VV_S3,	MASK_V_OP11,	match_opcode,	0},

{"pv.subrotmj.h",	"Xgap9",   "d,s,t",	MATCH_V_OP_SUBROTMJ|MATCH_V_OP_H_VV_S0,	MASK_V_OP,	match_opcode,	0},
{"pv.subrotmj.h.div2",	"Xgap9",   "d,s,t",	MATCH_V_OP_SUBROTMJ|MATCH_V_OP_H_VV_S1,	MASK_V_OP,	match_opcode,	0},
{"pv.subrotmj.h.div4",	"Xgap9",   "d,s,t",	MATCH_V_OP_SUBROTMJ|MATCH_V_OP_H_VV_S2,	MASK_V_OP,	match_opcode,	0},
{"pv.subrotmj.h.div8",	"Xgap9",   "d,s,t",	MATCH_V_OP_SUBROTMJ|MATCH_V_OP_H_VV_S3,	MASK_V_OP,	match_opcode,	0},

{"pv.cplxconj.h",	"Xgap9",   "d,s",	MATCH_V_OP_CPLXCONJ|MATCH_V_OP_H_VV,	MASK_V_OP2,	match_opcode,	0},

{"pv.add.h.div2",	"Xgap9",   "d,s,t",	MATCH_V_OP_ADD_DIV|MATCH_V_OP_H_VV_S1,	MASK_V_OP,	match_opcode,	0},
{"pv.add.h.div4",	"Xgap9",   "d,s,t",	MATCH_V_OP_ADD_DIV|MATCH_V_OP_H_VV_S2,	MASK_V_OP,	match_opcode,	0},
{"pv.add.h.div8",	"Xgap9",   "d,s,t",	MATCH_V_OP_ADD_DIV|MATCH_V_OP_H_VV_S3,	MASK_V_OP,	match_opcode,	0},

{"pv.sub.h.div2",	"Xgap9",   "d,s,t",	MATCH_V_OP_SUB_DIV|MATCH_V_OP_H_VV_S1,	MASK_V_OP,	match_opcode,	0},
{"pv.sub.h.div4",	"Xgap9",   "d,s,t",	MATCH_V_OP_SUB_DIV|MATCH_V_OP_H_VV_S2,	MASK_V_OP,	match_opcode,	0},
{"pv.sub.h.div8",	"Xgap9",   "d,s,t",	MATCH_V_OP_SUB_DIV|MATCH_V_OP_H_VV_S3,	MASK_V_OP,	match_opcode,	0},

{"pv.pack.h.h",		"Xgap9",   "d,s,t",	MATCH_V_OP_PACKH|MATCH_V_OP_H_VV,	MASK_V_OP,	match_opcode,	0},

/* Bit reverse */

{"p.bitrev",		"Xgap9",   "d,s,bi,b5",	MATCH_BITREV,				MASK_BITREV,	match_opcode,	0},

/* Load from event unit */

{"p.elw",		"Xgap9", "d,o(s)",	MATCH_LWU, 				MASK_LWU, 	match_opcode,   0},



/* Single-precision floating-point instruction subset  when float registers are mapped on the int reg file */

/* Riscv F with floats mapped on to integer register file */
{"frsr",      "Xgap9",   "d",  		MATCH_FRCSR, 			MASK_FRCSR, 		match_opcode, 0 },
{"fssr",      "Xgap9",   "s",  		MATCH_FSCSR, 			MASK_FSCSR | MASK_RD, 	match_opcode, 0 },
{"fssr",      "Xgap9",   "d,s",  	MATCH_FSCSR, 			MASK_FSCSR, 		match_opcode, 0 },
{"frcsr",     "Xgap9",   "d",  		MATCH_FRCSR, 			MASK_FRCSR, 		match_opcode, 0 },
{"fscsr",     "Xgap9",   "s",  		MATCH_FSCSR, 			MASK_FSCSR | MASK_RD, 	match_opcode, 0 },
{"fscsr",     "Xgap9",   "d,s",  	MATCH_FSCSR, 			MASK_FSCSR, 		match_opcode, 0 },
{"frrm",      "Xgap9",   "d",  		MATCH_FRRM, 			MASK_FRRM, 		match_opcode, 0 },
{"fsrm",      "Xgap9",   "s",  		MATCH_FSRM, 			MASK_FSRM | MASK_RD, 	match_opcode, 0 },
{"fsrm",      "Xgap9",   "d,s",  	MATCH_FSRM, 			MASK_FSRM, 		match_opcode, 0 },
{"frflags",   "Xgap9",   "d",  		MATCH_FRFLAGS, 			MASK_FRFLAGS, 		match_opcode, 0 },
{"fsflags",   "Xgap9",   "s",  		MATCH_FSFLAGS, 			MASK_FSFLAGS | MASK_RD, match_opcode, 0 },
{"fsflags",   "Xgap9",   "d,s",  	MATCH_FSFLAGS, 			MASK_FSFLAGS, 		match_opcode, 0 },

/* Unused since we map float to int register file
{"flw",       "32C", "D,Cm(Cc)",  MATCH_C_FLWSP, MASK_C_FLWSP, match_opcode, INSN_ALIAS },
{"flw",       "32C", "CD,Ck(Cs)",  MATCH_C_FLW, MASK_C_FLW, match_opcode, INSN_ALIAS },
{"flw",       "F",   "D,o(s)",  MATCH_FLW, MASK_FLW, match_opcode, 0 },
{"flw",       "F",   "D,A,s",  0, (int) M_FLW, match_never, INSN_MACRO },
{"fsw",       "32C", "CT,CM(Cc)",  MATCH_C_FSWSP, MASK_C_FSWSP, match_opcode, INSN_ALIAS },
{"fsw",       "32C", "CD,Ck(Cs)",  MATCH_C_FSW, MASK_C_FSW, match_opcode, INSN_ALIAS },
{"fsw",       "F",   "T,q(s)",  MATCH_FSW, MASK_FSW, match_opcode, 0 },
{"fsw",       "F",   "T,A,s",  0, (int) M_FSW, match_never, INSN_MACRO },
{"fmv.x.s",   "F",   "d,S",  MATCH_FMV_X_S, MASK_FMV_X_S, match_opcode, 0 },
{"fmv.s.x",   "F",   "D,s",  MATCH_FMV_S_X, MASK_FMV_S_X, match_opcode, 0 },
{"fmv.s",     "F",   "D,U",  MATCH_FSGNJ_S, MASK_FSGNJ_S, match_rs1_eq_rs2, INSN_ALIAS },
*/

{"fneg.s",    "Xgap9",   "d,w",  	MATCH_FSGNJN_S, 		MASK_FSGNJN_S, 		match_rs1_eq_rs2, INSN_ALIAS },
{"fabs.s",    "Xgap9",   "d,w",  	MATCH_FSGNJX_S, 		MASK_FSGNJX_S, 		match_rs1_eq_rs2, INSN_ALIAS },
{"fsgnj.s",   "Xgap9",   "d,s,t",  	MATCH_FSGNJ_S, 			MASK_FSGNJ_S, 		match_opcode, 0 },
{"fsgnjn.s",  "Xgap9",   "d,s,t",  	MATCH_FSGNJN_S, 		MASK_FSGNJN_S, 		match_opcode, 0 },
{"fsgnjx.s",  "Xgap9",   "d,s,t",  	MATCH_FSGNJX_S, 		MASK_FSGNJX_S, 		match_opcode, 0 },
{"fadd.s",    "Xgap9",   "d,s,t",  	MATCH_FADD_S | MASK_RM, 	MASK_FADD_S | MASK_RM, 	match_opcode, 0 },
{"fadd.s",    "Xgap9",   "d,s,t,m",  	MATCH_FADD_S, 			MASK_FADD_S, 		match_opcode, 0 },
{"fsub.s",    "Xgap9",   "d,s,t",  	MATCH_FSUB_S | MASK_RM, 	MASK_FSUB_S | MASK_RM, 	match_opcode, 0 },
{"fsub.s",    "Xgap9",   "d,s,t,m",  	MATCH_FSUB_S, 			MASK_FSUB_S, 		match_opcode, 0 },
{"fmul.s",    "Xgap9",   "d,s,t",  	MATCH_FMUL_S | MASK_RM, 	MASK_FMUL_S | MASK_RM, 	match_opcode, 0 },
{"fmul.s",    "Xgap9",   "d,s,t,m",  	MATCH_FMUL_S, 			MASK_FMUL_S, 		match_opcode, 0 },
{"fdiv.s",    "Xgap9",   "d,s,t",  	MATCH_FDIV_S | MASK_RM, 	MASK_FDIV_S | MASK_RM, 	match_opcode, 0 },
{"fdiv.s",    "Xgap9",   "d,s,t,m",  	MATCH_FDIV_S, 			MASK_FDIV_S, 		match_opcode, 0 },
{"fsqrt.s",   "Xgap9",   "d,s",  	MATCH_FSQRT_S | MASK_RM, 	MASK_FSQRT_S | MASK_RM, match_opcode, 0 },
{"fsqrt.s",   "Xgap9",   "d,s,m",  	MATCH_FSQRT_S, 			MASK_FSQRT_S, 		match_opcode, 0 },
{"fmin.s",    "Xgap9",   "d,s,t",  	MATCH_FMIN_S, 			MASK_FMIN_S, 		match_opcode, 0 },
{"fmax.s",    "Xgap9",   "d,s,t",  	MATCH_FMAX_S, 			MASK_FMAX_S, 		match_opcode, 0 },
{"fmadd.s",   "Xgap9",   "d,s,t,e",  	MATCH_FMADD_S | MASK_RM, 	MASK_FMADD_S | MASK_RM, match_opcode, 0 },
{"fmadd.s",   "Xgap9",   "d,s,t,e,m",  	MATCH_FMADD_S, 			MASK_FMADD_S, 		match_opcode, 0 },
{"fnmadd.s",  "Xgap9",   "d,s,t,e",  	MATCH_FNMADD_S | MASK_RM, 	MASK_FNMADD_S| MASK_RM, match_opcode, 0 },
{"fnmadd.s",  "Xgap9",   "d,s,t,e,m",  	MATCH_FNMADD_S,			MASK_FNMADD_S, 		match_opcode, 0 },
{"fmsub.s",   "Xgap9",   "d,s,t,e",  	MATCH_FMSUB_S | MASK_RM, 	MASK_FMSUB_S | MASK_RM, match_opcode, 0 },
{"fmsub.s",   "Xgap9",   "d,s,t,e,m",  	MATCH_FMSUB_S, 			MASK_FMSUB_S, 		match_opcode, 0 },
{"fnmsub.s",  "Xgap9",   "d,s,t,e",  	MATCH_FNMSUB_S | MASK_RM, 	MASK_FNMSUB_S| MASK_RM, match_opcode, 0 },
{"fnmsub.s",  "Xgap9",   "d,s,t,e,m",  	MATCH_FNMSUB_S, 		MASK_FNMSUB_S, 		match_opcode, 0 },
{"fcvt.w.s",  "Xgap9",   "d,s",  	MATCH_FCVT_W_S | MASK_RM, 	MASK_FCVT_W_S| MASK_RM, match_opcode, 0 },
{"fcvt.w.s",  "Xgap9",   "d,s,m",  	MATCH_FCVT_W_S, 		MASK_FCVT_W_S, 		match_opcode, 0 },
{"fcvt.wu.s", "Xgap9",   "d,s",  	MATCH_FCVT_WU_S | MASK_RM, 	MASK_FCVT_WU_S|MASK_RM, match_opcode, 0 },
{"fcvt.wu.s", "Xgap9",   "d,s,m",  	MATCH_FCVT_WU_S, 		MASK_FCVT_WU_S, 	match_opcode, 0 },
{"fcvt.s.w",  "Xgap9",   "d,s",  	MATCH_FCVT_S_W | MASK_RM, 	MASK_FCVT_S_W| MASK_RM,	match_opcode, 0 },
{"fcvt.s.w",  "Xgap9",   "d,s,m",  	MATCH_FCVT_S_W, 		MASK_FCVT_S_W,	 	match_opcode, 0 },
{"fcvt.s.wu", "Xgap9",   "d,s",  	MATCH_FCVT_S_WU | MASK_RM, 	MASK_FCVT_S_W| MASK_RM, match_opcode, 0 },
{"fcvt.s.wu", "Xgap9",   "d,s,m",  	MATCH_FCVT_S_WU, 		MASK_FCVT_S_WU,		match_opcode, 0 },
{"fclass.s",  "Xgap9",   "d,s",  	MATCH_FCLASS_S, 		MASK_FCLASS_S, 		match_opcode, 0 },
{"feq.s",     "Xgap9",   "d,s,t",    	MATCH_FEQ_S, 			MASK_FEQ_S, 		match_opcode, 0 },
{"flt.s",     "Xgap9",   "d,s,t",    	MATCH_FLT_S, 			MASK_FLT_S, 		match_opcode, 0 },
{"fle.s",     "Xgap9",   "d,s,t",    	MATCH_FLE_S, 			MASK_FLE_S, 		match_opcode, 0 },
{"fgt.s",     "Xgap9",   "d,t,s",    	MATCH_FLT_S, 			MASK_FLT_S, 		match_opcode, 0 },
{"fge.s",     "Xgap9",   "d,t,s",    	MATCH_FLE_S, 			MASK_FLE_S, 		match_opcode, 0 },


/* Gap9 FP16 ALT,  Riscv F based but on fp16alt */
{"fmadd.ah",   "Xgap9",  "d,s,t,e",  	MATCH_FMADD_AH, 		MASK_FMADD_AH, 		match_opcode, 0 },
{"fmsub.ah",   "Xgap9",  "d,s,t,e",  	MATCH_FMSUB_AH, 		MASK_FMSUB_AH, 		match_opcode, 0 },
{"fnmsub.ah",  "Xgap9",  "d,s,t,e",  	MATCH_FNMSUB_AH,		MASK_FNMSUB_AH, 	match_opcode, 0 },
{"fnmadd.ah",  "Xgap9",  "d,s,t,e",  	MATCH_FNMADD_AH, 		MASK_FNMADD_AH, 	match_opcode, 0 },
{"fadd.ah",    "Xgap9",  "d,s,t",  	MATCH_FADD_AH, 			MASK_FADD_AH, 		match_opcode, 0 },
{"fsub.ah",    "Xgap9",  "d,s,t",  	MATCH_FSUB_AH, 			MASK_FSUB_AH, 		match_opcode, 0 },
{"fmul.ah",    "Xgap9",  "d,s,t",  	MATCH_FMUL_AH, 			MASK_FMUL_AH, 		match_opcode, 0 },
{"fdiv.ah",    "Xgap9",  "d,s,t",  	MATCH_FDIV_AH, 			MASK_FDIV_AH, 		match_opcode, 0 },
{"fsqrt.ah",   "Xgap9",  "d,s",  	MATCH_FSQRT_AH, 		MASK_FSQRT_AH, 		match_opcode, 0 },
{"fsgnj.ah",   "Xgap9",  "d,s,t",  	MATCH_FSGNJ_AH, 		MASK_FSGNJ_AH, 		match_opcode, 0 },
{"fsgnjn.ah",  "Xgap9",  "d,s,t",  	MATCH_FSGNJN_AH, 		MASK_FSGNJN_AH, 	match_opcode, 0 },
{"fsgnjx.ah",  "Xgap9",  "d,s,t",  	MATCH_FSGNJX_AH, 		MASK_FSGNJX_AH, 	match_opcode, 0 },
{"fmin.ah",    "Xgap9",  "d,s,t",  	MATCH_FMIN_AH, 			MASK_FMIN_AH, 		match_opcode, 0 },
{"fmax.ah",    "Xgap9",  "d,s,t",  	MATCH_FMAX_AH, 			MASK_FMAX_AH, 		match_opcode, 0 },
{"feq.ah",     "Xgap9",  "d,s,t",    	MATCH_FEQ_AH, 			MASK_FEQ_AH, 		match_opcode, 0 },
{"flt.ah",     "Xgap9",  "d,s,t",    	MATCH_FLT_AH, 			MASK_FLT_AH, 		match_opcode, 0 },
{"fle.ah",     "Xgap9",  "d,s,t",    	MATCH_FLE_AH, 			MASK_FLE_AH, 		match_opcode, 0 },
{"fcvt.w.ah",  "Xgap9",  "d,s",  	MATCH_FCVT_W_AH, 		MASK_FCVT_W_AH, 	match_opcode, 0 },
{"fcvt.wu.ah", "Xgap9",  "d,s",  	MATCH_FCVT_WU_AH, 		MASK_FCVT_WU_AH, 	match_opcode, 0 },
{"fcvt.ah.w",  "Xgap9",  "d,s",  	MATCH_FCVT_AH_W, 		MASK_FCVT_AH_W, 	match_opcode, 0 },
{"fcvt.ah.wu", "Xgap9",  "d,s",  	MATCH_FCVT_AH_WU, 		MASK_FCVT_AH_W, 	match_opcode, 0 },
{"fcvt.s.ah",  "Xgap9",  "d,s",  	MATCH_FCVT_S_AH, 		MASK_FCVT_S_AH, 	match_opcode, 0 },
{"fcvt.ah.s",  "Xgap9",  "d,s",  	MATCH_FCVT_AH_S, 		MASK_FCVT_AH_S, 	match_opcode, 0 },

{"fclass.ah",  "Xgap9",  "d,s",  	MATCH_FCLASS_AH, 		MASK_FCLASS_AH, 	match_opcode, 0 },
// pseudos
{"fgt.ah",     "Xgap9",  "d,t,s",    	MATCH_FLT_AH, 			MASK_FLT_AH, 		match_opcode, 0 },
{"fge.ah",     "Xgap9",  "d,t,s",    	MATCH_FLE_AH, 			MASK_FLE_AH, 		match_opcode, 0 },
{"fabs.ah",    "Xgap9",  "d,w",  	MATCH_FSGNJX_AH, 		MASK_FSGNJX_AH, 	match_rs1_eq_rs2, INSN_ALIAS },
{"fneg.ah",    "Xgap9",  "d,w",  	MATCH_FSGNJN_AH, 		MASK_FSGNJN_AH, 	match_rs1_eq_rs2, INSN_ALIAS },

/* Gap9 FP16,  Riscv F based but on fp16 */
{"fmadd.h",   "Xgap9",   "d,s,t,e",  	MATCH_FMADD_H | MASK_RM, 	MASK_FMADD_H | MASK_RM,	match_opcode,   0},
{"fmadd.h",   "Xgap9",   "d,s,t,e,m",  	MATCH_FMADD_H, 			MASK_FMADD_H, 		match_opcode,   0},
{"fmsub.h",   "Xgap9",   "d,s,t,e",  	MATCH_FMSUB_H | MASK_RM, 	MASK_FMSUB_H | MASK_RM, match_opcode,   0},
{"fmsub.h",   "Xgap9",   "d,s,t,e,m",  	MATCH_FMSUB_H, 			MASK_FMSUB_H, 		match_opcode,   0},
{"fnmsub.h",  "Xgap9",   "d,s,t,e",  	MATCH_FNMSUB_H | MASK_RM, 	MASK_FNMSUB_H|MASK_RM,	match_opcode,   0},
{"fnmsub.h",  "Xgap9",   "d,s,t,e,m",  	MATCH_FNMSUB_H, 		MASK_FNMSUB_H, 		match_opcode,   0},
{"fnmadd.h",  "Xgap9",   "d,s,t,e",  	MATCH_FNMADD_H | MASK_RM, 	MASK_FNMADD_H|MASK_RM,	match_opcode,   0},
{"fnmadd.h",  "Xgap9",   "d,s,t,e,m",  	MATCH_FNMADD_H, 		MASK_FNMADD_H, 		match_opcode,   0},
{"fadd.h",    "Xgap9",   "d,s,t",  	MATCH_FADD_H | MASK_RM, 	MASK_FADD_H | MASK_RM, 	match_opcode,   0},
{"fadd.h",    "Xgap9",   "d,s,t,m",  	MATCH_FADD_H, 			MASK_FADD_H, 		match_opcode,   0},
{"fsub.h",    "Xgap9",   "d,s,t",  	MATCH_FSUB_H | MASK_RM, 	MASK_FSUB_H | MASK_RM, 	match_opcode,   0},
{"fsub.h",    "Xgap9",   "d,s,t,m",  	MATCH_FSUB_H, 			MASK_FSUB_H, 		match_opcode,   0},
{"fmul.h",    "Xgap9",   "d,s,t",  	MATCH_FMUL_H | MASK_RM, 	MASK_FMUL_H | MASK_RM, 	match_opcode,   0},
{"fmul.h",    "Xgap9",   "d,s,t,m",  	MATCH_FMUL_H, 			MASK_FMUL_H, 		match_opcode,   0},
{"fdiv.h",    "Xgap9",   "d,s,t",  	MATCH_FDIV_H | MASK_RM, 	MASK_FDIV_H | MASK_RM, 	match_opcode,   0},
{"fdiv.h",    "Xgap9",   "d,s,t,m",  	MATCH_FDIV_H, 			MASK_FDIV_H, 		match_opcode,   0},
{"fsqrt.h",   "Xgap9",   "d,s",  	MATCH_FSQRT_H | MASK_RM, 	MASK_FSQRT_H | MASK_RM, match_opcode,  	0},
{"fsqrt.h",   "Xgap9",   "d,s,m",  	MATCH_FSQRT_H, 			MASK_FSQRT_H, 		match_opcode,  	0},
{"fsgnj.h",   "Xgap9",   "d,s,t",  	MATCH_FSGNJ_H, 			MASK_FSGNJ_H, 		match_opcode,   0},
{"fsgnjn.h",  "Xgap9",   "d,s,t",  	MATCH_FSGNJN_H, 		MASK_FSGNJN_H, 		match_opcode,   0},
{"fsgnjx.h",  "Xgap9",   "d,s,t",  	MATCH_FSGNJX_H, 		MASK_FSGNJX_H, 		match_opcode,   0},
{"fmin.h",    "Xgap9",   "d,s,t",  	MATCH_FMIN_H, 			MASK_FMIN_H, 		match_opcode,   0},
{"fmax.h",    "Xgap9",   "d,s,t",  	MATCH_FMAX_H, 			MASK_FMAX_H, 		match_opcode,   0},
{"feq.h",     "Xgap9",   "d,s,t",    	MATCH_FEQ_H, 			MASK_FEQ_H, 		match_opcode,  	0},
{"flt.h",     "Xgap9",   "d,s,t",    	MATCH_FLT_H, 			MASK_FLT_H, 		match_opcode,  	0},
{"fle.h",     "Xgap9",   "d,s,t",    	MATCH_FLE_H, 			MASK_FLE_H, 		match_opcode,  	0},

{"fcvt.w.h",  "Xgap9",   "d,s",  	MATCH_FCVT_W_H | MASK_RM, 	MASK_FCVT_W_H|MASK_RM,	match_opcode,  	0},
{"fcvt.w.h",  "Xgap9",   "d,s,m",  	MATCH_FCVT_W_H, 		MASK_FCVT_W_H, 		match_opcode,  	0},
{"fcvt.wu.h", "Xgap9",   "d,s",  	MATCH_FCVT_WU_H | MASK_RM, 	MASK_FCVT_WU_H|MASK_RM, match_opcode,  	0},
{"fcvt.wu.h", "Xgap9",   "d,s,m",  	MATCH_FCVT_WU_H, 		MASK_FCVT_WU_H, 	match_opcode,  	0},
{"fcvt.h.w",  "Xgap9",   "d,s",  	MATCH_FCVT_H_W | MASK_RM, 	MASK_FCVT_H_W|MASK_RM, 	match_opcode,   0},
{"fcvt.h.w",  "Xgap9",   "d,s,m",  	MATCH_FCVT_H_W, 		MASK_FCVT_H_W, 		match_opcode,   0},
{"fcvt.h.wu", "Xgap9",   "d,s",  	MATCH_FCVT_H_WU | MASK_RM, 	MASK_FCVT_H_W|MASK_RM, 	match_opcode,   0},
{"fcvt.h.wu", "Xgap9",   "d,s,m",  	MATCH_FCVT_H_WU, 		MASK_FCVT_H_WU, 	match_opcode,   0},
{"fcvt.s.h",  "Xgap9",   "d,s",  	MATCH_FCVT_S_H, 		MASK_FCVT_S_H, 		match_opcode,   0},
{"fcvt.h.s",  "Xgap9",   "d,s",  	MATCH_FCVT_H_S | MASK_RM, 	MASK_FCVT_H_S|MASK_RM, 	match_opcode,   0},
{"fcvt.h.s",  "Xgap9",   "d,s,m",  	MATCH_FCVT_H_S, 		MASK_FCVT_H_S, 		match_opcode,   0},

{"fclass.h",  "Xgap9",   "d,s",  	MATCH_FCLASS_H, 		MASK_FCLASS_H, 		match_opcode,   0},

// pseudos
{"fgt.h",     "Xgap9",   "d,t,s",    	MATCH_FLT_H, 			MASK_FLT_H, 		match_opcode,  INSN_ALIAS },
{"fge.h",     "Xgap9",   "d,t,s",    	MATCH_FLE_H, 			MASK_FLE_H, 		match_opcode,  INSN_ALIAS },
{"fabs.h",    "Xgap9",   "d,w",  	MATCH_FSGNJX_H, 		MASK_FSGNJX_H, 		match_rs1_eq_rs2,   INSN_ALIAS },
{"fneg.h",    "Xgap9",   "d,w",  	MATCH_FSGNJN_H, 		MASK_FSGNJN_H, 		match_rs1_eq_rs2,   INSN_ALIAS },



/* Gap9 FP16 ALT <-> Fp16 */
{"fcvt.h.ah", 	"Xgap9",  "d,s"  ,  	MATCH_FCVT_H_AH | MASK_RM, 	MASK_FCVT_H_AH|MASK_RM, match_opcode, 0 },
{"fcvt.h.ah", 	"Xgap9",  "d,s,m",  	MATCH_FCVT_H_AH, 		MASK_FCVT_H_AH, 	match_opcode, 0 },
{"fcvt.ah.h", 	"Xgap9",  "d,s",    	MATCH_FCVT_AH_H, 		MASK_FCVT_AH_H, 	match_opcode, 0 },

/* Gap9 FP16 ALT,  Fp16Alt, Fp16Alt => Fp32 */
{"fmulex.s.ah", "Xgap9", "d,s,t", 	MATCH_FMULEX_S_AH, 		MASK_FMULEX_S_AH, 	match_opcode, 0},
{"fmacex.s.ah", "Xgap9", "d,s,t", 	MATCH_FMACEX_S_AH, 		MASK_FMACEX_S_AH, 	match_opcode, 0},


/* Gap9 SIMD2 FP16ALT */
{"vfadd.ah",    "Xgap9", "d,s,t",   	MATCH_VFADD_AH, 		MASK_VFADD_AH, 		match_opcode, 0},
{"vfadd.r.ah",  "Xgap9", "d,s,t",   	MATCH_VFADD_R_AH, 		MASK_VFADD_R_AH, 	match_opcode, 0},
{"vfsub.ah",    "Xgap9", "d,s,t",   	MATCH_VFSUB_AH, 		MASK_VFSUB_AH, 		match_opcode, 0},
{"vfsub.r.ah",  "Xgap9", "d,s,t",   	MATCH_VFSUB_R_AH, 		MASK_VFSUB_R_AH, 	match_opcode, 0},
{"vfmul.ah",    "Xgap9", "d,s,t",   	MATCH_VFMUL_AH, 		MASK_VFMUL_AH, 		match_opcode, 0},
{"vfmul.r.ah",  "Xgap9", "d,s,t",   	MATCH_VFMUL_R_AH, 		MASK_VFMUL_R_AH, 	match_opcode, 0},
// {"vfdiv.ah",    "Xgap9", "d,s,t",   	MATCH_VFDIV_AH, 		MASK_VFDIV_AH, 		match_opcode, 0},
// {"vfdiv.r.ah",  "Xgap9", "d,s,t",   	MATCH_VFDIV_R_AH, 		MASK_VFDIV_R_AH, 	match_opcode, 0},
{"vfmin.ah",    "Xgap9", "d,s,t",   	MATCH_VFMIN_AH, 		MASK_VFMIN_AH, 		match_opcode, 0},
{"vfmin.r.ah",  "Xgap9", "d,s,t",   	MATCH_VFMIN_R_AH, 		MASK_VFMIN_R_AH, 	match_opcode, 0},
{"vfmax.ah",    "Xgap9", "d,s,t",   	MATCH_VFMAX_AH, 		MASK_VFMAX_AH, 		match_opcode, 0},
{"vfmax.r.ah",  "Xgap9", "d,s,t",   	MATCH_VFMAX_R_AH, 		MASK_VFMAX_R_AH, 	match_opcode, 0},
// {"vfsqrt.ah",   "Xgap9", "d,s",     	MATCH_VFSQRT_AH, 		MASK_VFSQRT_AH, 	match_opcode, 0},
{"vfmac.ah",    "Xgap9", "d,s,t",   	MATCH_VFMAC_AH, 		MASK_VFMAC_AH, 		match_opcode, 0},
{"vfmac.r.ah",  "Xgap9", "d,s,t",   	MATCH_VFMAC_R_AH, 		MASK_VFMAC_R_AH, 	match_opcode, 0},
{"vfmre.ah",    "Xgap9", "d,s,t",   	MATCH_VFMRE_AH, 		MASK_VFMRE_AH, 		match_opcode, 0},
{"vfmre.r.ah",  "Xgap9", "d,s,t",   	MATCH_VFMRE_R_AH, 		MASK_VFMRE_R_AH, 	match_opcode, 0},
{"vfclass.ah",  "Xgap9", "d,s",     	MATCH_VFCLASS_AH, 		MASK_VFCLASS_AH, 	match_opcode, 0},
{"vfsgnj.ah",   "Xgap9", "d,s,t",   	MATCH_VFSGNJ_AH, 		MASK_VFSGNJ_AH, 	match_opcode, 0},
{"vfsgnj.r.ah", "Xgap9", "d,s,t",   	MATCH_VFSGNJ_R_AH, 		MASK_VFSGNJ_R_AH, 	match_opcode, 0},
{"vfsgnjn.ah",  "Xgap9", "d,s,t",   	MATCH_VFSGNJN_AH, 		MASK_VFSGNJN_AH, 	match_opcode, 0},
{"vfsgnjn.r.ah","Xgap9", "d,s,t",   	MATCH_VFSGNJN_R_AH, 		MASK_VFSGNJN_R_AH, 	match_opcode, 0},
{"vfsgnjx.ah",  "Xgap9", "d,s,t",   	MATCH_VFSGNJX_AH, 		MASK_VFSGNJX_AH, 	match_opcode, 0},
{"vfsgnjx.r.ah","Xgap9", "d,s,t",   	MATCH_VFSGNJX_R_AH, 		MASK_VFSGNJX_R_AH, 	match_opcode, 0},
{"vfeq.ah",     "Xgap9", "d,s,t", 	MATCH_VFEQ_AH, 			MASK_VFEQ_AH, 		match_opcode, 0},
{"vfeq.r.ah",   "Xgap9", "d,s,t",   	MATCH_VFEQ_R_AH, 		MASK_VFEQ_R_AH, 	match_opcode, 0},
{"vfne.ah",     "Xgap9", "d,s,t", 	MATCH_VFNE_AH, 			MASK_VFNE_AH, 		match_opcode, 0},
{"vfne.r.ah",   "Xgap9", "d,s,t",   	MATCH_VFNE_R_AH, 		MASK_VFNE_R_AH, 	match_opcode, 0},
{"vflt.ah",     "Xgap9", "d,s,t", 	MATCH_VFLT_AH, 			MASK_VFLT_AH, 		match_opcode, 0},
{"vflt.r.ah",   "Xgap9", "d,s,t",   	MATCH_VFLT_R_AH, 		MASK_VFLT_R_AH, 	match_opcode, 0},
{"vfge.ah",     "Xgap9", "d,s,t", 	MATCH_VFGE_AH, 			MASK_VFGE_AH, 		match_opcode, 0},
{"vfge.r.ah",   "Xgap9", "d,s,t",   	MATCH_VFGE_R_AH, 		MASK_VFGE_R_AH, 	match_opcode, 0},
{"vfle.ah",     "Xgap9", "d,s,t", 	MATCH_VFLE_AH, 			MASK_VFLE_AH, 		match_opcode, 0},
{"vfle.r.ah",   "Xgap9", "d,s,t",   	MATCH_VFLE_R_AH, 		MASK_VFLE_R_AH, 	match_opcode, 0},
{"vfgt.ah",     "Xgap9", "d,s,t",   	MATCH_VFGT_AH, 			MASK_VFGT_AH, 		match_opcode, 0},
{"vfgt.r.ah",   "Xgap9", "d,s,t",   	MATCH_VFGT_R_AH, 		MASK_VFGT_R_AH, 	match_opcode, 0},
{"vfcpka.ah.s", "Xgap9", "d,s,t",   	MATCH_VFCPKA_AH_S, 		MASK_VFCPKA_AH_S, 	match_opcode, 0},

{"vfcvt.x.ah",  "Xgap9", "d,s",     	MATCH_VFCVT_X_AH, 		MASK_VFCVT_X_AH, 	match_opcode, 0},
{"vfcvt.xu.ah", "Xgap9", "d,s",     	MATCH_VFCVT_XU_AH, 		MASK_VFCVT_XU_AH, 	match_opcode, 0},
{"vfcvt.ah.x",  "Xgap9", "d,s",     	MATCH_VFCVT_AH_X, 		MASK_VFCVT_AH_X, 	match_opcode, 0},
{"vfcvt.ah.xu", "Xgap9", "d,s",     	MATCH_VFCVT_AH_XU, 		MASK_VFCVT_AH_XU, 	match_opcode, 0},

/* Pseudos */
{"vfabs.ah",    "Xgap9",   "d,w",  	MATCH_VFSGNJX_AH, 		MASK_VFSGNJX_AH, 	match_rs1_eq_rs2,   INSN_ALIAS },
{"vfneg.ah",    "Xgap9",   "d,w",  	MATCH_VFSGNJN_AH, 		MASK_VFSGNJN_AH, 	match_rs1_eq_rs2,   INSN_ALIAS },


/* Gap9 SIMD2 FP16 */
{"vfadd.h",	"Xgap9", "d,s,t", 	MATCH_VFADD_H, 			MASK_VFADD_H, 		match_opcode, 0},
{"vfadd.r.h",	"Xgap9", "d,s,t",   	MATCH_VFADD_R_H, 		MASK_VFADD_R_H, 	match_opcode, 0},
{"vfsub.h",	"Xgap9", "d,s,t", 	MATCH_VFSUB_H, 			MASK_VFSUB_H, 		match_opcode, 0},
{"vfsub.r.h",	"Xgap9", "d,s,t",   	MATCH_VFSUB_R_H, 		MASK_VFSUB_R_H, 	match_opcode, 0},
{"vfmul.h",	"Xgap9", "d,s,t", 	MATCH_VFMUL_H, 			MASK_VFMUL_H, 		match_opcode, 0},
{"vfmul.r.h",	"Xgap9", "d,s,t",   	MATCH_VFMUL_R_H, 		MASK_VFMUL_R_H, 	match_opcode, 0},
// {"vfdiv.h",	"Xgap9", "d,s,t", 	MATCH_VFDIV_H, 			MASK_VFDIV_H, 		match_opcode, 0},
// {"vfdiv.r.h",	"Xgap9", "d,s,t",   	MATCH_VFDIV_R_H, 		MASK_VFDIV_R_H, 	match_opcode, 0},
{"vfmin.h",	"Xgap9", "d,s,t", 	MATCH_VFMIN_H, 			MASK_VFMIN_H, 		match_opcode, 0},
{"vfmin.r.h",	"Xgap9", "d,s,t",   	MATCH_VFMIN_R_H, 		MASK_VFMIN_R_H, 	match_opcode, 0},
{"vfmax.h",	"Xgap9", "d,s,t", 	MATCH_VFMAX_H, 			MASK_VFMAX_H, 		match_opcode, 0},
{"vfmax.r.h",	"Xgap9", "d,s,t",   	MATCH_VFMAX_R_H, 		MASK_VFMAX_R_H, 	match_opcode, 0},
// {"vfsqrt.h",	"Xgap9", "d,s",     	MATCH_VFSQRT_H, 		MASK_VFSQRT_H, 		match_opcode, 0},
{"vfmac.h",	"Xgap9", "d,s,t",       MATCH_VFMAC_H, 			MASK_VFMAC_H, 		match_opcode, 0},
{"vfmac.r.h",	"Xgap9", "d,s,t",   	MATCH_VFMAC_R_H, 		MASK_VFMAC_R_H, 	match_opcode, 0},
{"vfmre.h",	"Xgap9", "d,s,t", 	MATCH_VFMRE_H, 			MASK_VFMRE_H, 		match_opcode, 0},
{"vfmre.r.h",	"Xgap9", "d,s,t",   	MATCH_VFMRE_R_H, 		MASK_VFMRE_R_H, 	match_opcode, 0},
{"vfclass.h",	"Xgap9", "d,s",     	MATCH_VFCLASS_H, 		MASK_VFCLASS_H, 	match_opcode, 0},
{"vfsgnj.h",	"Xgap9", "d,s,t",   	MATCH_VFSGNJ_H, 		MASK_VFSGNJ_H, 		match_opcode, 0},
{"vfsgnj.r.h",	"Xgap9", "d,s,t",   	MATCH_VFSGNJ_R_H, 		MASK_VFSGNJ_R_H, 	match_opcode, 0},
{"vfsgnjn.h",	"Xgap9", "d,s,t",   	MATCH_VFSGNJN_H, 		MASK_VFSGNJN_H, 	match_opcode, 0},
{"vfsgnjn.r.h",	"Xgap9", "d,s,t",   	MATCH_VFSGNJN_R_H, 		MASK_VFSGNJN_R_H, 	match_opcode, 0},
{"vfsgnjx.h",	"Xgap9", "d,s,t",   	MATCH_VFSGNJX_H, 		MASK_VFSGNJX_H, 	match_opcode, 0},
{"vfsgnjx.r.h",	"Xgap9", "d,s,t",   	MATCH_VFSGNJX_R_H, 		MASK_VFSGNJX_R_H, 	match_opcode, 0},
{"vfeq.h",	"Xgap9", "d,s,t", 	MATCH_VFEQ_H, 			MASK_VFEQ_H, 		match_opcode, 0},
{"vfeq.r.h",	"Xgap9", "d,s,t",   	MATCH_VFEQ_R_H, 		MASK_VFEQ_R_H, 		match_opcode, 0},
{"vfne.h",	"Xgap9", "d,s,t",   	MATCH_VFNE_H, 			MASK_VFNE_H, 		match_opcode, 0},
{"vfne.r.h",	"Xgap9", "d,s,t",   	MATCH_VFNE_R_H, 		MASK_VFNE_R_H, 		match_opcode, 0},
{"vflt.h",	"Xgap9", "d,s,t", 	MATCH_VFLT_H, 			MASK_VFLT_H, 		match_opcode, 0},
{"vflt.r.h",	"Xgap9", "d,s,t",   	MATCH_VFLT_R_H, 		MASK_VFLT_R_H, 		match_opcode, 0},
{"vfge.h",	"Xgap9", "d,s,t", 	MATCH_VFGE_H, 			MASK_VFGE_H, 		match_opcode, 0},
{"vfge.r.h",	"Xgap9", "d,s,t",   	MATCH_VFGE_R_H, 		MASK_VFGE_R_H, 		match_opcode, 0},
{"vfle.h",	"Xgap9", "d,s,t", 	MATCH_VFLE_H, 			MASK_VFLE_H, 		match_opcode, 0},
{"vfle.r.h",	"Xgap9", "d,s,t",   	MATCH_VFLE_R_H, 		MASK_VFLE_R_H, 		match_opcode, 0},
{"vfgt.h",	"Xgap9", "d,s,t", 	MATCH_VFGT_H, 			MASK_VFGT_H, 		match_opcode, 0},
{"vfgt.r.h",	"Xgap9", "d,s,t",   	MATCH_VFGT_R_H, 		MASK_VFGT_R_H, 		match_opcode, 0},
{"vfcpka.h.s",  "Xgap9", "d,s,t",   	MATCH_VFCPKA_H_S, 		MASK_VFCPKA_H_S, 	match_opcode, 0},

{"vfcvt.x.h",   "Xgap9", "d,s",     	MATCH_VFCVT_X_H, 		MASK_VFCVT_X_H, 	match_opcode, 0},
{"vfcvt.xu.h",  "Xgap9", "d,s",     	MATCH_VFCVT_XU_H, 		MASK_VFCVT_XU_H, 	match_opcode, 0},
{"vfcvt.h.x",   "Xgap9", "d,s",     	MATCH_VFCVT_H_X, 		MASK_VFCVT_H_X, 	match_opcode, 0},
{"vfcvt.h.xu",  "Xgap9", "d,s",     	MATCH_VFCVT_H_XU, 		MASK_VFCVT_H_XU, 	match_opcode, 0},

/* Pseudos */
{"vfabs.h",    	"Xgap9", "d,w",  	MATCH_VFSGNJX_H, 		MASK_VFSGNJX_H, 	match_rs1_eq_rs2,   INSN_ALIAS },
{"vfneg.h",    	"Xgap9", "d,w",  	MATCH_VFSGNJN_H, 		MASK_VFSGNJN_H, 	match_rs1_eq_rs2,   INSN_ALIAS },


/* SIMD2 FP16 <-> FP16ALT */
{"vfcvt.h.ah",  "Xgap9", "d,s",     	MATCH_VFCVT_H_AH, 		MASK_VFCVT_H_AH, 	match_opcode, 0},
{"vfcvt.ah.h",  "Xgap9", "d,s",     	MATCH_VFCVT_AH_H, 		MASK_VFCVT_AH_H, 	match_opcode, 0},


/***********************************************************************************************************************/
/**************************************     End of Gap9              ***************************************************/
/***********************************************************************************************************************/



/* Terminate the list.  */
{0, 0, 0, 0, 0, 0, 0}
};
