#include "instruction.h"

#include <boost/filesystem.hpp>
#include <unistd.h>
#include <list>
#include <cstdlib>
#include <bitset>
#include <math.h> 
#include <algorithm>
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
              uint32_t *t_12, uint32_t *t_23,
              uint32_t *input_location,
              std::vector<uint32_t> &input_pattern,  std::vector<uint32_t> &ref_pattern,
              std::vector<uint32_t> &input_rows_idx, std::vector<uint32_t> &ref_rows_idx,
              uint32_t *n_frac_times, uint32_t *t_frac, uint32_t *n_frac_rows,
              uint32_t *which_half, uint32_t *num_iter,
              uint32_t *is_rand,
              uint32_t *bank_id,
              std::ofstream &out_file)
{

  if(argc != 18)
  {
    printf("Usage: \n ./quac-test <r_first> <r_second> <t_12> <t_23> <input_location> <input_pattern> <ref_pattern> <input_rows_idx> <ref_rows_idx> <n_frac> <t_frac> <which_half> <num_iter> <bank_id> <out_file>\n");
    return(0); 
  }
  int arg_i = 1;
  *r_first    = atoi(argv[arg_i++]);
  *r_second   = atoi(argv[arg_i++]);
  *t_12       = atoi(argv[arg_i++]);
  *t_23       = atoi(argv[arg_i++]);
  *input_location = atoi(argv[arg_i++]);
  parse_file(std::string(argv[arg_i++]), input_pattern);
  parse_file(std::string(argv[arg_i++]), ref_pattern);
  parse_file(std::string(argv[arg_i++]), input_rows_idx);
  parse_file(std::string(argv[arg_i++]), ref_rows_idx);
  *n_frac_times    = atoi(argv[arg_i++]);
  *t_frac    = atoi(argv[arg_i++]);
  *n_frac_rows    = atoi(argv[arg_i++]);
  *which_half    = atoi(argv[arg_i++]);
  *num_iter    = atoi(argv[arg_i++]);
  *is_rand    = atoi(argv[arg_i++]);
  *bank_id    = atoi(argv[arg_i++]);
  out_file.open(std::string(argv[arg_i++]), std::ios::app);

  return 1;
}
Program test_prog(uint32_t r_first, uint32_t r_second, 
                  uint32_t t_12, uint32_t t_23, 
                  uint32_t bank_id,
                  std::vector<uint32_t> &upper_pattern, std::vector<uint32_t> &lower_pattern,
                  std::vector<uint32_t> &upper_rows_idx, std::vector<uint32_t> &lower_rows_idx,
                  uint32_t input_row_idx, uint32_t ref_row_idx,
                  std::vector<uint32_t> &r_frac_idx, 
                  uint32_t n_frac, uint32_t t_frac)
{

  Program program;                                
  srand((unsigned) time(NULL));
  int random = rand();
  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_below(PRE(BAR, 0, 0));
  for(int i = 0; i < upper_pattern.size(); i++)
  {
    random = rand();
    program.add_below(wrRow_immediate_label(BAR, upper_rows_idx[i], upper_pattern[i], random));
  }
  program.add_inst(SMC_SLEEP(6));
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(SMC_SLEEP(6));

  for(int i = 0; i < lower_pattern.size(); i++)
  {
    random = rand();
    program.add_below(wrRow_immediate_label(BAR, lower_rows_idx[i], lower_pattern[i], random));
  }
  program.add_inst(SMC_SLEEP(6));
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(SMC_SLEEP(6));

  for (int i = 0; i < r_frac_idx.size(); i++)
  {
      for(int j = 0; j < n_frac; j++)
      {
        program.add_inst(SMC_SLEEP(6));
        program.add_below(frac(t_frac,r_frac_idx[i]));
        program.add_inst(SMC_SLEEP(6));
      }
  }

  program.add_inst(SMC_SLEEP(6));
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(SMC_SLEEP(6));
  program.add_below(doubleACT(t_12, t_23, r_first, r_second));
  program.add_inst(SMC_SLEEP(6));
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(SMC_SLEEP(6));

  program.add_below(rdRow_immediate_label(BAR, input_row_idx, input_row_idx));
  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(all_nops());
  program.add_inst(all_nops());

  program.add_below(rdRow_immediate_label(BAR, ref_row_idx, ref_row_idx));
  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(all_nops());
  program.add_inst(all_nops());

program.add_inst(SMC_END());
return program;
}
std::vector<uint8_t> logic_operation(std::vector<uint32_t> pattern, uint32_t operation)
{

  int n_patterns = pattern.size();
  std::vector <uint8_t> bit_logic;
  for(int j = 0; j < 32 ; j++)
  { 
    uint8_t temp = 0;
    for(int i = 0; i < n_patterns; i++)
    {
      temp += (pattern[i] >> j) & 0x1; 
    }

    if (operation == 0)
    {
      //means it is OR operation
      bit_logic.push_back(temp > 0 ? 1 : 0);
    }
    else
    {
      //means it is AND operation
      bit_logic.push_back(temp == n_patterns ? 1 : 0);
    }
  }

  return bit_logic;
}


