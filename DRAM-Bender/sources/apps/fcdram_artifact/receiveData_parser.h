#ifndef RECEIVEDATAPARSER_HH
#define RECEIVEDATAPARSER_HH

#include "prog.h"
#include "platform.h"
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

void parse_file(std::string file_name, std::vector<uint32_t> &vec);
int pattern_index(uint8_t stored_patt, uint8_t wr_patt, uint8_t init_patt, uint8_t dead_patt);
void fault_positions_twoACT(SoftMCPlatform &platform, 
                    bool plotting,
                    uint32_t bank_id,
                    uint32_t r_first, uint32_t r_second,
                    uint32_t wr_pattern, uint32_t f_init_pattern, uint32_t s_init_pattern,
                    uint32_t f_offset_pattern, uint32_t s_offset_pattern,
                    uint32_t f_start, uint32_t f_end, uint32_t s_start, uint32_t s_end,
void fault_positions_twoACT_one_pair(SoftMCPlatform &platform, 
                    bool plotting,
                    uint32_t bank_id,
                    uint32_t r_first, uint32_t r_second,
                    uint32_t wr_pattern, uint32_t f_init_pattern, uint32_t s_init_pattern,
                    uint32_t f_offset_pattern, uint32_t s_offset_pattern,
                    uint32_t f_start, uint32_t f_end, uint32_t s_start, uint32_t s_end,
                    std::ofstream &out_file);
#endif