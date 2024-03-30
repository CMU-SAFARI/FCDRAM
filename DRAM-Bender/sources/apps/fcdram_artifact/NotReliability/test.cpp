#include "instruction.h"
//#include "prog.h"
//include "platform.h"
#include <boost/filesystem.hpp>
#include <unistd.h>
#include <list>
#include <cstdlib>
#include <bitset>
#include <math.h> 
#include <random>
#include "../util.h"
#include "../receiveData_parser.h"
#include "../test_programs.h"
using namespace std;



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
#define BAR 7
#define NUM_ROWS_REG 8
#define ITER_REG 9
#define RF_REG 10
#define NUM_BANKS_REG 11
#define PATTERN_REG 12 
#define LOOP_COLS 13 //R3_REG
#define NUM_COLS_REG 14
#define LOOP_ITER 15

#define ZERO 0x00000000
#define ONE 0xFFFFFFFF



int read_args_n_parse(int argc, char* argv[],
              uint32_t *r_first, uint32_t *r_second, 
              std::vector<uint32_t> &upper_rows_idx, 
              std::vector<uint32_t> &lower_rows_idx, uint32_t *num_iter,
              uint32_t *t_12, uint32_t *t_23,
              uint32_t *which_half,
              uint32_t *bank_id,
              std::ofstream &out_file)
{

  if(argc != 11)
  {
    printf("Usage: \n ./quac-test <r_first> <r_second> <upper_rows_idx> <lower_rows_idx> <num_iter> <t_12> <t_23> <bank_id> <out_file>\n");
    return(0); 
  }
  int arg_i = 1;
  *r_first        = atoi(argv[arg_i++]);
  *r_second       = atoi(argv[arg_i++]);
  parse_file(std::string(argv[arg_i++]), upper_rows_idx);
  parse_file(std::string(argv[arg_i++]), lower_rows_idx);
  *num_iter       = atoi(argv[arg_i++]);
  *t_12           = atoi(argv[arg_i++]);
  *t_23           = atoi(argv[arg_i++]);
  *which_half     = atoi(argv[arg_i++]);
  *bank_id        = atoi(argv[arg_i++]);
  out_file.open(std::string(argv[arg_i++]), std::ios::app);

  return 1;
}

Program test_prog(uint32_t r_first, uint32_t r_second, uint32_t t_12, uint32_t t_23, 
                              uint32_t wr_pattern, uint32_t init_pattern, uint32_t num_iter,
                              std::vector<uint32_t> &upper_rows_idx, std::vector<uint32_t> &lower_rows_idx,
                              uint32_t bank_id)
{

  Program program;
  program.add_below(_init(bank_id));
  program.add_below(PRE(BAR, 0, 1));
  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_inst(all_nops());

  for (auto row : upper_rows_idx)
  {
    program.add_below(wrRow_immediate_label(BAR, row, init_pattern, row));
  }
  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_below(PRE(BAR, 0, 0));
  for (auto row : lower_rows_idx)
  {
    program.add_below(wrRow_immediate_label(BAR, row, init_pattern, row));
  }
  program.add_inst(SMC_SLEEP(6));
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(SMC_SLEEP(6));
  program.add_below(wrRow_immediate_label(BAR, r_first, wr_pattern, 65537));
  program.add_inst(SMC_SLEEP(6));
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(SMC_SLEEP(16));

  program.add_below(doubleACT(t_12, t_23, r_first, r_second));

  program.add_inst(SMC_SLEEP(6));
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(SMC_SLEEP(6));
  program.add_below(rdRow_immediate_label(BAR, upper_rows_idx[0], upper_rows_idx[0]));
  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(all_nops());
  program.add_inst(all_nops());

  program.add_below(rdRow_immediate_label(BAR, lower_rows_idx[0], lower_rows_idx[0]));
  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(all_nops());
  program.add_inst(all_nops());
  
  program.add_inst(SMC_END());
  return program;

}
int pattern_index(uint8_t stored_patt, uint8_t wr_patt, uint8_t init_patt)
{
  if(stored_patt == wr_patt) 
    return 0;
  else if(stored_patt == init_patt)
    return 1;
  else if (stored_patt == (uint8_t)(wr_patt ^ 0xFF))
    return 2;
  else if (stored_patt == (uint8_t)(init_patt ^ 0xFF))
    return 3;
  else
    return 4;
}