void report_coverage_results(SoftMCPlatform &platform, std::vector<uint32_t> ref_pattern, 
                              std::vector<uint32_t> input_pattern, uint32_t *ref_arr, uint32_t *op_arr, uint32_t which_half, std::ofstream &out_file, bool ref_row)
{
  std::vector <uint8_t> expected_logic = logic_operation(input_pattern, (ref_pattern[0] & 0x1));

  uint8_t row[8192];
  platform.receiveData((void*)row, 8192);

  uint32_t exp_logic_result = 0;
  for(int i = 0; i < expected_logic.size(); i++)
  {
    exp_logic_result |= ((expected_logic[i] & 0x1) << i);
  }
  if (ref_row)
    exp_logic_result = ~exp_logic_result;

  uint32_t count_f = 0;
  uint32_t count_s = 0;
  for(int j = 0; j < 8192/4; j++)
  {
    uint32_t read_logic_result= row[j*4] | (row[j*4+1] << 8) | (row[j*4+2] << 16) | (row[j*4+3] << 24);
    bitset<32> result(~(read_logic_result ^ exp_logic_result));
    for (int i = 0; i < 32; i++)
    {
      if(ref_row)
      {
        ref_arr[j*32 + i] += result.test(i) ? 1 : 0;
      }
      else
      {
        op_arr[j*32 + i]  += result.test(i) ? 1 : 0;
      }
    }
  }
}

