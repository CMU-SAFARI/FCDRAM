import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os
import sys
import warnings
warnings.filterwarnings('ignore')


#your tested modules
modules = ["my_module0"]

# your experimental data path
#example:  data_path = "/home/<user>/FCDRAM/experimental_data/"  
data_path = "/home/ismail/FCDRAM/experimental_data/"


# your plots path
# example: output_path = "/home/<user>/FCDRAM/analysis/plots/"
output_path = "/home/ismail/FCDRAM/analysis/plots/"

not_df_df = pd.DataFrame()
for module in modules:
    result_path = data_path + module + "/"
    filename = result_path + "open_rows.csv"
    if not os.path.exists(filename):
        print("Dir does not exist " + filename)
        exit()
        
    t_df = pd.read_csv(filename)

    a_df = pd.read_csv(result_path + "random_subarrays_list.csv")

    a_df['f_start'] = a_df.apply(lambda x: int(x['rows'][1:20].split(",")[0]), axis=1)
    a_df.drop(columns=['group', 'rows'], inplace=True)
    a_df.rename(columns={'Unnamed: 0': 's_id', 'n_rows': 'subarray_size'}, inplace=True)
    a_df.drop(columns=['Unnamed: 0.1'], inplace=True)
    a_df['s_id'] = np.arange(0, len(a_df))
    t_df = t_df.merge(a_df, on='s_id', how='left')
    t_df['s_id2'] = t_df['s_id'] + 1
    t_df.rename(columns={'subarray_size': 's_size_f'}, inplace=True)
    a_df.rename(columns={'s_id': 's_id2'}, inplace=True)
    t_df = t_df.merge(a_df, on='s_id2', how='left')
    t_df.rename(columns={'subarray_size': 's_size_s', 'f_start_x': 'f_start'}, inplace=True)
    t_df.drop(columns=['s_id2', 'f_start_y'], inplace=True)
    t_df['module'] = module

    not_df_df = pd.concat([not_df_df, t_df])
not_df_df.drop(columns=['Unnamed: 0', 'which_half', 'total_open_row', 'upper_n_row', 'lower_n_row'], inplace=True)

nand_dp_df = pd.DataFrame()
for module in modules:
    result_path = data_path + module + "/NandNorSR/"
    for filename in os.listdir(result_path):
        if not filename.endswith("50.csv") or len(filename.split("_")) != 4:
            continue
        if not os.path.exists(result_path + filename):
            print("Dir does not exist " + result_path + filename)
            exit()
        n_rows_total = int(filename.split("_")[2])
        temp = int(filename.split("_")[3][:-4])
        if module.startswith("hytg"):
            if not n_rows_total in [2, 4, 8, 16, 32]:
                continue
        elif module.startswith("hytt") or module.startswith("hyhy"):
            if not n_rows_total in [3, 4, 6, 8, 12, 16, 24, 32]:
                continue
            
        t_df = pd.read_csv(result_path + filename)
        if module.startswith("hytg"):
            t_df = t_df[t_df['n_ref_rows'] == t_df['n_input_rows']].copy()
        elif module.startswith("hytt") or module.startswith("hyhy"):
            t_df = t_df[(t_df['n_ref_rows'] == t_df['n_input_rows']) | (t_df['n_ref_rows'] * 2 == t_df['n_input_rows'])].copy()
        t_df = t_df[t_df['input_location'] == 1].copy()
        t_df['module'] = module
        t_df['n_rows_total'] = n_rows_total
        t_df['n_rows'] = t_df['n_ref_rows'].astype(str) + "-" + t_df['n_input_rows'].astype(str)
        t_df['temp'] = temp
        t_df = t_df[t_df['n_input_rows'].isin([2, 4, 8, 16])].copy()
        nand_dp_df = pd.concat([nand_dp_df, t_df])


nand_dp_df.sort_values(by=['n_rows_total'], inplace=True)
nand_dp_df.drop(['Unnamed: 0', 't_12', 't_23', 'n_frac_rows', 'n_frac', 't_frac', 'input_location', 'ref_success_rate', 'input_success_rate', 'n_ones', 'input_operation'], axis=1, inplace=True)
nand_dp_df.replace({'random_pattern': {0: 'All 1s/0s', 1: 'Random'}}, inplace=True)
nand_dp_df.rename(columns={'random_pattern': 'data_pattern'}, inplace=True)

nand_dp_df = nand_dp_df[(nand_dp_df['data_pattern'] == "Random")].copy()

nand_dp_df = nand_dp_df.melt(id_vars=['bank_id', 's_id', 'r_first', 'r_second', 'ref_operation', 'n_ref_rows',
                            'n_input_rows', 'data_pattern', 'module', 'n_rows_total', 'n_rows', 'temp'], 
                            var_name='operation',
                            value_vars=['avg_ref_stability', 'avg_input_stability'],
                            value_name='success_rate')

nand_dp_df.loc[(nand_dp_df['operation'] == 'avg_ref_stability') & (nand_dp_df['ref_operation'] == 'NAND'), 'operation'] = 'NAND'
nand_dp_df.loc[(nand_dp_df['operation'] == 'avg_ref_stability') & (nand_dp_df['ref_operation'] == 'NOR'), 'operation'] = 'NOR'
nand_dp_df.loc[(nand_dp_df['operation'] == 'avg_input_stability') & (nand_dp_df['ref_operation'] == 'NAND'), 'operation'] = 'AND'
nand_dp_df.loc[(nand_dp_df['operation'] == 'avg_input_stability') & (nand_dp_df['ref_operation'] == 'NOR'), 'operation'] = 'OR'

