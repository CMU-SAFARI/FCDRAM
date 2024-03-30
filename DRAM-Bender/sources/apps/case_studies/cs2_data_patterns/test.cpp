#include "instruction.h"
#include "prog.h"
#include "platform.h"
#include "tools.h"

#include <string>
#include <fstream>
#include <iostream>
#include <list>
#include <cassert>
#include <bitset>
#include <chrono>
#include <iomanip>
#include <math.h>

#include <array>
#include <algorithm>
#include <numeric>
#include <ctime>

// #define PRINT_SOFTMC_PROGS

using namespace std;

#define CASR 0
#define BASR 1
#define RASR 2

#define NUM_SOFTMC_REGS 16
#define FPGA_PERIOD 1.5015f // ns

#define RED_TXT "\033[31m"
#define GREEN_TXT "\033[32m"
#define YELLOW_TXT "\033[33m"
#define BLUE_TXT "\033[34m"
#define MAGENTA_TXT "\033[35m"
#define NORMAL_TXT "\033[0m"

int NUM_BANKS = 16; // this is the total number of banks in the chip
int NUM_BANK_GROUPS = 4;
int NUM_ROWS  = 32768;
int ROW_SIZE = 8192;
int NUM_COLS_PER_ROW = 128;
int CHIP_NUM = 4;
int CACHE_LINE_BITS = 512;

float DEFAULT_TRCD = 13.5f; // ns
float DEFAULT_TRAS = 35.0f; // ns
float DEFAULT_TRP = 13.5f; // ns
float DEFAULT_TWR = 15.0f; // ns
float DEFAULT_TRFC = 260.0f; // ns
float DEFAULT_TRRDS = 5.3f; // ns (ACT-ACT to different bank groups)
float DEFAULT_TRRDL = 6.4f; // ns (ACT-ACT to same bank group)
float DEFAULT_TREFI = 7800.0f;

int trcd_cycles = (int) ceil(DEFAULT_TRCD/FPGA_PERIOD);
int tras_cycles = (int) ceil(DEFAULT_TRAS/FPGA_PERIOD);
int trp_cycles = (int) ceil(DEFAULT_TRP/FPGA_PERIOD);
int twr_cycles = (int) ceil(DEFAULT_TWR/FPGA_PERIOD);
int trfc_cycles = (int) ceil(DEFAULT_TRFC/FPGA_PERIOD);
int trrds_cycles = (int) ceil(DEFAULT_TRRDS/FPGA_PERIOD);
int trrdl_cycles = (int) ceil(DEFAULT_TRRDL/FPGA_PERIOD);
int trefi_cycles = (int) ceil(DEFAULT_TREFI/FPGA_PERIOD);

vector<uint32_t> reserved_regs{CASR, BASR, RASR};
vector<uint> special_pattern;

typedef struct RowSet {
    vector<uint> victim_ids;
    vector<uint> aggr_ids;
    vector<uint> isol_ids;
    uint bank_id;
} RowSet;

void init_program(Program& prog){
    add_op_with_delay(prog, SMC_PRE(0, 0, 1), 0, trp_cycles); // precharge all banks
}

void end_program(Program& prog){   
    prog.add_inst(all_nops());
    prog.add_inst(SMC_END());
}

