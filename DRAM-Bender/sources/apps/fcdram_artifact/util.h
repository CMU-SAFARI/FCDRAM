#ifndef UTIL_HH
#define UTIL_HH

#include "prog.h"
#include <cstring>
#include <cassert>

#define ZERO 0x00000000
#define ONE 0xFFFFFFFF

// Stride register ids are fixed and should not be changed
// CASR should always be reg 0
// BASR should always be reg 1
// RASR should always be reg 2

#define CASR 0
#define BASR 1
#define RASR 2

#define LOOP_BANKS 3
#define CAR 4
#define LOOP_ROWS 5
#define RAR 6
#define NUM_COLS_REG 14
#define NUM_ROWS_REG 8
#define LOOP_SEGMENT 9
#define RF_REG 10
#define NUM_BANKS_REG 11 
#define PATTERN_REG 12 
#define LOOP_COLS 13 

#define BAR 7



Inst all_nops();
Program PRE(int bank_reg, int ibar, int pall);
Program ACT(int bank_reg, int ibar, int row_reg, int irar);
Program WRITE(int bank_reg, int col_reg, int icar);
Program READ(int bank_reg, int col_reg, int icar);
Program wrRow_base_offset_label(int bank_reg, uint32_t row_base, uint32_t row_offset, uint32_t wr_pattern,int label);
Program wrRow_immediate(int bank_reg, uint32_t row_immd, uint32_t wr_pattern);
Program wrRow_immediate_label(int bank_reg, uint32_t row_immd, uint32_t wr_pattern, int label);
Program wrRow_base_offset(int bank_reg, uint32_t row_base, uint32_t row_offset, uint32_t wr_pattern);
Program rdRow_immediate(int bank_reg, uint32_t row_immd);
Program rdRow_immediate_label(int bank_reg, uint32_t row_immd, int label);
Program rdRow_base_offset(int bank_reg, uint32_t row_base, uint32_t row_offset);
Program rdCacheLine_immediate(int bank_reg, uint32_t row_immd, uint32_t cell_idx);
Program doubleACT(int t_12, int t_23, int r_first, int r_second);
Program doubleACT_immd_reg(int t_12, int t_23, int r_first, int r_second_reg);
Program frac(int t_frac, int r_frac_addr);
Program wrRow_after_double_act(int bank_reg, uint32_t wr_pattern);
#endif