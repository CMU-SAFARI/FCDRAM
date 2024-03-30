#include "test_programs.h"

Program _init(uint32_t bank_id)
{
  Program program;
  program.add_inst(SMC_LI(8, CASR));                  // Load 8 into CASR since each READ reads 8 columns
  program.add_inst(SMC_LI(1, BASR));                  // Load 1 into BASR
  program.add_inst(SMC_LI(1, RASR));                  // Load 1 into RASR
  program.add_inst(SMC_LI(128, NUM_COLS_REG));        // Load COL_SIZE register
  program.add_inst(SMC_LI(bank_id, BAR));
  return program;

}

Program test_prog_two_subarrays(uint32_t r_first, uint32_t r_second, uint32_t t_12, uint32_t t_23, 
                              uint32_t wr_pattern, uint32_t f_init_pattern, uint32_t s_init_pattern,
                              uint32_t f_offset_pattern, uint32_t s_offset_pattern,
                               uint32_t f_start, uint32_t f_end,
                              uint32_t s_start, uint32_t s_end, 
                              uint32_t bank_id)
{

  Program program;
  int offset = 1000;
  program.add_below(_init(bank_id));
  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_inst(SMC_LI(s_start, LOOP_SEGMENT));
  program.add_label("SEGMENT_LOOP");
  // program.add_inst(SMC_LI(f_start-offset, LOOP_ROWS));
  // program.add_inst(SMC_LI(f_start+1, NUM_ROWS_REG));
  //  program.add_label("WRITE_WHOLE_BANK_SUB_OFFSET_0");
  //   program.add_below(PRE(BAR, 0, 0));
  //   program.add_below(wrRow_base_offset_label(BAR, LOOP_ROWS, 0, f_offset_pattern, r_first-1));
  //   program.add_inst(SMC_ADDI(LOOP_ROWS, 1, LOOP_ROWS));
  // program.add_branch(program.BR_TYPE::BL, LOOP_ROWS, NUM_ROWS_REG, "WRITE_WHOLE_BANK_SUB_OFFSET_0");

  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_inst(SMC_LI(f_start, LOOP_ROWS));
  program.add_inst(SMC_LI(f_end+1, NUM_ROWS_REG));
   program.add_label("WRITE_WHOLE_BANK_SUB0");
    program.add_below(PRE(BAR, 0, 0));
    program.add_below(wrRow_base_offset_label(BAR, LOOP_ROWS, 0, f_init_pattern, f_init_pattern));
    program.add_inst(SMC_ADDI(LOOP_ROWS, 1, LOOP_ROWS));
  program.add_branch(program.BR_TYPE::BL, LOOP_ROWS, NUM_ROWS_REG, "WRITE_WHOLE_BANK_SUB0");

  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_inst(SMC_LI(s_start, LOOP_ROWS));
  program.add_inst(SMC_LI(s_end+1, NUM_ROWS_REG));
   program.add_label("WRITE_WHOLE_BANK_SUB1");
    program.add_below(PRE(BAR, 0, 0));
    program.add_below(wrRow_base_offset_label(BAR, LOOP_ROWS, 0, s_init_pattern, s_init_pattern));
    program.add_inst(SMC_ADDI(LOOP_ROWS, 1, LOOP_ROWS));
  program.add_branch(program.BR_TYPE::BL, LOOP_ROWS, NUM_ROWS_REG, "WRITE_WHOLE_BANK_SUB1");


  program.add_inst(all_nops());
  program.add_inst(all_nops());


  //Write a random pattern into r_first
  program.add_inst(SMC_SLEEP(6));
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(SMC_SLEEP(6));
  program.add_below(wrRow_immediate_label(BAR, r_first, wr_pattern, wr_pattern));
  program.add_inst(SMC_SLEEP(6));
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(SMC_SLEEP(16));


  //perform doubleACT (RowClone)
  program.add_below(doubleACT_immd_reg(t_12, t_23, r_first, LOOP_SEGMENT));
  program.add_inst(SMC_SLEEP(15));
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(SMC_SLEEP(6));

  //Read r_second
  program.add_inst(SMC_LI(f_start, LOOP_ROWS));
  program.add_inst(SMC_LI(s_end, NUM_ROWS_REG));
  program.add_label("READ_WHOLE_BANK");  
      program.add_below(rdRow_base_offset(BAR, LOOP_ROWS, 0));
      program.add_inst(all_nops());
      program.add_inst(all_nops());
      program.add_below(PRE(BAR, 0, 0));
      program.add_inst(SMC_ADDI(LOOP_ROWS, 1, LOOP_ROWS));
  program.add_branch(program.BR_TYPE::BL, LOOP_ROWS, NUM_ROWS_REG, "READ_WHOLE_BANK");
  
  program.add_inst(SMC_SLEEP(6));

  program.add_inst(SMC_ADDI(LOOP_SEGMENT, 1, LOOP_SEGMENT));
  program.add_inst(SMC_LI(s_end, NUM_ROWS_REG));   // load NUM_ROWS into NUM_ROWS_REG
  program.add_branch(program.BR_TYPE::BL, LOOP_SEGMENT, NUM_ROWS_REG, "SEGMENT_LOOP");

  program.add_inst(SMC_END());
  return program;

}


