import os,sys
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import warnings
warnings.filterwarnings('ignore')

## module name -- command line argument (e.g., python3 process_which_half.py my_module0)

module = sys.argv[1]

print("Started " + module)

# your experimental data path
#example:  data_path = "/home/<user>/FCDRAM/experimental_data/"  
data_path = "/home/ismail/FCDRAM/experimental_data/"

output_path = data_path + module + '/'


if not os.path.exists(data_path + module + "/open_rows.csv"):
    print("File does not exist " + data_path + module + "/open_rows.csv")
    exit()

if not os.path.exists(output_path):
    os.makedirs(output_path)

open_rows_df = pd.read_csv(data_path + module + "/open_rows.csv")
open_rows_df.drop(set(open_rows_df.columns) - set(['bank_id', 'which_half', 's_id']), axis=1, inplace=True)
open_rows_df = open_rows_df.groupby(['bank_id', 's_id']).agg(lambda x:x.value_counts().index[0]).reset_index()
open_rows_df['which_half'] = open_rows_df['which_half'].apply(lambda x: int((x%8)/4))
open_rows_df.sort_values(['s_id', 'bank_id'], inplace=True)
open_rows_df.to_csv(output_path + "which_half.csv", index=False)

print("Finished " + module)