#include "receiveData_parser.h"

void parse_file(std::string file_name, std::vector<uint32_t> &vec)
{
  std::ifstream file;
  file.open(file_name, std::ios::app);
  std::string line;
  while (std::getline(file, line))
  {
    vec.push_back(stol(line));
  }
  file.close();
}
int pattern_index(uint8_t stored_patt, uint8_t wr_patt, uint8_t f_init_patt, uint8_t s_init_patt, uint8_t f_offset_patt, uint8_t s_offset_patt)
{
  if(stored_patt == wr_patt) 
    return 0;
  else if(stored_patt == f_init_patt)
    return 1;
  else if (stored_patt == (uint8_t)(wr_patt ^ 0xFF))
    return 2;
  else if (stored_patt == (uint8_t)(f_init_patt ^ 0xFF))
    return 3;
  else if(stored_patt == s_init_patt)
    return 4;
  else if(stored_patt == (uint8_t)(s_init_patt ^ 0xFF))
    return 5;
  else if (stored_patt == f_offset_patt)
    return 6;
  else if (stored_patt == s_offset_patt)
    return 7;
  else if (stored_patt == (uint8_t)(f_offset_patt ^ 0xFF))
    return 8;
  else if (stored_patt == (uint8_t)(s_offset_patt ^ 0xFF))
    return 9;
  else
    return 10;
}
void fault_positions_twoACT(SoftMCPlatform &platform, 
                    bool plotting,
                    uint32_t bank_id,
                    uint32_t r_first, uint32_t r_second,
                    uint32_t wr_pattern, uint32_t f_init_pattern, uint32_t s_init_pattern,
                    uint32_t f_offset_pattern, uint32_t s_offset_pattern,
                    uint32_t f_start, uint32_t f_end, uint32_t s_start, uint32_t s_end,
                    std::ofstream &out_file)
{
  uint32_t test_pattern = wr_pattern; 
  uint8_t row[8192];
  for (int r_sec = s_start; r_sec < s_end; r_sec++)
  {
    for (int rc  = f_start; rc < s_end; rc++)
    {
      std::vector<std::vector<int>> faulty_bits(11);
      platform.receiveData((void*)row, 8192);
      for(int j = 0 ; j < 128 ; j++){
        for (int k = 0 ; k < 64 ; k++){ 
          uint8_t wr_patt = wr_pattern >> ((uint32_t)(k%4)*8);
          uint8_t f_init_patt = f_init_pattern >> ((uint32_t)(k%4)*8);
          uint8_t s_init_patt = s_init_pattern >> ((uint32_t)(k%4)*8);
          uint8_t f_offset_patt = f_offset_pattern >> ((uint32_t)(k%4)*8);
          uint8_t s_offset_patt = s_offset_pattern >> ((uint32_t)(k%4)*8);
          int vec_idx = pattern_index((uint8_t)row[j*64 + k], wr_patt, f_init_patt, s_init_patt, f_offset_patt, s_offset_patt);
          faulty_bits[vec_idx].push_back(j*64 + k);
        }
      }
      
      uint32_t check_id = rc < s_start ? 1 : 4;
      if(faulty_bits[check_id].size() < 8192)
      {
        if(plotting)
        {
          for(int l = 0; l < 11; l++)
          {
            std::ofstream faulty_bit_file;
            // get the current path
            char path[1024];
            getcwd(path, sizeof(path));
            std::string path_str(path);
            // create the directory if it doesn't exist
            std::string dir_name = path_str + std::string("/faulty_bits");
            //faulty_bit_file.open((std::string("faulty_bits/faulty_bits_") + std::to_string((rc)) + std::string("_")+std::to_string((l))));
            faulty_bit_file.open((dir_name + std::string("/faulty_bits_") + std::to_string((rc)) + std::string("_")+std::to_string((l))));
            for(int i = 0; i < faulty_bits[l].size(); i++)
            {
              faulty_bit_file << (int)faulty_bits[l][i] << std::endl;
            }
            faulty_bit_file.close();
          }
        }
        out_file << "\n" << bank_id << ",";
        out_file <<  f_start << ",";
        out_file <<  s_start << ",";
        out_file <<  r_first << ",";
        out_file <<  r_sec << ",";
        out_file <<  rc << ",";
        if (!faulty_bits[2].empty()) {
            out_file << (int)faulty_bits[2][0]<< ",";
        }
        else {
            out_file << "-1,";
        }
        out_file << (float)faulty_bits[0].size()/8192.0 << ",";
        out_file << (float)faulty_bits[1].size()/8192.0 << ",";
        out_file << (float)faulty_bits[2].size()/8192.0 << ",";
        out_file << (float)faulty_bits[3].size()/8192.0 << ",";
        out_file << (float)faulty_bits[4].size()/8192.0 << ",";
        out_file << (float)faulty_bits[5].size()/8192.0 << ",";
        out_file << (float)faulty_bits[6].size()/8192.0 << ",";
        out_file << (float)faulty_bits[7].size()/8192.0 << ",";
        out_file << (float)faulty_bits[8].size()/8192.0 << ",";
        out_file << (float)faulty_bits[9].size()/8192.0 << ",";
        out_file << (float)faulty_bits[10].size()/8192.0;
        
      }
      for(int i = 0; i < 11; i++)
      {
        faulty_bits[i].clear();
      }
    }
  } 
}

