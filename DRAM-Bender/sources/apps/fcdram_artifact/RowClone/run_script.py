import subprocess
import os
import pandas as pd
import warnings
import rowclone_functions as rf
from random import randrange
import argparse
warnings.simplefilter(action='ignore', category=FutureWarning)

parser = argparse.ArgumentParser()
#module type
parser.add_argument('--module', type=str, required=True, help='Module type: my_module0')
parser.add_argument('--apps_path', type=str, required=True, help='Path to apps folder')
parser.add_argument('--result_path', type=str, required=True, help='Path to result folder')
args = parser.parse_args()
module = args.module
apps_path = args.apps_path
result_path = args.result_path


artifact_folder = 'fcdram_artifact/'

exe_path ="RowClone/"
exe_file ="row-clone-exe"

exe = apps_path + artifact_folder + exe_path + exe_file
out_file = apps_path + artifact_folder + exe_path + "row_clone.txt"
rc_csv = apps_path + artifact_folder + exe_path + 'row-clone.csv'
sa_csv = apps_path + artifact_folder + exe_path+ 'all_subarrays.csv'

r_first = 0
num_rows = 1024
counter = 0
csv_lst = []

os.system(f'rm {rc_csv}')
os.system(f'rm {sa_csv}')
os.system(f'rm {out_file}')
os.system(f'{apps_path}ResetBoard/full_reset.sh')

csv_lst = []

#number of rows to search
n_rows = 65000

while r_first < n_rows:
    lst = pd.DataFrame(columns=['r_first','r_second','t_12','t_23'])
    lst.to_csv(rc_csv)
    print(f'Search in {r_first} - {r_first+num_rows}')
    rf.first_search(lst,r_first,r_first+num_rows,r_first,num_rows,rc_csv,out_file,exe)    
    subarray_list = rf.subarray_list(r_first,rc_csv)
    csv_lst.append([len(subarray_list),counter,subarray_list])
    r_first = subarray_list[-1] + 1
    
csv_lst = pd.DataFrame(csv_lst,columns=['n_rows','group','rows'])
csv_lst.to_csv(sa_csv)

send_file = sa_csv
send_cmd = f'cp {send_file} {result_path}'  
sp = subprocess.run([send_cmd], shell=True, check=True, stdout=subprocess.PIPE, universal_newlines=True)