void init_row(Program* prog, SoftMCRegAllocator* reg_alloc, const uint target_bank, const uint target_row, 
              const bitset<512> pattern){

    SMC_REG reg_row_addr = reg_alloc->allocate_SMC_REG();
    SMC_REG reg_col_addr = reg_alloc->allocate_SMC_REG();
    SMC_REG reg_bank_addr = reg_alloc->allocate_SMC_REG();
    SMC_REG reg_num_cols = reg_alloc->allocate_SMC_REG();
    SMC_REG reg_wrdata = reg_alloc->allocate_SMC_REG();

    bitset<512> bitset_int_mask(0xFFFFFFFF);
    
    prog->add_inst(SMC_LI(NUM_COLS_PER_ROW*8, reg_num_cols));
    prog->add_inst(SMC_LI(target_bank, reg_bank_addr));
    prog->add_inst(SMC_LI(target_row, reg_row_addr));
    prog->add_inst(SMC_LI(8, CASR));

    // set up the input data in the wide register
    for(int pos = 0; pos < 16; pos++){
        prog->add_inst(SMC_LI((((pattern >> 32*pos) & bitset_int_mask).to_ulong() & 0xFFFFFFFF), reg_wrdata));
        prog->add_inst(SMC_LDWD(reg_wrdata, pos));
    }

    // activate the target row
    uint remaining = add_op_with_delay(*prog, SMC_ACT(reg_bank_addr, 0, reg_row_addr, 0), 0, trcd_cycles - 5);

    // write data to the row and precharge
    prog->add_inst(SMC_LI(0, reg_col_addr));

    string new_lbl = createSMCLabel("INIT_ROW");
    prog->add_label(new_lbl);
    add_op_with_delay(*prog, SMC_WRITE(reg_bank_addr, 0, reg_col_addr, 1, 0, 0), 0, 0);
    prog->add_branch(Program::BR_TYPE::BL, reg_col_addr, reg_num_cols, new_lbl);
    
    // precharge the open bank
    add_op_with_delay(*prog, SMC_PRE(reg_bank_addr, 0, 0), 0, trp_cycles);

    reg_alloc->free_SMC_REG(reg_row_addr);
    reg_alloc->free_SMC_REG(reg_col_addr);
    reg_alloc->free_SMC_REG(reg_wrdata);
    reg_alloc->free_SMC_REG(reg_bank_addr);
    reg_alloc->free_SMC_REG(reg_num_cols);

}

void init_rs(Program* prog, SoftMCRegAllocator* reg_alloc, const RowSet rs, 
             const bitset<512> vic_pattern, const bitset<512> aggr_pattern) {
    //init aggrs
    for(auto aggr_id: rs.aggr_ids)
        init_row(prog, reg_alloc, rs.bank_id, aggr_id, aggr_pattern);
    //init victims
    for(auto victim_id: rs.victim_ids)
        init_row(prog, reg_alloc, rs.bank_id, victim_id, vic_pattern);
}

void hammer_rs(Program* prog, SoftMCRegAllocator* reg_alloc, const RowSet rs, 
               const uint hc){

    SMC_REG reg_bank_addr = reg_alloc->allocate_SMC_REG();
    SMC_REG reg_row_addr = reg_alloc->allocate_SMC_REG();
    SMC_REG reg_num_hammers = reg_alloc->allocate_SMC_REG();
    SMC_REG reg_cur_hammers = reg_alloc->allocate_SMC_REG();

    prog->add_inst(SMC_LI(rs.bank_id, reg_bank_addr));
    prog->add_inst(SMC_LI(hc, reg_num_hammers));
    prog->add_inst(SMC_LI(0, reg_cur_hammers));

    string lbl_rh = createSMCLabel("ROWHAMMERING");
    prog->add_label(lbl_rh);
    for(auto aggr_id: rs.aggr_ids){
        prog->add_inst(SMC_LI(aggr_id, reg_row_addr));

        uint remaining_cycs = add_op_with_delay(*prog, SMC_ACT(reg_bank_addr, 0, reg_row_addr, 0), 0, tras_cycles - 1);
        remaining_cycs = add_op_with_delay(*prog, SMC_PRE(reg_bank_addr, 0, 0), remaining_cycs, trp_cycles - 25);
    }
    prog->add_inst(SMC_ADDI(reg_cur_hammers, 1, reg_cur_hammers));
    prog->add_branch(Program::BR_TYPE::BL, reg_cur_hammers, reg_num_hammers, lbl_rh);

    reg_alloc->free_SMC_REG(reg_bank_addr);
    reg_alloc->free_SMC_REG(reg_row_addr);
    reg_alloc->free_SMC_REG(reg_num_hammers);
    reg_alloc->free_SMC_REG(reg_cur_hammers);
}

