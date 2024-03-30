#include "util.h"

Inst all_nops()
{
  return __pack_mininsts(SMC_NOP(), SMC_NOP(), SMC_NOP(), SMC_NOP());
}
Program PRE(int bank_reg, int ibar, int pall)
{
  Program p;
  p.add_inst(
      SMC_PRE(bank_reg, ibar, pall),
      SMC_NOP(), SMC_NOP(), SMC_NOP());
      p.add_inst(SMC_SLEEP(4));

 // p.add_inst(all_nops());
 // p.add_inst(all_nops());
 // p.add_inst(all_nops());
  return p;
}
Program ACT(int bank_reg, int ibar, int row_reg, int irar)
{
  // ACT & wait for tRCD
  Program p;
  p.add_inst(
      SMC_ACT(bank_reg, ibar, row_reg, irar),
      SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(SMC_SLEEP(4));

  return p;
}
Program WRITE(int bank_reg, int col_reg, int icar)
{
  // ACT & wait for tRCD
  Program p;
  p.add_inst(
      SMC_WRITE(bank_reg, 0, col_reg, icar, 0, 0),
      SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(all_nops());
  return p;
}

Program READ(int bank_reg, int col_reg, int icar)
{
  Program p;
  p.add_inst(
      SMC_READ(bank_reg, 0, col_reg, icar, 0, 0),
      SMC_NOP(), SMC_NOP(), SMC_NOP());
  p.add_inst(all_nops());
  return p;
}
Program wrRow_base_offset_label(int bank_reg, uint32_t row_base, uint32_t row_offset, uint32_t wr_pattern,int label)
{
  Program p;
  p.add_inst(SMC_LI(wr_pattern, PATTERN_REG)); // s
  for (int i = 0; i < 16; i++)
    p.add_inst(SMC_LDWD(PATTERN_REG, i));

  p.add_inst(SMC_ADDI(row_base, row_offset, RAR));
  p.add_inst(SMC_LI(0, CAR));
  p.add_inst(SMC_LI(0, LOOP_COLS));
  p.add_inst(SMC_LI(128, NUM_COLS_REG));
  p.add_below(PRE(bank_reg, 0, 0));
  p.add_below(ACT(bank_reg, 0, RAR, 0));
  p.add_label("WR_ROW_BO_" + std::to_string(label));
    p.add_below(WRITE(bank_reg, CAR, 1));
    p.add_inst(SMC_ADDI(LOOP_COLS, 1, LOOP_COLS));
  p.add_branch(p.BR_TYPE::BL, LOOP_COLS, NUM_COLS_REG, "WR_ROW_BO_" + std::to_string(label));
  p.add_inst(all_nops());

  return p;
}
Program wrRow_base_offset(int bank_reg, uint32_t row_base, uint32_t row_offset, uint32_t wr_pattern)
{
  Program p;
  p.add_inst(SMC_LI(wr_pattern, PATTERN_REG)); // s
  for (int i = 0; i < 16; i++)
    p.add_inst(SMC_LDWD(PATTERN_REG, i));

  
  p.add_inst(SMC_ADDI(row_base, row_offset, RAR));
  p.add_inst(SMC_LI(0, CAR));
  p.add_below(PRE(bank_reg, 0, 0));
  p.add_below(ACT(bank_reg, 0, RAR, 0));
  for(int i = 0 ; i < 32 ; i++)
  {
    p.add_inst(SMC_WRITE(bank_reg, 0, CAR, 1, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
    p.add_inst(SMC_NOP(), SMC_WRITE(bank_reg, 0, CAR, 1, 0, 0), SMC_NOP(), SMC_NOP());
    p.add_inst(SMC_NOP(), SMC_NOP(), SMC_WRITE(bank_reg, 0, CAR, 1, 0, 0), SMC_NOP());
    p.add_inst(SMC_NOP(), SMC_NOP(), SMC_NOP(), SMC_WRITE(bank_reg, 0, CAR, 1, 0, 0));
    p.add_inst(SMC_NOP(),SMC_NOP(),SMC_NOP(),SMC_NOP());
    p.add_inst(all_nops());
  }
  p.add_inst(all_nops());
  return p;
}

Program wrRow_immediate(int bank_reg, uint32_t row_immd, uint32_t wr_pattern)
{
  Program p;
  p.add_inst(SMC_LI(wr_pattern, PATTERN_REG)); // s
  for (int i = 0; i < 16; i++)
    p.add_inst(SMC_LDWD(PATTERN_REG, i));

  p.add_inst(SMC_LI(row_immd, RAR));
  p.add_inst(SMC_LI(0, CAR));
  p.add_below(PRE(bank_reg, 0, 0));
  p.add_below(ACT(bank_reg, 0, RAR, 0));
  for(int i = 0 ; i < 32 ; i++)
  {
    p.add_inst(SMC_WRITE(bank_reg, 0, CAR, 1, 0, 0), SMC_NOP(), SMC_NOP(), SMC_NOP());
    p.add_inst(SMC_NOP(), SMC_WRITE(bank_reg, 0, CAR, 1, 0, 0), SMC_NOP(), SMC_NOP());
    p.add_inst(SMC_NOP(), SMC_NOP(), SMC_WRITE(bank_reg, 0, CAR, 1, 0, 0), SMC_NOP());
    p.add_inst(SMC_NOP(), SMC_NOP(), SMC_NOP(), SMC_WRITE(bank_reg, 0, CAR, 1, 0, 0));
    p.add_inst(SMC_NOP(),SMC_NOP(),SMC_NOP(),SMC_NOP());
    p.add_inst(all_nops());
  }
  p.add_inst(all_nops());
  return p;
}
Program wrRow_immediate_label(int bank_reg, uint32_t row_immd, uint32_t wr_pattern,int label)
{
  Program p;
  p.add_inst(SMC_LI(wr_pattern, PATTERN_REG)); // s
  for (int i = 0; i < 16; i++)
    p.add_inst(SMC_LDWD(PATTERN_REG, i));

  p.add_inst(SMC_LI(row_immd, RAR));
  p.add_inst(SMC_LI(0, CAR));
  p.add_inst(SMC_LI(0, LOOP_COLS));
  p.add_inst(SMC_LI(128, NUM_COLS_REG));

  p.add_below(PRE(bank_reg, 0, 0));
  p.add_below(ACT(bank_reg, 0, RAR, 0));
  p.add_label("WR_ROW_IMMD_" + std::to_string(label));
    p.add_below(WRITE(bank_reg, CAR, 1));
    p.add_inst(SMC_ADDI(LOOP_COLS, 1, LOOP_COLS));
  p.add_branch(p.BR_TYPE::BL, LOOP_COLS, NUM_COLS_REG, "WR_ROW_IMMD_" + std::to_string(label));
  p.add_inst(all_nops());

  return p;
}

Program rdRow_immediate(int bank_reg, uint32_t row_immd)
{
  Program p;

  p.add_inst(SMC_LI(row_immd, RAR));
  p.add_inst(SMC_LI(0, CAR));
  p.add_inst(SMC_LI(0, LOOP_COLS));
  p.add_inst(SMC_LI(128, NUM_COLS_REG));
  p.add_below(PRE(bank_reg, 0, 0));
  p.add_below(ACT(bank_reg, 0, RAR, 0));
  p.add_inst(SMC_SLEEP(4));
  p.add_inst(SMC_ADDI(LOOP_COLS, 0, LOOP_COLS));
  p.add_inst(SMC_ADDI(LOOP_COLS, 0, LOOP_COLS));
  p.add_label("READ_ROW_IMMD");
  p.add_below(READ(bank_reg, CAR, 1));
  p.add_inst(SMC_SLEEP(4));
  p.add_inst(SMC_ADDI(LOOP_COLS, 1, LOOP_COLS));
  p.add_branch(p.BR_TYPE::BL, LOOP_COLS, NUM_COLS_REG,
               "READ_ROW_IMMD");
  p.add_inst(all_nops());
  return p;
}
Program rdRow_base_offset(int bank_reg, uint32_t row_base, uint32_t row_offset)
{
  Program p;

  p.add_inst(SMC_ADDI(row_base, row_offset, RAR));
  p.add_inst(SMC_LI(0, CAR));
  p.add_inst(SMC_LI(0, LOOP_COLS));
  p.add_inst(SMC_LI(128, NUM_COLS_REG));

  p.add_below(PRE(bank_reg, 0, 0));
  p.add_below(ACT(bank_reg, 0, RAR, 0));
  p.add_inst(SMC_SLEEP(4));
  p.add_inst(SMC_ADDI(LOOP_COLS, 0, LOOP_COLS));
  p.add_inst(SMC_ADDI(LOOP_COLS, 0, LOOP_COLS));
  p.add_label("READ_ROW_B+O");
  p.add_below(READ(bank_reg, CAR, 1));
  p.add_inst(SMC_SLEEP(4));
  p.add_inst(SMC_ADDI(LOOP_COLS, 1, LOOP_COLS));
  p.add_branch(p.BR_TYPE::BL, LOOP_COLS, NUM_COLS_REG,
               "READ_ROW_B+O");

  return p;
}
Program rdRow_immediate_label(int bank_reg, uint32_t row_immd, int label)
{
  Program p;

  p.add_inst(SMC_LI(row_immd, RAR));
  p.add_inst(SMC_LI(0, CAR));
  p.add_inst(SMC_LI(0, LOOP_COLS));
  p.add_inst(SMC_LI(128, NUM_COLS_REG));
  p.add_below(PRE(bank_reg, 0, 0));
  p.add_below(ACT(bank_reg, 0, RAR, 0));
  p.add_inst(SMC_SLEEP(4));
  p.add_inst(SMC_ADDI(LOOP_COLS, 0, LOOP_COLS));
  p.add_inst(SMC_ADDI(LOOP_COLS, 0, LOOP_COLS));
  p.add_label("READ_ROW_IMMD_" + std::to_string(label));
  p.add_below(READ(bank_reg, CAR, 1));
  p.add_inst(SMC_SLEEP(4));
  p.add_inst(SMC_ADDI(LOOP_COLS, 1, LOOP_COLS));
  p.add_branch(p.BR_TYPE::BL, LOOP_COLS, NUM_COLS_REG,
               "READ_ROW_IMMD_" + std::to_string(label));
  p.add_inst(all_nops());
  return p;
}
Program rdCacheLine_immediate(int bank_reg, uint32_t row_immd, uint32_t cell_idx)
{
  Program p;
  p.add_inst(SMC_LI(row_immd, RAR));
  p.add_below(PRE(bank_reg, 0, 0));
  p.add_below(ACT(bank_reg, 0, RAR, 0));
  p.add_inst(SMC_LI((cell_idx/512)*8, CAR));
  p.add_inst(SMC_SLEEP(4));
  p.add_below(READ(bank_reg, CAR, 0));
  p.add_inst(SMC_SLEEP(4));


  return p;
}

Program doubleACT(int t_12, int t_23, int r_first, int r_second)
{

  Program p;
  int bank_reg = BAR;
  p.add_inst(all_nops());
  p.add_inst(SMC_LI(r_first, RF_REG));
  //p.add_inst(SMC_ADDI(row_reg, r0, R0_REG));
  int RS_REG  = LOOP_COLS;
  p.add_inst(SMC_LI(r_second, LOOP_COLS));

  /*       --Bank -> bank_reg--
  *        | 00 -------------- |-> r0_reg
  *        | 01 -------------- |
  *        | 10 -------------- |
  *        | 11 -------------- |-> r3_reg
  *
  * Cmd:        ----|ACT R0|----| PRE |----|ACT R3|----FINISH
  * Time:     T0----|  T1  |----| T2  |----|  T3  |----------
  * Interval: -------------|t_12|-----|t_23|-----------------
  */

  // act - pre - act -> 3
  // enough nops -> t_12 + t_23
  int num_cmd = 3 + t_12 + t_23;
  // it needs to be divided by 4
  // ex: num_cmd 11 -> 12
  // num_cmd%4 =3 4-num_cmd
  num_cmd += 4 - (num_cmd % 4);
  Mininst q_inst[num_cmd];
  for (int i = 0; i < num_cmd; i++)
    q_inst[i] = SMC_NOP();
  //q_inst[0] = SMC_ACT(bank_reg, 0, R0_REG, 0);
  q_inst[0] = SMC_ACT(bank_reg, 0, RF_REG, 0);
  q_inst[t_12 + 1] = SMC_PRE(bank_reg, 0, 0);
  //q_inst[2 + t_12 + t_23] = SMC_ACT(bank_reg, 0, LOOP_COLS, 0);
  q_inst[2 + t_12 + t_23] = SMC_ACT(bank_reg, 0, LOOP_COLS, 0);

  for (int i = 0; i < num_cmd; i += 4)
    p.add_inst(q_inst[i], q_inst[i + 1], q_inst[i + 2], q_inst[i + 3]);


  return p;
}
Program doubleACT_immd_reg(int t_12, int t_23, int r_first, int r_second_reg)
{

  Program p;
  int bank_reg = BAR;
  p.add_inst(all_nops());
  p.add_inst(SMC_LI(r_first, RF_REG));
  //p.add_inst(SMC_ADDI(row_reg, r0, R0_REG));
  int RS_REG  = LOOP_COLS;
  //p.add_inst(SMC_LI(r_second, LOOP_COLS));

  /*       --Bank -> bank_reg--
  *        | 00 -------------- |-> r0_reg
  *        | 01 -------------- |
  *        | 10 -------------- |
  *        | 11 -------------- |-> r3_reg
  *
  * Cmd:        ----|ACT R0|----| PRE |----|ACT R3|----FINISH
  * Time:     T0----|  T1  |----| T2  |----|  T3  |----------
  * Interval: -------------|t_12|-----|t_23|-----------------
  */

  // act - pre - act -> 3
  // enough nops -> t_12 + t_23
  int num_cmd = 3 + t_12 + t_23;
  // it needs to be divided by 4
  // ex: num_cmd 11 -> 12
  // num_cmd%4 =3 4-num_cmd
  num_cmd += 4 - (num_cmd % 4);
  Mininst q_inst[num_cmd];
  for (int i = 0; i < num_cmd; i++)
    q_inst[i] = SMC_NOP();
  //q_inst[0] = SMC_ACT(bank_reg, 0, R0_REG, 0);
  q_inst[0] = SMC_ACT(bank_reg, 0, RF_REG, 0);
  q_inst[t_12 + 1] = SMC_PRE(bank_reg, 0, 0);
  //q_inst[2 + t_12 + t_23] = SMC_ACT(bank_reg, 0, LOOP_COLS, 0);
  q_inst[2 + t_12 + t_23] = SMC_ACT(bank_reg, 0, r_second_reg, 0);

  for (int i = 0; i < num_cmd; i += 4)
    p.add_inst(q_inst[i], q_inst[i + 1], q_inst[i + 2], q_inst[i + 3]);


  return p;
}
Program frac(int t_frac, int r_frac_addr){
  //this function assumes you precharge beforehand
  Program p;
  p.add_inst(all_nops());
  int R_FRAC_REG = RF_REG;
  int bank_reg = BAR;
  p.add_inst(SMC_LI(r_frac_addr, R_FRAC_REG));

    /*       --Bank -> bank_reg--
    *        |  X  -------------- |-> rfrac_reg (R_FRAC_REG)
    *        | ... -------------- |
    *        | ... -------------- |
    *
    * Cmd:        ----| ACT R_FRAC_REG |------|       PRE      |------FINISH
    * Time:     T0----|       T1       |------|       T2       |------FINISH
    * Interval: -----------------------|t_frac|----------------|------FINISH
    */

  int num_cmd = 2 + t_frac;
  num_cmd += 4 - (num_cmd % 4);
  Mininst q_inst[num_cmd];
  for (int i = 0; i < num_cmd; i++)
    q_inst[i] = SMC_NOP();
  
  q_inst[0]          = SMC_ACT(bank_reg, 0, R_FRAC_REG, 0);
  q_inst[t_frac + 1] = SMC_PRE(bank_reg, 0, 0);

  for (int i = 0; i < num_cmd; i += 4)
    p.add_inst(q_inst[i], q_inst[i + 1], q_inst[i + 2], q_inst[i + 3]);

  return p;
  
}


Program wrRow_after_double_act(int bank_reg, uint32_t wr_pattern)
{
  Program p;

  p.add_inst(SMC_LI(wr_pattern, PATTERN_REG)); // s
  for (int i = 0; i < 16; i++)
    p.add_inst(SMC_LDWD(PATTERN_REG, i));
  p.add_inst(SMC_LI(0, CAR));
  p.add_inst(SMC_LI(0, LOOP_COLS));
  //p.add_below(PRE(bank_reg, 0, 0));
  //p.add_below(ACT(bank_reg, 0, RAR, 0)); // Load loop variable
  p.add_label("WR_ROW_QUACC");
  // Write to a whole row, increment CAR per WR
  p.add_below(WRITE(bank_reg, CAR, 1));
  p.add_inst(SMC_ADDI(LOOP_COLS, 1, LOOP_COLS));
  p.add_branch(p.BR_TYPE::BL, LOOP_COLS, NUM_COLS_REG, "WR_ROW_QUACC");

  return p;
}