void fault_positions_twoACT_one_pair(SoftMCPlatform &platform, 
                    bool plotting,
                    uint32_t bank_id,
                    uint32_t r_first, uint32_t r_second,
                    uint32_t wr_pattern, uint32_t f_init_pattern, uint32_t s_init_pattern,
                    uint32_t f_offset_pattern, uint32_t s_offset_pattern,
                    uint32_t f_start, uint32_t f_end, uint32_t s_start, uint32_t s_end,
                    std::ofstream &out_file)
{
  uint8_t row[8192];
  for (int rc  = f_start; rc < s_end; rc++)
  {
    std::vector<std::vector<int>> faulty_bits(11);
    platform.receiveData((void*)row, 8192);
    for(int j = 0 ; j < 128 ; j++){
      for (int k = 0 ; k < 64 ; k++){ 
        uint8_t wr_patt = wr_pattern >> ((uint32_t)(k%4)*8);
        uint8_t f_init_patt = f_init_pattern >> ((uint32_t)(k%4)*8);
        uint8_t s_init_patt = s_init_pattern >> ((uint32_t)(k%4)*8);
        uint8_t f_offset_patt = f_offset_pattern >> ((uint32_t)(k%4)*8);
        uint8_t s_offset_patt = s_offset_pattern >> ((uint32_t)(k%4)*8);
        int vec_idx = pattern_index((uint8_t)row[j*64 + k], wr_patt, f_init_patt, s_init_patt, f_offset_patt, s_offset_patt);
        faulty_bits[vec_idx].push_back(j*64 + k);
      }
    }
        
    uint32_t check_id = rc < s_start ? 1 : 4;
    if(faulty_bits[check_id].size() < 8192)
    {
      if(plotting)
      {
        for(int l = 0; l < 11; l++)
        {
          std::ofstream faulty_bit_file;
          // get the current path
          char path[1024];
          getcwd(path, sizeof(path));
          std::string path_str(path);
          // create the directory if it doesn't exist
          std::string dir_name = path_str + std::string("/faulty_bits");
          //faulty_bit_file.open((std::string("faulty_bits/faulty_bits_") + std::to_string((rc)) + std::string("_")+std::to_string((l))));
          faulty_bit_file.open((dir_name + std::string("/faulty_bits_") + std::to_string((rc)) + std::string("_")+std::to_string((l))));
          for(int i = 0; i < faulty_bits[l].size(); i++)
          {
            faulty_bit_file << (int)faulty_bits[l][i] << std::endl;
          }
          faulty_bit_file.close();
        }
      }
      out_file << "\n" << bank_id << ",";
      out_file <<  f_start << ",";
      out_file <<  s_start << ",";
      out_file <<  r_first << ",";
      out_file <<  r_second << ",";
      out_file <<  rc << ",";
      if (!faulty_bits[2].empty()) {
          out_file << (int)faulty_bits[2][0]<< ",";
      }
      else {
          out_file << "-1,";
      }
      out_file << (float)faulty_bits[0].size()/8192.0 << ",";
      out_file << (float)faulty_bits[1].size()/8192.0 << ",";
      out_file << (float)faulty_bits[2].size()/8192.0 << ",";
      out_file << (float)faulty_bits[3].size()/8192.0 << ",";
      out_file << (float)faulty_bits[4].size()/8192.0 << ",";
      out_file << (float)faulty_bits[5].size()/8192.0 << ",";
      out_file << (float)faulty_bits[6].size()/8192.0 << ",";
      out_file << (float)faulty_bits[7].size()/8192.0 << ",";
      out_file << (float)faulty_bits[8].size()/8192.0 << ",";
      out_file << (float)faulty_bits[9].size()/8192.0 << ",";
      out_file << (float)faulty_bits[10].size()/8192.0;
      
    }
    for(int i = 0; i < 11; i++)
    {
      faulty_bits[i].clear();
    }
  }
} 
