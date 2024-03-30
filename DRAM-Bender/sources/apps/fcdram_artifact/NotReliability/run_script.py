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
parser.add_argument('--module', type=str, required=True, help='Module type: hytgXX, hyttXX')
parser.add_argument('--temperature', type=str, required=True, help='Temp: 50, 60, etc.')
parser.add_argument('--apps_path', type=str, required=True, help='Path to apps folder')
parser.add_argument('--result_path', type=str, required=True, help='Path to result folder')
args = parser.parse_args()
module = args.module
temperature = args.temperature
apps_path = args.apps_path
result_path = args.result_path



artifact_folder = 'fcdram_artifact/'

exe_path ="NotReliability/"
exe_file ="not-exe"

pair_path = apps_path + artifact_folder + 'RowPairs'
output_path = apps_path + artifact_folder + exe_path + 'NotCoverage'


## get the subarray boundaries (the output of the RowClone experiment)
sa_csv = result_path + 'all_subarrays.csv'
s_df = pd.read_csv(sa_csv)
subarrays = hf.create_subarray_list(s_df)


## get the processed which sides of the subarrays are connected (i.e., even cells or odd cells)
wh_csv = result_path + 'which_half.csv'
which_half_df = pd.read_csv(wh_csv)

os.system(f'rm {output_path}/*')
try:
    os.system(f'rm {pair_path}/*')
except:
    pass

try:
    os.mkdir(pair_path)
except:
    pass
try:
    os.mkdir(output_path)
except:
    pass

os.system(f'{apps_path}ResetBoard/full_reset.sh')
exe = apps_path + artifact_folder + exe_path + exe_file

## get the simultaneously activated (open) rows data

sa_csv = result_path + 'open_rows.csv'
s_df = pd.read_csv(sa_csv)
s_df['open_upper_indices'] = s_df['open_upper_indices'].str.replace('[','').str.replace(']','').str.replace(' ','').str.split(',').apply(lambda x: [int(i) for i in x])
s_df['open_lower_indices'] = s_df['open_lower_indices'].str.replace('[','').str.replace(']','').str.replace(' ','').str.split(',').apply(lambda x: [int(i) for i in x])

n_samples = 100

for total_open_row in s_df['total_open_row'].unique():
    if total_open_row > 64:
        continue
    csv_file = f'{pair_path}/row_pairs_{total_open_row}.csv'
    if(os.path.isfile(csv_file)):
        continue
    #lst.to_csv(csv_file)
    for bank_id in range(16):
        for s_id in s_df['s_id'].unique():
            print("Running for bank_id: ", bank_id, " s_id: ", s_id, " total_open_row: ", total_open_row)
            samples_df = s_df[((s_df['s_id'] == s_id) & (s_df['total_open_row'] == total_open_row)) & (s_df['bank_id'] == bank_id)]
            samples_df = samples_df.reset_index(drop=True)
            if(len(samples_df) == 0):
                continue
            elif(len(samples_df) < n_samples):
                samples_df = samples_df.sample(n=len(samples_df)).reset_index(drop=True)
            else:
                samples_df = samples_df.sample(n=n_samples).reset_index(drop=True)
            #append samples_df to csv_file
            if(os.path.isfile(csv_file)):
                samples_df.to_csv(csv_file, mode='a', header=False, index=False)
            else:
                samples_df.to_csv(csv_file, mode='a', header=True, index=False)   

upper_txt = apps_path + artifact_folder + exe_path + 'upper.txt'
lower_txt = apps_path + artifact_folder + exe_path + 'lower.txt'

out_file = apps_path + artifact_folder + exe_path + 'out.txt'
os.system(f'rm {out_file}')

t_12 = 30
t_23 = 1
num_iter = 1000

columns_df = ['t_12','t_23', 'bank_id','s_id', 'r_first','r_second', 'len_upper_indices', 'len_lower_indices',
              'avg_rowclone_stability','rowclone_success_rate','avg_not_stability','not_success_rate']

for total_open_row in s_df['total_open_row'].unique():
    if total_open_row > 64:
        continue
    
    not_lst = pd.DataFrame(columns=columns_df)
    result_csv_file = f'{output_path}/not_coverage_{total_open_row}_{temperature}.csv'
    not_lst.to_csv(result_csv_file)
    csv_file = f'{pair_path}/row_pairs_{total_open_row}.csv'
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
        hf.write_to_file(open_upper_indices,upper_txt)
        hf.write_to_file(open_lower_indices,lower_txt)
        cmd = ( apps_path + artifact_folder + exe_path + exe_file + " " + 
                str(r_first)  + " " + str(r_second) + " " + 
                str(upper_txt) + " " + str(lower_txt)  + " " + str(num_iter)  + " " +
                str(t_12) + " " + str(t_23) + " " + str(which_half) + " " +
                str(bank_id) + " " +str(out_file) 
        )   
        sp = subprocess.run([cmd], shell=True, check=True, stdout=subprocess.PIPE, universal_newlines=True) 
        if(os.stat(out_file).st_size != 0):
            res_lst = (hf.read_result_file(out_file))
            temp_data = [[t_12, t_23, bank_id, s_id, r_first, r_second, len(open_upper_indices), len(open_lower_indices),
                        res_lst[0], res_lst[1], res_lst[2], res_lst[3]]]
            test_df = pd.DataFrame(temp_data, columns=columns_df)
            test_df.to_csv(result_csv_file, mode='a', header=False)
        os.system(f'rm {out_file}')
        os.system(f'rm {upper_txt}')
        os.system(f'rm {lower_txt}')
        
for send_path in [pair_path, output_path]:
    send_cmd = f'cp -r {send_path} {result_path}'  
    print(send_cmd)
    sp = subprocess.run([send_cmd], shell=True, check=True, stdout=subprocess.PIPE, universal_newlines=True)