nand_dp_df.drop(['ref_operation', 'data_pattern'], axis=1, inplace=True)


nand_df = not_df_df.merge(nand_dp_df, how='inner', on=['bank_id', 's_id', 'r_first', 'r_second', 'module'])

nand_df['first_th0'] = nand_df['s_size_f']/3
nand_df['first_th1'] = nand_df['s_size_f']*2/3
nand_df['second_th0'] = nand_df['s_size_f'] + nand_df['s_size_s']/3
nand_df['second_th1'] = nand_df['s_size_f'] + nand_df['s_size_s']*2/3

def check_proximity(row_list_f, row_list_s, first_th0, first_th1, first_th2, second_th0, second_th1, second_th2, f_start):
    proximity_f = ""
    proximity_s = ""
    row_list_f = row_list_f[1:-1].split(",")
    row_list_s = row_list_s[1:-1].split(",")
    proximity_list = []
    for row in row_list_f:
        row = int(row) - f_start
        if row <= first_th0:
            proximity_list.append("F")
        elif row <= first_th1:
            proximity_list.append("M")
        elif row <= first_th2:
            proximity_list.append("C")
        else:
            proximity_list.append("ERROR")
    if "ERROR" in proximity_list or len(set(proximity_list)) != 1:
        return "-1"
    proximity_f = proximity_list[0]

    proximity_list = []
    for row in row_list_s:
        row = int(row) - f_start
        if row <= second_th0 and row > first_th2:
            proximity_list.append("C")
        elif row <= second_th1:
            proximity_list.append("M")
        elif row <= second_th2:
            proximity_list.append("F")
        else:
            proximity_list.append("ERROR")
    if "ERROR" in proximity_list or len(set(proximity_list)) != 1:
        return "-1"
    proximity_s = proximity_list[0]
    return proximity_f + proximity_s

nand_df['proximity'] = nand_df.apply(lambda x: check_proximity(x['open_upper_indices'], x['open_lower_indices'], x['first_th0'], x['first_th1'], x['s_size_f'], x['second_th0'], x['second_th1'], x['s_size_f'] + x['s_size_s'], x['f_start']), axis=1)
nand_df.drop(['first_th0', 'first_th1', 'second_th0', 'second_th1'], axis=1, inplace=True)
df = nand_df[nand_df['proximity'] != "-1"].copy()
ddf = df.copy()
ddf['p_f'] = ddf['proximity'].apply(lambda x: x[0])
ddf['p_s'] = ddf['proximity'].apply(lambda x: x[1])
# sort p_f and p_s in the order of C M F
ddf['p_f'] = ddf['p_f'].astype('category')
ddf['p_f'].cat.reorder_categories(['F', 'M', 'C'], inplace=True)
ddf['p_s'] = ddf['p_s'].astype('category')
ddf['p_s'].cat.reorder_categories(['C', 'M', 'F'], inplace=True)
for op in ['AND', 'OR', 'NAND', 'NOR']:
    all_df = ddf[ddf['operation'] == op].groupby(['p_f', 'p_s'])['success_rate'].mean().reset_index()
    heat_all = all_df.pivot('p_f', 'p_s', 'success_rate')
    heat_all.to_json('heat_' + op + '.json')
    
sns.set(font='sans-serif', style='white')
sns.set_theme(style="ticks", rc={'xtick.bottom': True,'ytick.left': True})
colors = ["#8475c0", "#47b89c", "#c5d46b", "#d28196"]
sns.set_palette(sns.color_palette(colors))

for op in ['AND', 'NAND', 'OR', 'NOR']:
    heat_all = pd.read_json('heat_' + op + '.json')

    fig, ax = plt.subplots(figsize=(5, 4))

    sns.heatmap(heat_all, annot=True, fmt=".2f", cmap="RdYlGn", cbar=True, square=True,
                # linecolor='black', linewidths=0.5, clip_on=False
                vmin=70, vmax=100, ax=ax, annot_kws={"size": 18},
                cbar_kws={'label': 'Success Rate (%)', 'location': 'right', 'ticks': [70, 80, 90, 100]})

    ax.set_ylabel("Reference", size=22, labelpad=10)
    ax.set_xlabel("Compute", size=22, labelpad=10)
    ax.tick_params(axis='both', which='major', labelsize=20, length=0, pad=5)

    ax.set_xticklabels(["Close", "Middle", "Far"], size=20)
    ax.set_yticklabels(["Far", "Middle", "Close"], size=20)

    ax.collections[0].colorbar.set_label("Success Rate (%)", size=22, labelpad=20, rotation=270)
    ax.collections[0].colorbar.set_ticklabels([70, 80, 90, 100], fontsize=20)

    for _, spine in ax.spines.items():
        spine.set_visible(True)
    for _, spine in ax.collections[0].colorbar.ax.spines.items():
        spine.set_visible(True)

    for axis in ['top','bottom','left','right']:
        ax.spines[axis].set_linewidth(4)
        ax.spines[axis].set_color('black')


    plt.savefig(output_file + "heatmap_" + op + "_all.pdf", bbox_inches='tight')
    plt.savefig(output_file + "heatmap_" + op + "_all.png", bbox_inches='tight')



    