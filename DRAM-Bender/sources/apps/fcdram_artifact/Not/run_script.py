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
parser.add_argument('--apps_path', type=str, required=True, help='Path to apps folder')
parser.add_argument('--result_path', type=str, required=True, help='Path to result folder')
args = parser.parse_args()
module = args.module
apps_path = args.apps_path
result_path = args.result_path

artifact_folder = 'fcdram_artifact/'
exe_path ="Not/"
exe_file ="not-exe"



out_file = apps_path + artifact_folder + exe_path + "not.txt"
os.system(f'rm {out_file}')

sa_csv = f'{result_path}all_subarrays.csv'
s_df = pd.read_csv(sa_csv)
subarrays = hf.create_subarray_list(s_df)


os.system(f'{apps_path}ResetBoard/full_reset.sh')


exe = apps_path + artifact_folder + exe_path + exe_file




not_column_lst = ['t_12','t_23','bank_id','s_id','f_start','s_start','r_first','r_second','which_half','row_addr', 
               'wr_p','f_init_p','n_wr_p','n_f_init_p','s_init_p','n_s_init_p',
               'f_offset_p','s_offset_p','n_f_offset_p','n_s_offset_p','sth_else']

detailed_column_lst = ['bank_id','n_rows','f_start','s_start','r_first','r_second','which_half','row_addr','wr_p','f_init_p','n_wr_p','n_f_init_p','s_init_p','n_s_init_p',
            'f_offset_p','s_offset_p','n_f_offset_p','n_s_offset_p','sth_else']

or_detailed_csv = apps_path + artifact_folder + exe_path + 'open_rows_detailed.csv'
os.system(f'rm {or_detailed_csv}') 
a = pd.DataFrame(columns=detailed_column_lst)
a.to_csv(or_detailed_csv)

not_csv = apps_path + artifact_folder + exe_path + 'not.csv'
os.system(f'rm {not_csv}')
lst = pd.DataFrame(columns=not_column_lst)
lst.to_csv(not_csv) 

or_csv = apps_path + artifact_folder + exe_path + 'open_rows.csv'
os.system(f'rm {or_csv}')

open_rows_lst = ['bank_id','s_id','r_first','r_second','which_half','total_open_row', 'upper_n_row', 'lower_n_row', 'open_upper_indices', 'open_lower_indices']
a_lst = pd.DataFrame(lst, columns=open_rows_lst)
a_lst.to_csv(or_csv)

random_subarray_csv = apps_path + artifact_folder + exe_path + 'random_subarrays_list.csv'

if(os.path.isfile(random_subarray_csv)):
    random_subarray_df = pd.read_csv(random_subarray_csv)
    subarray_list = hf.create_subarray_list(random_subarray_df)
else:
    n_subarray_pairs = 4
    while True:
        random_subarray_pairs = hf.create_random_subarray_pairs(subarrays, n_subarray_pairs)
        if hf.check_subarray_rows(subarrays, random_subarray_pairs) == 1:
            break
    subarray_list = []
    for i in range(len(random_subarray_pairs)):
        subarray_list.append(subarrays[random_subarray_pairs[i]])
    hf.create_random_subarray_csv(s_df, random_subarray_pairs, random_subarray_csv)    
    


t_12 = 30
t_23 = 1
bank_id = 1