Program test_prog_two_subarrays_one_pair(uint32_t r_first, uint32_t r_second, uint32_t t_12, uint32_t t_23, 
                              uint32_t wr_pattern, uint32_t f_init_pattern, uint32_t s_init_pattern,
                              uint32_t f_offset_pattern, uint32_t s_offset_pattern,
                               uint32_t f_start, uint32_t f_end,
                              uint32_t s_start, uint32_t s_end, 
                              uint32_t bank_id)
{

  Program program;
  int offset = 1000;
  program.add_below(_init(bank_id));
  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_inst(SMC_LI(f_start-offset, LOOP_ROWS));
  program.add_inst(SMC_LI(f_start+1, NUM_ROWS_REG));
   program.add_label("WRITE_WHOLE_BANK_SUB_OFFSET_0");
    program.add_below(PRE(BAR, 0, 0));
    program.add_below(wrRow_base_offset_label(BAR, LOOP_ROWS, 0, f_offset_pattern, r_first-1));
    program.add_inst(SMC_ADDI(LOOP_ROWS, 1, LOOP_ROWS));
  program.add_branch(program.BR_TYPE::BL, LOOP_ROWS, NUM_ROWS_REG, "WRITE_WHOLE_BANK_SUB_OFFSET_0");

  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_inst(SMC_LI(f_start, LOOP_ROWS));
  program.add_inst(SMC_LI(f_end+1, NUM_ROWS_REG));
   program.add_label("WRITE_WHOLE_BANK_SUB0");
    program.add_below(PRE(BAR, 0, 0));
    program.add_below(wrRow_base_offset_label(BAR, LOOP_ROWS, 0, f_init_pattern, f_init_pattern));
    program.add_inst(SMC_ADDI(LOOP_ROWS, 1, LOOP_ROWS));
  program.add_branch(program.BR_TYPE::BL, LOOP_ROWS, NUM_ROWS_REG, "WRITE_WHOLE_BANK_SUB0");

  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_inst(SMC_LI(s_start, LOOP_ROWS));
  program.add_inst(SMC_LI(s_end+1, NUM_ROWS_REG));
   program.add_label("WRITE_WHOLE_BANK_SUB1");
    program.add_below(PRE(BAR, 0, 0));
    program.add_below(wrRow_base_offset_label(BAR, LOOP_ROWS, 0, s_init_pattern, s_init_pattern));
    program.add_inst(SMC_ADDI(LOOP_ROWS, 1, LOOP_ROWS));
  program.add_branch(program.BR_TYPE::BL, LOOP_ROWS, NUM_ROWS_REG, "WRITE_WHOLE_BANK_SUB1");


  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_inst(SMC_LI(s_end+1, LOOP_ROWS));
  program.add_inst(SMC_LI(s_end+offset, NUM_ROWS_REG));
   program.add_label("WRITE_WHOLE_BANK_SUB_OFFSET_1");
    program.add_below(PRE(BAR, 0, 0));
    program.add_below(wrRow_base_offset_label(BAR, LOOP_ROWS, 0, s_offset_pattern, r_second+1));
    program.add_inst(SMC_ADDI(LOOP_ROWS, 1, LOOP_ROWS));
  program.add_branch(program.BR_TYPE::BL, LOOP_ROWS, NUM_ROWS_REG, "WRITE_WHOLE_BANK_SUB_OFFSET_1");

  //Write a random pattern into r_first
  program.add_inst(SMC_SLEEP(6));
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(SMC_SLEEP(6));
  program.add_below(wrRow_immediate_label(BAR, r_first, wr_pattern, wr_pattern));
  program.add_inst(SMC_SLEEP(6));
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(SMC_SLEEP(16));


  //perform doubleACT (RowClone)
  program.add_below(doubleACT(t_12, t_23, r_first, r_second));
  program.add_inst(SMC_SLEEP(15));
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(SMC_SLEEP(6));

  //Read r_second
  program.add_inst(SMC_LI(f_start, LOOP_ROWS));
  program.add_inst(SMC_LI(s_end, NUM_ROWS_REG));
  program.add_label("READ_WHOLE_BANK");  
      program.add_below(rdRow_base_offset(BAR, LOOP_ROWS, 0));
      program.add_inst(all_nops());
      program.add_inst(all_nops());
      program.add_below(PRE(BAR, 0, 0));
      program.add_inst(SMC_ADDI(LOOP_ROWS, 1, LOOP_ROWS));
  program.add_branch(program.BR_TYPE::BL, LOOP_ROWS, NUM_ROWS_REG, "READ_WHOLE_BANK");
  
  program.add_inst(SMC_SLEEP(6));
  program.add_inst(SMC_END());
  return program;

}