void read_row(Program* prog, SoftMCRegAllocator* reg_alloc, const uint bank_id, const uint row_id){

    SMC_REG reg_bank_addr = reg_alloc->allocate_SMC_REG();
    SMC_REG reg_num_cols = reg_alloc->allocate_SMC_REG();
    SMC_REG reg_row_addr = reg_alloc->allocate_SMC_REG();
    SMC_REG reg_col_addr = reg_alloc->allocate_SMC_REG();

    prog->add_inst(SMC_LI(8, CASR));
    prog->add_inst(SMC_LI(NUM_COLS_PER_ROW*8, reg_num_cols));
    prog->add_inst(SMC_LI(bank_id, reg_bank_addr));
    prog->add_inst(SMC_LI(row_id, reg_row_addr));

    // activate the victim row
    add_op_with_delay(*prog, SMC_ACT(reg_bank_addr, 0, reg_row_addr, 0), 0, trcd_cycles - 5);
    prog->add_inst(SMC_LI(0, reg_col_addr));
    
    // read data from the row and precharge
    string new_lbl = createSMCLabel("READ_ROW");
    prog->add_label(new_lbl);
    add_op_with_delay(*prog, SMC_READ(reg_bank_addr, 0, reg_col_addr, 1, 0, 0), 0, 0);
    prog->add_branch(Program::BR_TYPE::BL, reg_col_addr, reg_num_cols, new_lbl);

    // precharge the open bank
    add_op_with_delay(*prog, SMC_PRE(reg_bank_addr, 0, 0), 0, trp_cycles);

    reg_alloc->free_SMC_REG(reg_bank_addr);
    reg_alloc->free_SMC_REG(reg_num_cols);
    reg_alloc->free_SMC_REG(reg_row_addr);
    reg_alloc->free_SMC_REG(reg_col_addr);
    
}

void read_rs(Program* prog, SoftMCRegAllocator* reg_alloc, const RowSet rs){
    for(auto victim_id: rs.victim_ids)
        read_row(prog, reg_alloc, rs.bank_id, victim_id);
}

vector<uint> collect_bitflips(const char* read_data, 
                              const bitset<512> vic_pattern){

    bitset<512> read_data_bitset;
    vector<uint> bitflips;
    uint32_t* iread_data = (uint32_t*) read_data;
    uint bit_loc;

    // check for bitflips in each cache line
    for(int cl = 0; cl < ROW_SIZE/64; cl++) {
        read_data_bitset.reset();
        for(int i = 0; i < 512/32; i++) {
            bitset<512> tmp_bitset = iread_data[cl*(512/32) + i];

            read_data_bitset |= (tmp_bitset << i*32);
        }
        // compare and print errors
        bitset<512> error_mask = read_data_bitset ^ vic_pattern;
        if(error_mask.any()){
            // there is at least one bitflip in this cache line
            for(uint i = 0; i < error_mask.size(); i++){
                if(error_mask.test(i)){
                    bit_loc = cl*CACHE_LINE_BITS + i;
                    bitflips.push_back(bit_loc);
                }
            }
        }
    }
    
    return bitflips;
}

vector<vector<uint>> get_bitflips(SoftMCPlatform& platform, const RowSet& rs, 
                                  const bitset<512> vic_pattern){
    
    vector<vector<uint>> loc_bitflips;
    uint read_data_size = ROW_SIZE*rs.victim_ids.size();

    char buf[read_data_size*2];

    platform.receiveData(buf, read_data_size);
    for(uint vic_ind = 0; vic_ind < rs.victim_ids.size(); vic_ind++){
        auto bitflips = collect_bitflips(buf + vic_ind*ROW_SIZE, vic_pattern);
        loc_bitflips.push_back(bitflips);
    }
    return loc_bitflips;
}

vector<vector<uint>> run_single_test(SoftMCPlatform& platform, const RowSet rs, const uint hc,
                                     const bitset<512> vic_pattern, const bitset<512> aggr_pattern){

    Program prog;
    SoftMCRegAllocator reg_alloc = SoftMCRegAllocator(NUM_SOFTMC_REGS, reserved_regs);
    
    init_program(prog);
    // INIT DATA
    init_rs(&prog, &reg_alloc, rs, vic_pattern, aggr_pattern);

    // HAMMER 
    hammer_rs(&prog, &reg_alloc, rs, hc);

    // READ DATA
    read_rs(&prog, &reg_alloc, rs);

    // END PROGRAM
    end_program(prog);

    #ifdef PRINT_SOFTMC_PROGS
    std::cout << "--- SoftMCProg ---" << std::endl;
    prog.pretty_print(); // DEBUG
    #endif

    // EXECUTE PROGRAM
    platform.execute(prog);

    return get_bitflips(platform, rs, vic_pattern);
}

