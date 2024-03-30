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
              bool *plotting,
              uint32_t *r_first, uint32_t *r_second, 
              uint32_t *f_start, uint32_t *f_end, uint32_t *s_start, uint32_t *s_end,
              uint32_t *t_12, uint32_t *t_23,
              uint32_t *bank_id,
              std::ofstream &out_file)
{

  if(argc != 12)
  {
    printf("Usage: \n ./quac-test <plotting> <r_first> <r_second> <f_start> <f_end> <s_start> <s_end> <t_12> <t_23> <bank_id> <out_file>\n");
    return(0); 
  }
  int arg_i = 1;
  *plotting   = atoi(argv[arg_i++]);
  *r_first    = atoi(argv[arg_i++]);
  *r_second   = atoi(argv[arg_i++]);
  *f_start    = atoi(argv[arg_i++]);
  *f_end      = atoi(argv[arg_i++]);
  *s_start    = atoi(argv[arg_i++]);
  *s_end      = atoi(argv[arg_i++]);
  *t_12       = atoi(argv[arg_i++]);
  *t_23       = atoi(argv[arg_i++]);
  *bank_id    = atoi(argv[arg_i++]);
  out_file.open(std::string(argv[arg_i++]), std::ios::app);

  return 1;
}


int main(int argc, char*argv[])
{
  SoftMCPlatform platform;
  int err;
  bool plotting;
  uint32_t r_first;
  uint32_t r_second;
  uint32_t r_third;
  uint32_t f_start;
  uint32_t f_end;
  uint32_t s_start;
  uint32_t s_end;
  uint32_t t_start;
  uint32_t t_12;
  uint32_t t_23;
  uint32_t bank_id;

  std::ofstream   out_file;
  if(!read_args_n_parse(argc, argv,
                &plotting,
                &r_first, &r_second,
                &f_start, &f_end, &s_start, &s_end,
                &t_12, &t_23,
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
  uint32_t f_init_pattern = gen();
  uint32_t s_init_pattern = gen();
  uint32_t f_offset_pattern = gen();
  uint32_t s_offset_pattern = gen();
  // print all patterns

  

 Program program = test_prog_two_subarrays(r_first, r_second,  t_12,  t_23, 
                                wr_pattern, f_init_pattern, s_init_pattern,
                                f_offset_pattern, s_offset_pattern,
                                f_start,  f_end,
                                s_start,  s_end, 
                                bank_id);

  platform.execute(program); 

  fault_positions_twoACT( platform, 
                          plotting, 
                          bank_id,
                          r_first,  r_second,
                          wr_pattern,  f_init_pattern,s_init_pattern,
                          f_offset_pattern, s_offset_pattern,
                          f_start, f_end,  
                          s_start, s_end, 
                          out_file
                        );
  
  out_file.close();
  return 0;
}
