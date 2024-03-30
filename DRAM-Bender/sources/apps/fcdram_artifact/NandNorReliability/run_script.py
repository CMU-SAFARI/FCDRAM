import subprocess
import os
import pandas as pd
import warnings
import helper_functions as hf
import random
import argparse


warnings.simplefilter(action='ignore', category=FutureWarning)

parser = argparse.ArgumentParser()
#module type
parser.add_argument('--module', type=str, required=True, help='Module type: my_module0')
parser.add_argument('--temperature', type=str, required=True, help='Temp: 50, 60, etc.')
parser.add_argument('--apps_path', type=str, required=True, help='Path to apps folder')
parser.add_argument('--result_path', type=str, required=True, help='Path to result folder')
args = parser.parse_args()
module = args.module
temperature = args.temperature
apps_path = args.apps_path
result_path = args.result_path



artifact_folder = 'fcdram_artifact/'


exe_path ="NandNorReliability/"
exe_file ="not-exe"
output_path = apps_path + artifact_folder + exe_path + 'NandNorSR'

row_pairs_path = result_path  + 'RowPairs'

out_file = apps_path + artifact_folder + exe_path + "not.txt"
os.system(f'rm {apps_path}{artifact_folder}{exe_path}out*')

sa_csv = result_path + 'open_rows.csv'
s_df = pd.read_csv(sa_csv)

wh_csv = result_path + 'which_half.csv'
which_half_df = pd.read_csv(wh_csv)

os.system(f'rm {output_path}/*')

try:
    os.mkdir(apps_path + artifact_folder + exe_path + 'patterns')
except:
    pass
try:
    os.mkdir(output_path)
except:
    pass

os.system(f'{apps_path}ResetBoard/full_reset.sh')
exe = apps_path + artifact_folder + exe_path + exe_file

ref_idx_file_name = apps_path + artifact_folder + exe_path + 'ref.txt'
input_idx_file_name = apps_path + artifact_folder + exe_path + 'input.txt'

out_file = apps_path + artifact_folder + exe_path + 'out.txt'
os.system(f'rm {out_file}')

### Timing delays -> ACT-PRE = t_12, PRE-ACT = t_23
## 0 means 1.5ns, 1 means 3ns, 2 means 4.5ns, etc.
t_12 = 0
t_23 = 0

num_iter = 10000
input_location = 1

columns_df = ['t_12','t_23','n_frac_rows','n_frac','t_frac', 'bank_id','s_id','input_location',
              'r_first','r_second', 'ref_operation','input_operation', 
              'n_ref_rows', 'n_input_rows', 'n_ones', 'random_pattern',
              'avg_ref_stability','ref_success_rate','avg_input_stability','input_success_rate']