RowSet get_rowset(string rh_pattern, uint bank, uint row){
    RowSet rs;

    rs.bank_id = bank;
    for(int i = 0; i < rh_pattern.length(); i++){
        if(rh_pattern.at(i) == 'A')
            rs.aggr_ids.push_back(to_physical_row_id(row + i));
        else if(rh_pattern.at(i) == 'V')
            rs.victim_ids.push_back(to_physical_row_id(row + i));
        else if(rh_pattern.at(i) == 'I')
            rs.isol_ids.push_back(to_physical_row_id(row + i));
    }

    return rs;
}

vector<vector<bitset<512>>> parse_data_patterns(const uint aggr_pattern_select, const uint vic_pattern_select, const uint isol_pattern_select,
                                                const vector<uint> spec_data_patterns){
    
    vector<bitset<512>> aggr_patterns, vic_patterns, isol_patterns;
    bitset<512> aggr_set;
    aggr_set.reset();
    switch (aggr_pattern_select){
        case 0:
            aggr_patterns.push_back(aggr_set);
            break;
        case 1:
            aggr_set.set();
            aggr_patterns.push_back(aggr_set);
            break;
        case 2:
            for(uint b = 0; b < 64; b++){
                aggr_set |= 0x55;
                aggr_set <<= 8;
            }
            aggr_patterns.push_back(aggr_set);
            break;
        case 3:
            for(uint b = 0; b < 64; b++){
                aggr_set |= 0xAA;
                aggr_set <<= 8;
            }
            aggr_patterns.push_back(aggr_set);
            break;
        case 8:
            for(uint b = 0; b < 64; b++){
                aggr_set |= 0x0F;
                aggr_set <<= 8;
            }
            aggr_patterns.push_back(aggr_set);
            break;
        case 9:
            for(uint b = 0; b < 64; b++){
                aggr_set |= 0xF0;
                aggr_set <<= 8;
            }
            aggr_patterns.push_back(aggr_set);
            break;
        case 4:
            for(uint p = 0; p < 256; p++){
                aggr_set.reset();
                for(uint b = 0; b < 64; b++){
                    aggr_set |= p;
                    aggr_set <<= 8;
                }
                aggr_patterns.push_back(aggr_set);
            }
            break;
        case 6:
            for(auto p: spec_data_patterns){
                aggr_set.reset();
                for(uint b = 0; b < 64; b++){
                    aggr_set |= p;
                    aggr_set <<= 8;
                }
                aggr_patterns.push_back(aggr_set);
            }
            break;
        case 7:
            for(uint b = 0; b < 64; b++){
                aggr_set |= spec_data_patterns[b];
                aggr_set <<= 8;
            }
            aggr_patterns.push_back(aggr_set);
            break;
    }

    bitset<512> vic_set;
    vic_set.reset();
    switch (vic_pattern_select){
        case 0:
            vic_patterns.push_back(vic_set);
            break;
        case 1:
            vic_set.set();
            vic_patterns.push_back(vic_set);
            break;
        case 2:
            for(uint b = 0; b < 64; b++){
                vic_set |= 0x55;
                vic_set <<= 8;
            }
            vic_patterns.push_back(vic_set);
            break;
        case 3:
            for(uint b = 0; b < 64; b++){
                vic_set |= 0xAA;
                vic_set <<= 8;
            }
            vic_patterns.push_back(vic_set);
            break;
        case 8:
            for(uint b = 0; b < 64; b++){
                vic_set |= 0x0F;
                vic_set <<= 8;
            }
            vic_patterns.push_back(vic_set);
            break;
        case 9:
            for(uint b = 0; b < 64; b++){
                vic_set |= 0xF0;
                vic_set <<= 8;
            }
            vic_patterns.push_back(vic_set);
            break;
        case 5:
            vic_patterns.push_back(vic_set);
            break;
    }

    bitset<512> isol_set;
    isol_set.reset();
    switch (isol_pattern_select){
        case 0:
            isol_patterns.push_back(isol_set);
            break;
        case 1:
            isol_set.set();
            isol_patterns.push_back(isol_set);
            break;
        case 2:
            for(uint b = 0; b < 64; b++){
                isol_set |= 0x55;
                isol_set <<= 8;
            }
            isol_patterns.push_back(isol_set);
            break;
        case 3:
            for(uint b = 0; b < 64; b++){
                isol_set |= 0xAA;
                isol_set <<= 8;
            }
            isol_patterns.push_back(isol_set);
            break;
        case 8:
            for(uint b = 0; b < 64; b++){
                isol_set |= 0x0F;
                isol_set <<= 8;
            }
            isol_patterns.push_back(isol_set);
            break;
        case 9:
            for(uint b = 0; b < 64; b++){
                isol_set |= 0xF0;
                isol_set <<= 8;
            }
            isol_patterns.push_back(isol_set);
            break;
    }

    vector<vector<bitset<512>>> patterns;
    patterns.push_back(aggr_patterns);
    patterns.push_back(vic_patterns);
    patterns.push_back(isol_patterns);

    return patterns;
}