void report_results(SoftMCPlatform &platform, 
                    uint32_t wr_pattern, uint32_t init_pattern,
                    uint32_t *upper_bitline_arr,
                    uint32_t *lower_bitline_arr)
{
  uint8_t row[8192];
  for(int s_id = 0 ; s_id < 2 ; s_id++)
  {
    uint32_t expected_result = (s_id == 0) ? wr_pattern : ~(wr_pattern);
    platform.receiveData((void*)row, 8192);
    for(int j = 0 ; j < 128 ; j++){
        for (int k = 0 ; k < 64 ; k++){ 
          uint8_t exp_patt = expected_result >> ((uint32_t)(k%4)*8);
          uint8_t stored_patt = (uint8_t)row[j*64 + k];
          bitset<8> result(~(exp_patt ^ stored_patt));
          for (int i = 0; i < 8; i++)
          {
            if(s_id == 0)
              upper_bitline_arr[j*64*8 + k*8 + i] += result.test(i) ? 1 : 0;
            else
              lower_bitline_arr[j*64*8 + k*8 + i] += result.test(i) ? 1 : 0;
          }
        }
    }
  }
}
int main(int argc, char*argv[])
{
  SoftMCPlatform platform;
  int err;
  bool plotting;
  uint32_t r_first;
  uint32_t r_second;
  std::vector<uint32_t> upper_rows_idx;
  std::vector<uint32_t> lower_rows_idx;
  uint32_t num_iter;
  uint32_t t_12;
  uint32_t t_23;
  uint32_t which_half;
  uint32_t bank_id;

  std::ofstream   out_file;
  if(!read_args_n_parse(argc, argv,
                &r_first, &r_second,
                upper_rows_idx, 
                lower_rows_idx, &num_iter,
                &t_12, &t_23,
                &which_half,
                &bank_id,
                out_file))
  {
    exit(0);
  }

  if ((err = platform.init()) != SOFTMC_SUCCESS)
  {
    cerr << "Could not initialize SoftMC Platform: " << err << endl;
  }
  platform.reset_fpga();

  
  
  
  random_device rd;   // non-deterministic generator
  mt19937 gen(rd());  

  uint32_t wr_pattern = gen();
  uint32_t init_pattern = gen();
  // print all patterns

  


  uint32_t upper_bitline_results[8192*8] {0};
  uint32_t lower_bitline_results[8192*8] {0};
  for(int ii = 0; ii < num_iter; ii++)
  {
    Program program = test_prog(r_first, r_second,  t_12,  t_23, 
                                wr_pattern, init_pattern, num_iter,
                                upper_rows_idx, lower_rows_idx,
                                bank_id);
    platform.execute(program); 
    report_results(platform, wr_pattern, init_pattern, upper_bitline_results, lower_bitline_results);
  }
  double avg_upper_coverage = 0.0;
  double avg_lower_coverage = 0.0;
  double max_stability = 0.0;
  double min_stability = 0.0;
  uint32_t upper_full_stable_bitlines = 0;
  uint32_t lower_full_stable_bitlines = 0;
  for(int j = 0; j < 65536; j++)
  {
    if(upper_bitline_results[j] == num_iter)
    {
      max_stability +=1;
      upper_full_stable_bitlines++;
    }
    avg_upper_coverage += (double)upper_bitline_results[j]/(double)num_iter;
    if(which_half == 0)
    {
      if(j%64 < 32)
      {
        if(lower_bitline_results[j] == num_iter)
        {
          lower_full_stable_bitlines++;
        }
        avg_lower_coverage += (double)lower_bitline_results[j]/(double)num_iter;
      }
    }
    else
    {
      if(j%64 >= 32)
      {
        if(lower_bitline_results[j] == num_iter)
        {
          lower_full_stable_bitlines++;
        }
        avg_lower_coverage += (double)lower_bitline_results[j]/(double)num_iter;
      }
    }
  }
  out_file << std::to_string((avg_upper_coverage/65536.0)*100.0) << ",";
  out_file << std::to_string(((double)upper_full_stable_bitlines/65536.0)*100.0) << ",";
  out_file << std::to_string((avg_lower_coverage/32768.0)*100.0) << ",";
  out_file << std::to_string(((double)lower_full_stable_bitlines/32768.0)*100.0);
  out_file.close();
  
  return 0;
}