for total_open_row in s_df['total_open_row'].unique():
    if total_open_row > 64 or total_open_row < 3:
        continue
    
    nand_lst = pd.DataFrame(columns=columns_df)
    result_csv_file = f'{output_path}/nand_coverage_{total_open_row}_{temperature}.csv'
    nand_lst.to_csv(result_csv_file)
    csv_file = f'{row_pairs_path}/row_pairs_{total_open_row}.csv'
    print("Running for csv_file: ", csv_file)
    df = pd.read_csv(csv_file)
    df['open_upper_indices'] = df['open_upper_indices'].str.replace('[','').str.replace(']','').str.replace(' ','').str.split(',').apply(lambda x: [int(i) for i in x])
    df['open_lower_indices'] = df['open_lower_indices'].str.replace('[','').str.replace(']','').str.replace(' ','').str.split(',').apply(lambda x: [int(i) for i in x])

    for sample_iter, e_idx in enumerate(df.index):
        element = df.iloc[[e_idx]]
        r_first = element['r_first'][e_idx]
        r_second = element['r_second'][e_idx]
        s_id = element['s_id'][e_idx]
        open_upper_indices = element['open_upper_indices'][e_idx]
        open_lower_indices = element['open_lower_indices'][e_idx]
        bank_id = element['bank_id'][e_idx]
        which_half = which_half_df[(which_half_df['s_id'] == s_id) & (which_half_df['bank_id'] == bank_id)]['which_half'].values[0]

        for input_location in [0,1]:            
            if input_location == 1:
                #means upper is the reference
                #lower is the operands
                input_indices = open_lower_indices
                ref_indices = open_upper_indices
            else:
                #means lower is the reference
                #upper is the operands
                n_frac_times = 0
                input_indices = open_upper_indices
                ref_indices = open_lower_indices
            if len(input_indices) == len(ref_indices):
                n_frac_times = 5
                t_frac = 0
                n_frac_rows = 1
            elif len(ref_indices)*2 == len(input_indices):
                n_frac_times = 5
                t_frac = 0
                n_frac_rows = 0
            else:
                continue   
            hf.write_to_file(input_indices,input_idx_file_name)
            hf.write_to_file(ref_indices,ref_idx_file_name)
            
            for operation in ['AND','OR']:
                ref_pattern = hf.ref_patt_generator(operation,ref_indices,r_first,input_location)
                ref_pattern_file_name = f'{apps_path}{artifact_folder}{exe_path}patterns/ref_pattern_{operation}.txt'
                hf.write_to_file(ref_pattern,ref_pattern_file_name)
                for i,n_ones in enumerate(range(0,len(input_indices)+2)):
                    is_rand = 0
                    input_pattern = hf.one_pattern_creator(n_ones,len(input_indices))
                    if n_ones > len(input_indices):
                        is_rand = 1
                        input_pattern = hf.one_pattern_creator(n_ones-1,len(input_indices))
                
                    for n_frac_rows in [n_frac_rows]:
                        for n_frac_times in [n_frac_times]:
                            for t_12 in [t_12]:
                                for t_23 in [t_23]:
                                    input_pattern_file_name = f'{apps_path}{artifact_folder}{exe_path}patterns/input_pattern_{operation}_{i}_{len(input_indices)}.txt'
                                    hf.write_to_file(input_pattern,input_pattern_file_name)
                                    out_file_name = f'{out_file}_{i}_{n_ones}_{operation}'
                                    os.system(f'touch {out_file_name}')          

                                    cmd = ( apps_path + artifact_folder + exe_path + exe_file + " " + 
                                            str(r_first)  + " " + str(r_second) + " " + 
                                            str(t_12) + " " + str(t_23)  + " " + str(input_location)  + " " +
                                            str(input_pattern_file_name) + " " + str(ref_pattern_file_name) + " " + 
                                            str(input_idx_file_name) + " " + str(ref_idx_file_name) + " " + 
                                            str(n_frac_times) + " " + str(t_frac) + " " + str(n_frac_rows) + " " + 
                                            str(which_half) + " " + str(num_iter) + " " + str(is_rand) + " " + 
                                            str(bank_id) + " " +str(out_file_name) 
                                        )
                                    sp = subprocess.run([cmd], shell=True, check=True, stdout=subprocess.PIPE, universal_newlines=True) 
                                    if(os.stat(out_file_name).st_size != 0):
                                        res_lst = (hf.read_result_file(out_file_name))
                                        if(input_location == 0):
                                            temp_data = [[t_12, t_23, n_frac_rows, n_frac_times, t_frac, bank_id, s_id, input_location, 
                                                        r_first, r_second, f'N{operation}',operation, 
                                                        len(ref_indices), len(input_indices), n_ones, is_rand,
                                                            res_lst[2], res_lst[3], res_lst[0], res_lst[1]]]
                                        else:
                                            temp_data = [[t_12, t_23, n_frac_rows, n_frac_times, t_frac, bank_id, s_id, input_location, 
                                                        r_first, r_second, f'N{operation}',operation, 
                                                        len(ref_indices), len(input_indices), n_ones, is_rand,
                                                            res_lst[0], res_lst[1], res_lst[2], res_lst[3]]]
                                        test_df = pd.DataFrame(temp_data, columns=columns_df)
                                        test_df.to_csv(result_csv_file, mode='a', header=False)
                        
                                    os.system(f'rm {out_file_name}')
                                    os.system(f'rm {input_pattern_file_name}') 

                
            

for send_path in [output_path]:            
    send_cmd = f'cp -pr {send_path} {result_path}'  
    print(send_cmd)
    sp = subprocess.run([send_cmd], shell=True, check=True, stdout=subprocess.PIPE, universal_newlines=True)