int main(int argc, char** argv){

    //========================Program Options========================
    string out_filename = "";
    string rh_pattern = "AVA";
    uint target_bank = 0;
    uint target_row = 0;
    string aggr_pattern_select = "SoftMC";
    uint vic_pattern_select = 0;
    uint arg_log_phys_conv_scheme = 0;
    uint hc = 1000;

    if(argc != 6){
        cerr << RED_TXT << "Usage: " << argv[0] << " <hc_per_aggr> <target_row> <aggr_pattern_select> <vic_pattern_select> <out_filename>" << NORMAL_TXT << endl;
        return -1;
    }

    hc = atoi(argv[1]);
    target_row = atoi(argv[2]);
    aggr_pattern_select = argv[3];
    vic_pattern_select = atoi(argv[4]);
    out_filename = argv[5];

    //========================Input Checking========================
    if(arg_log_phys_conv_scheme >= LogPhysRowIDScheme::MAX){
        cerr << RED_TXT << "No logical to physical conversion scheme found." << NORMAL_TXT << endl;
        return -1;
    }
    //========================Platform Config========================
    SoftMCPlatform platform;
    int err;
    if((err = platform.init()) != SOFTMC_SUCCESS){
        cerr << "Could not initialize SoftMC Platform: " << err << endl;
        return err;
    }
    platform.reset_fpga();  
    platform.set_aref(false); // disable refresh
    //========================Out_file Config========================
    std::ofstream out_file;
    if(out_filename != ""){
        out_file.open(out_filename);
    }else{
        out_file.open("/dev/null");
    }
    //========================Parse Inputs========================
    // Logical to physical conversion
    logical_physical_conversion_scheme = (LogPhysRowIDScheme) arg_log_phys_conv_scheme;
    //========================Parameter Calculations========================
    uint total_victims = count(rh_pattern.begin(), rh_pattern.end(), 'V');
    uint total_aggrs = count(rh_pattern.begin(), rh_pattern.end(), 'A');
    //========================INFO MSG========================
    std::cout << "RH_pattern: " << rh_pattern << endl
              << "Hammer Count: " << hc << endl
              << "Target Bank: " << target_bank << endl
              << "Target Row: " << target_row << endl
              << "Aggressor Data Pattern: " << aggr_pattern_select << endl
              << "Victim Data Pattern: " << vic_pattern_select << endl;
    //========================Run Analyzer========================

    RowSet rs = get_rowset(rh_pattern, target_bank, target_row);
    bitset<512> aggr_pattern, vic_pattern;
    for(uint pattern_id = 0; pattern_id < 256; pattern_id++){

        string ap_str;
        if(aggr_pattern_select == "SoftMC"){
            bitset<8> ap_set(pattern_id);
            ap_str = ap_set.to_string();
        }else if(aggr_pattern_select == "DRAM-Bender"){
            bitset<512> ap_set;
            for(uint b = 0; b < 512; b++){
                ap_set[b] = rand() % 2;
            }
            ap_str = "RAND_" + to_string(pattern_id);
        }

        if(vic_pattern_select == 0){
            vic_pattern.reset();
        } else if(vic_pattern_select == 1){
            vic_pattern.set();
        }

        auto loc_bitflips = run_single_test(platform, rs, hc, vic_pattern, aggr_pattern);
            
        uint vic_ind = 0;

        for(auto victim_id: rs.victim_ids){
            out_file << "Victim " << to_logical_row_id(victim_id) << " - Aggr Pat " 
                        << ap_str << ": " << loc_bitflips[vic_ind].size() << " : ";
            for(auto bit_loc: loc_bitflips[vic_ind]){
                out_file << bit_loc << ",";
            }
            vic_ind++;
        }
        out_file << endl;

    }

    std::cout << "The test has finished!" << endl;

    out_file.close();
    return 0;
}