for s_id,subarray in enumerate(subarray_list):
    if(s_id == len(subarray_list)-1):
        break
    f_start = subarray[0]
    f_end = subarray[-1]
    s_start= subarray_list[s_id+1][0]
    s_end = subarray_list[s_id+1][-1]
    #pretty print
    print(f'Running for: Subarray {s_id} Start:{f_start} End:{f_end}  |||  Subarray {s_id+1} Start:{s_start} End:{s_end} ')
    if(f_end + 1 != s_start):
        continue

    r_firsts = subarray
    r_seconds = subarray_list[s_id+1]
    plotting = 0

    for r_first in r_firsts:
        for bank_id in range(16):
            lst = pd.DataFrame(columns=not_column_lst)
            lst.to_csv(not_csv) 
            os.system(f'touch {out_file}')
            cmd = ( exe + " " + str(plotting) + " " + str(r_first) + " " + str(r_seconds[0]) + " " +
                        str(f_start)  + " " + str(f_end) + " " + 
                        str(s_start)  + " " + str(s_end) + " " + 
                        str(t_12)  + " " + str(t_23) + " " + 
                        str(bank_id) + " " + str(out_file) 
                )  
            sp = subprocess.run([cmd], shell=True, check=True, stdout=subprocess.PIPE, universal_newlines=True) 
            #print(sp.stdout)
            if(os.stat(out_file).st_size != 0):
                df = pd.read_csv(out_file, header=None)
                df.columns=['bank_id','f_start','s_start','r_first','r_second','row_addr','which_half','wr_p','f_init_p','n_wr_p','n_f_init_p','s_init_p','n_s_init_p','f_offset_p','s_offset_p','n_f_offset_p','n_s_offset_p','sth_else']
                df['t_12'] = t_12
                df['t_23'] = t_23
                df['s_id'] = s_id
                #reorder the columns according to the csv file
                df = df[not_column_lst]
                df.to_csv(not_csv, mode='a', header=False)   
                os.system(f'rm {out_file}')
            df = pd.read_csv('not.csv')
            df['r_first'] = df['r_first'] - df['f_start']
            df['r_second'] = df['r_second'] - df['f_start']
            df['row_addr'] = df['row_addr'] - df['f_start']
            #make multiple rows that have the same r_first and r_second and *_p values into one row and create a column that have the row_addr for each row
            df2 = df.groupby(['bank_id','r_first','r_second','f_start','s_start','which_half','wr_p','f_init_p','n_wr_p','n_f_init_p','s_init_p','n_s_init_p', 
                        'f_offset_p','s_offset_p','n_f_offset_p','n_s_offset_p','sth_else']).agg({'row_addr': lambda x: list(x)}).reset_index()
        #put the row_addr as the 3rd column
            df2['n_rows'] = df2['row_addr'].apply(lambda x: len(x))
            df2 = df2[detailed_column_lst]
            df2.to_csv(or_detailed_csv, mode='a', header=False) 
            #os.system(f'rm {not_csv}')  
    #exit(0)
            lst = []
            df = pd.read_csv('not.csv')
            f_starts = df['f_start'].unique()
            for f_start in f_starts:
                x = df.loc[df['f_start'] == f_start]
                r_firsts = x['r_first'].unique()
                r_seconds = x['r_second'].unique()
                for r_first in r_firsts:
                    for r_second in r_seconds:
                        y = x.loc[((x['r_first'] == r_first) & (x['r_second'] == r_second))]
                        upper_subarray_addr = y.loc[y['wr_p'] > 0.98, 'row_addr']
                        lower_subarray_addr = y.loc[y['n_wr_p'] > 0.48, 'row_addr']
                        upper_subarray_addr = [int(ii) for ii in upper_subarray_addr]
                        lower_subarray_addr = [int(ii) for ii in lower_subarray_addr]
                        if(len(upper_subarray_addr) == 0 or len(lower_subarray_addr) == 0):
                            continue
                        else:
                            which_half = int(y.loc[y['n_wr_p'] > 0.48, 'which_half'].iloc[0])
                            lst.append([bank_id,s_id,r_first, r_second, which_half, len(upper_subarray_addr) + len(lower_subarray_addr),
                                        len(upper_subarray_addr), len(lower_subarray_addr), 
                                        upper_subarray_addr, lower_subarray_addr])

                
            a_lst = pd.DataFrame(lst, columns=open_rows_lst)

            a_lst.to_csv(or_csv, mode='a', header=False)

            os.system(f'rm {not_csv}')  

for send_file in [random_subarray_csv, or_csv, or_detailed_csv]:
    send_cmd = f'cp -r {send_file} {result_path}'  
    sp = subprocess.run([send_cmd], shell=True, check=True, stdout=subprocess.PIPE, universal_newlines=True)