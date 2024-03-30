#include "instruction.h"
#include "prog.h"
#include "platform.h"
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <list>
#include <cstdlib>
#include "../util.h"

using namespace std;



#define NUM_BANKS 16
#define NUM_ROWS 1024 //32768
#define NUM_COLS 1024


int read_args_n_parse(int argc, char* argv[],uint32_t *t_12, uint32_t *t_23,
                      uint32_t *r_first, uint32_t *r_second, 
                      uint32_t *search_start_addr, uint32_t *search_last_addr, 
                      std::ofstream &out_file)
{

  if(argc != 8)
  {
    printf("Usage: \n ./row-clone <t_12> <t_23> <r_first> <r_second> <search_start_addr> <search_last_addr> <out_file>\n");
    return(0);
  }
  int arg_i = 1;
  *t_12        = atoi(argv[arg_i++]);
  *t_23        = atoi(argv[arg_i++]);
  *r_first     = atoi(argv[arg_i++]);
  *r_second    = atoi(argv[arg_i++]);
  *search_start_addr    = atoi(argv[arg_i++]);
  *search_last_addr    = atoi(argv[arg_i++]);
  out_file.open(std::string(argv[arg_i++]), std::ios::app);

  return 1;
}

Program _init()
{
  Program program;
  program.add_inst(SMC_LI(NUM_ROWS, NUM_ROWS_REG));   // load NUM_ROWS into NUM_ROWS_REG
  program.add_inst(SMC_LI(NUM_BANKS, NUM_BANKS_REG)); // load NUM_BANKS into NUM_BANKS_REG
  program.add_inst(SMC_LI(8, CASR));                  // Load 8 into CASR since each READ reads 8 columns
  program.add_inst(SMC_LI(1, BASR));                  // Load 1 into BASR
  program.add_inst(SMC_LI(1, RASR));                  // Load 1 into RASR
  program.add_inst(SMC_LI(128, NUM_COLS_REG));        // Load COL_SIZE register
  program.add_inst(SMC_LI(1, BAR));
  return program;
}

Program test_prog(uint32_t r_first, uint32_t r_second, uint32_t t_12, uint32_t t_23, uint32_t search_start_addr, uint32_t search_last_addr, uint32_t wr_pattern)
{


  Program program;
  program.add_below(_init());
  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_inst(SMC_LI(search_start_addr, LOOP_ROWS));
  program.add_inst(SMC_LI(search_last_addr, NUM_ROWS_REG));
  program.add_label("WRITE_WHOLE_BANK");
    program.add_below(PRE(BAR, 0, 0));
    program.add_below(wrRow_base_offset_label(BAR, LOOP_ROWS, 0, ZERO, r_first));
    program.add_inst(SMC_ADDI(LOOP_ROWS, 1, LOOP_ROWS));
  program.add_branch(program.BR_TYPE::BL, LOOP_ROWS, NUM_ROWS_REG, "WRITE_WHOLE_BANK");

  //Write a random pattern into r_first
  program.add_inst(SMC_SLEEP(6));
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(SMC_SLEEP(6));
  program.add_below(wrRow_immediate_label(BAR, r_first, wr_pattern, wr_pattern));
  program.add_inst(SMC_SLEEP(6));
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(SMC_SLEEP(6));


  //perform doubleACT (RowClone)
  program.add_below(doubleACT(t_12, t_23, r_first, r_second));
  program.add_inst(SMC_SLEEP(6));
  program.add_below(PRE(BAR, 0, 0));
  program.add_inst(SMC_SLEEP(6));

  //Read r_second
  program.add_inst(SMC_LI(0, LOOP_ROWS));
  program.add_below(rdRow_immediate(BAR, r_second));
  program.add_inst(all_nops());
  program.add_inst(all_nops());
  program.add_below(PRE(BAR, 0, 0));

  program.add_inst(SMC_END());
  return program;

}
void receive_results(SoftMCPlatform &platform, uint32_t r_first, uint32_t r_second, uint32_t wr_pattern, std::ofstream &out_file)
{
  uint32_t test_pattern = wr_pattern; 

  int rc = 0;
  long int err_count = 0;
  long int a_err_count = 0;

  uint8_t row[8192];

  platform.receiveData((void*)row, 8192);
  err_count = 0;
  a_err_count = 0;
  for(int j = 0 ; j < 128 ; j++)
  {
    for (int k = 0 ; k < 64 ; k++)
    { // each byte in cache line
      uint8_t mini_patt = wr_pattern >> ((uint32_t)(k%4)*8);
      if(mini_patt == (uint8_t)row[j*64 + k])
      {
        err_count++;
      }
    }
  }
  if(err_count == 128*64)
  {
    out_file << r_first << ",";
    out_file << r_second;
  }
  out_file.close();
}


int main(int argc, char*argv[])
{

  SoftMCPlatform platform;
  int err;
  // buffer allocated for reading data from the board
  uint32_t t_12, t_23, r_first,r_second;
  uint32_t search_start_addr, search_last_addr;
  std::ofstream out_file;
  if(!read_args_n_parse(argc, argv, &t_12, &t_23,
                        &r_first, &r_second, 
                        &search_start_addr, &search_last_addr,
                        out_file))
  {
    exit(0);
  }
  // Initialize the platform, opens file descriptors for the board PCI-E interface.
  if ((err = platform.init()) != SOFTMC_SUCCESS)
  {
    cerr << "Could not initialize SoftMC Platform: " << err << endl;
  }
  platform.reset_fpga();

  uint32_t wr_pattern;
  srand(time(NULL)+getpid());
  uint16_t rand1 = (uint16_t) rand();
  wr_pattern = rand1 << 16;
  wr_pattern |= ((uint16_t)rand());

  Program program = test_prog(r_first, r_second, t_12, t_23, search_start_addr, search_last_addr, wr_pattern);
  platform.execute(program);
  receive_results(platform, r_first, r_second, wr_pattern,  out_file);


  return 0;
}
