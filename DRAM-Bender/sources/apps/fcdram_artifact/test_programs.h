#ifndef TESTPROGRAMS_HH
#define TESTPROGRAMS_HH

#include "util.h"
#include <cstring>

Program _init(uint32_t bank_id);
Program test_prog_two_subarrays(uint32_t r_first, uint32_t r_second, uint32_t t_12, uint32_t t_23, 
                              uint32_t wr_pattern, uint32_t f_init_pattern, uint32_t s_init_pattern,
                              uint32_t f_offset_pattern, uint32_t s_offset_pattern,
                               uint32_t f_start, uint32_t f_end,
                              uint32_t s_start, uint32_t s_end, 
                              uint32_t bank_id);

Program test_prog_two_subarrays_one_pair(uint32_t r_first, uint32_t r_second, uint32_t t_12, uint32_t t_23, 
                              uint32_t wr_pattern, uint32_t f_init_pattern, uint32_t s_init_pattern,
                              uint32_t f_offset_pattern, uint32_t s_offset_pattern,
                               uint32_t f_start, uint32_t f_end,
                              uint32_t s_start, uint32_t s_end, 
                              uint32_t bank_id);

#endif