import os,sys
import pandas as pd
import warnings
warnings.filterwarnings('ignore')

## module name -- command line argument (e.g., python3 filter_not_data.py my_module0)

module = sys.argv[1]

print("Started " + module)
# your experimental data path
#example:  data_path = "/home/<user>/FCDRAM/experimental_data/"  
data_path = "/home/ismail/FCDRAM/experimental_data/"


output_path = data_path + module + '/'


if not os.path.exists(data_path + module + "/open_rows_detailed.csv"):
    print("File does not exist " + module)
    exit()

if not os.path.exists(output_path):
    os.makedirs(output_path)

df = pd.read_csv(data_path + module + "open_rows_detailed.csv")
# drop unnecessary columns
df.drop(['Unnamed: 0', 'which_half'], axis=1, inplace=True)
df.drop(['f_offset_p', 's_offset_p', 'n_f_offset_p', 'n_s_offset_p', 'sth_else'], axis=1, inplace=True)
# convert row_addr col to list of int
df['row_addr'] = df['row_addr'].apply(lambda x: [int(i) for i in x[1:-1].split(',')])
df['subarray_size'] = df['s_start'] - df['f_start']

df['subarray'] = df.apply(lambda x: 0 if x['row_addr'][0] < x['subarray_size'] else 1, axis=1)
df.drop_duplicates(subset=['subarray', 'bank_id', 'f_start', 's_start', 'r_first', 'r_second'], inplace=True, keep=False)
df = df[df['n_rows'].isin([1, 2, 4, 8, 16, 32, 64])]

first_df = df[df['subarray'] == 0]
second_df = df[df['subarray'] == 1]
first_df.drop(['subarray'], axis=1, inplace=True)
second_df.drop(['subarray'], axis=1, inplace=True)
merged_df = pd.merge(first_df, second_df, on=['bank_id', 'f_start', 's_start', 'r_first', 'r_second'], how='inner', suffixes=('_f', '_s'))
merged_df['n_rows'] = merged_df['n_rows_f'].astype(str) + "-" + merged_df['n_rows_s'].astype(str)
final_df = merged_df.copy()
final_df.drop(['f_init_p_f', 'n_wr_p_f', 'n_f_init_p_f', 's_init_p_f', 'n_s_init_p_f', 'wr_p_s', 'f_init_p_s', 'n_f_init_p_s', 's_init_p_s', 'n_s_init_p_s'], axis=1, inplace=True)
final_df['n_rows_f'] = final_df['n_rows_f'].astype(int)
final_df['n_rows_s'] = final_df['n_rows_s'].astype(int)
final_df = final_df[(final_df['n_rows_f'] == final_df['n_rows_s']) | (final_df['n_rows_f']*2 == final_df['n_rows_s'])]

final_df.to_csv(output_path + "filtered_not.csv", index=False)
print("Finished " + module)