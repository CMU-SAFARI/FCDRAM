import subprocess
import os
import pandas as pd
import warnings
import time
warnings.simplefilter(action='ignore', category=FutureWarning)

def inner(lst,search_start_addr,search_last_addr,r_first,r_second,range_low,range_high,rc_csv,out_file,exe):
    for t_12 in range(30,31):
        for t_23 in range(1,5):
            
            os.system(f'touch {out_file}')
            cmd = ( exe + " " + str(t_12) + " " + 
                        str(t_23)  + " " + str(r_first) + " " + 
                        str(r_second) + " " + str(search_start_addr) + " " + 
                        str(search_last_addr) + " " + str(out_file) 
                )
            sp = subprocess.run([cmd], shell=True, check=True, stdout=subprocess.PIPE, universal_newlines=True) 
            if(os.stat(out_file).st_size != 0):
                df = pd.read_csv(out_file,header=None) 
                df.columns=['r_first','r_second']
                df['t_12'] = t_12
                df['t_23'] = t_23
                lst = lst.append(df)
                os.system(f'rm {out_file}')
                lst.to_csv(rc_csv, mode='a', header=False)
                return      
                    
def first_search(lst,search_start_addr,search_last_addr, r_first,num_rows,rc_csv,out_file,exe):
    for r_second in range(r_first+1,r_first+num_rows):
        inner(lst,search_start_addr,search_last_addr,r_first,r_second,2,4,rc_csv,out_file,exe)
        
def subarray_list(r_first, csv_file):
    df = pd.read_csv(csv_file)
    subarray_list = [int(i) for i in df['r_second'].unique()]
    subarray_list.insert(0, r_first)
    subarray_list.sort()    
    return subarray_list