void write_stats(std::ofstream &out_file, uint32_t *ref_bitline_results, uint32_t *op_bitline_results, uint32_t which_half, uint32_t num_iter)
{
  double avg_ref_coverage = 0.0;
  double avg_op_coverage = 0.0;
  uint32_t ref_full_stable_bitlines = 0;
  uint32_t op_full_stable_bitlines = 0;
  for(int j = 0; j < 65536; j++)
  {
    if(which_half == 0)
    //every first 32bit out of 64 bit
    {
      if(j%64 < 32)
      {
        if(ref_bitline_results[j] == num_iter)
        {
          ref_full_stable_bitlines++;
        }

        if(op_bitline_results[j] == num_iter)
        {
          op_full_stable_bitlines++;
        }
        avg_ref_coverage += (double)ref_bitline_results[j]/(double)num_iter;
        avg_op_coverage += (double)op_bitline_results[j]/(double)num_iter;
      }
    }
    else
    {
      if(j%64 >= 32)
      {
        if(ref_bitline_results[j] == num_iter)
        {
          ref_full_stable_bitlines++;
        }

        if(op_bitline_results[j] == num_iter)
        {
          op_full_stable_bitlines++;
        }
        avg_ref_coverage += (double)ref_bitline_results[j]/(double)num_iter;
        avg_op_coverage += (double)op_bitline_results[j]/(double)num_iter;
      }
    }
  }
  out_file << std::to_string((avg_ref_coverage/32768.0)*100.0) << ",";
  out_file << std::to_string(((double)ref_full_stable_bitlines/32768.0)*100.0) << ",";
  out_file << std::to_string((avg_op_coverage/32768.0)*100.0) << ",";
  out_file << std::to_string(((double)op_full_stable_bitlines/32768.0)*100.0);
}
void upper_lower_patterns(std::vector<uint32_t> &upper_pattern, std::vector<uint32_t> &lower_pattern, 
                          std::vector<uint32_t> &input_pattern, std::vector<uint32_t> &ref_pattern, 
                          std::vector<uint32_t> &upper_rows_idx, std::vector<uint32_t> &lower_rows_idx,
                          std::vector<uint32_t> &input_rows_idx, std::vector<uint32_t> &ref_rows_idx,
                          uint32_t input_location, uint32_t is_rand)
{
  random_device rd;   // non-deterministic generator
  mt19937 gen(rd());  
  auto rng = std::default_random_engine {};
  if(is_rand == 1)
  {
    std::vector<int> n_ones_per_bit(32, 0);
    std::vector<std::vector<int>> pattern_bitmap(32, std::vector<int>(input_pattern.size(), 0));
    for(int j = 0; j < 32; j++)
    {
      int n_ones = gen() % (input_pattern.size() + 1);
      for(int i = 0; i < input_pattern.size(); i++)
      {
        pattern_bitmap[j][i] = (i < n_ones) ? 1 : 0;
      }
      std::shuffle(pattern_bitmap[j].begin(), pattern_bitmap[j].end(), rng);
    }

    for(int i = 0; i < input_pattern.size(); i++)
    {
      uint32_t pattern = 0;
      for(int j = 0; j < 32; j++)
      {
        pattern |= (pattern_bitmap[j][i] << j);
      }
      input_pattern[i] = pattern;
    }
  }


  else
  {
    std::shuffle(input_pattern.begin(), input_pattern.end(), rng);
  }
  if(input_location == 0)
  {
    //means input is in upper subarray
    upper_pattern = input_pattern;
    lower_pattern = ref_pattern;
    upper_rows_idx = input_rows_idx;
    lower_rows_idx = ref_rows_idx;
  }
  else 
  {
    //means input is in lower subarray
    upper_pattern = ref_pattern;
    lower_pattern = input_pattern;
    upper_rows_idx = ref_rows_idx;
    lower_rows_idx = input_rows_idx;
  }
}
int main(int argc, char*argv[])
{

  SoftMCPlatform platform;
  int err;
  uint32_t r_first;
  uint32_t r_second;
  uint32_t t_12;
  uint32_t t_23;
  uint32_t input_location;
  uint32_t n_frac_times;
  uint32_t t_frac;
  uint32_t n_frac_rows;
  std::vector<uint32_t> input_pattern;
  std::vector<uint32_t> ref_pattern;
  std::vector<uint32_t> input_rows_idx;
  std::vector<uint32_t> ref_rows_idx;

  uint32_t which_half;
  uint32_t num_iter;
  uint32_t is_rand;
  uint32_t bank_id;
  std::ofstream   out_file;
  if(!read_args_n_parse(argc, argv,
                &r_first, &r_second,
                &t_12, &t_23,
                &input_location,
                input_pattern, ref_pattern,
                input_rows_idx, ref_rows_idx,
                &n_frac_times, &t_frac, &n_frac_rows,
                &which_half, &num_iter,
                &is_rand,
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
auto rng = std::default_random_engine {};
std::vector<uint32_t> upper_pattern;
std::vector<uint32_t> lower_pattern;
std::vector<uint32_t> upper_rows_idx;
std::vector<uint32_t> lower_rows_idx;

uint32_t input_row_idx = input_rows_idx[0];
uint32_t ref_row_idx = ref_rows_idx[0];

std::vector<uint32_t> r_frac_idx;
for(int i = 0; i < n_frac_rows; i++)
{
  r_frac_idx.push_back(ref_rows_idx[i]);
}

uint32_t ref_bitline_results[8192*8] {0};
uint32_t op_bitline_results[8192*8] {0};
if(is_rand == 0)
  num_iter = 100;


for(int ii = 0; ii < num_iter; ii++)
{
  upper_lower_patterns( upper_pattern, lower_pattern, 
                        input_pattern, ref_pattern, 
                        upper_rows_idx,lower_rows_idx,
                        input_rows_idx,ref_rows_idx,
                        input_location, is_rand);

  Program program = test_prog(r_first, r_second,  t_12,  t_23, 
                            bank_id, upper_pattern, lower_pattern,
                            upper_rows_idx, lower_rows_idx,
                            input_row_idx, ref_row_idx,
                            r_frac_idx, n_frac_times, t_frac); 

  platform.execute(program); 

  for (int i = 0; i < 2; i++)
  {
    // i ==0 -> operand row (AND/OR Result)
    // i ==1 -> reference row (NAND/NOR Result)
    report_coverage_results(platform, ref_pattern, 
                          input_pattern, ref_bitline_results, op_bitline_results, which_half, out_file, i);
  }
}
  write_stats(out_file, ref_bitline_results, op_bitline_results, which_half, num_iter);
  out_file.close();

  return 0;
}
