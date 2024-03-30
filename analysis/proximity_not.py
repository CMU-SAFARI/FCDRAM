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

## filtered_not.csv data path
filtered_not_path = data_path + module + '/'

# your plots path
# example: output_path = "/home/<user>/FCDRAM/analysis/plots/"
output_path = "/home/ismail/FCDRAM/analysis/plots/"

if not os.path.exists(output_path):
    os.makedirs(output_path)

not_df = pd.DataFrame()
for module in modules:
    filename = filtered_not_path + "filtered_not.csv"
    if not os.path.exists(filename):
        print("Dir does not exist " + filename)
        exit()

    t_df = pd.read_csv(filename)
    if module.startswith("hytg"): ## example module codename (that has only N:N Activation Type)
        t_df = t_df[t_df['n_rows_f'] == t_df['n_rows_s']].copy()
    elif module.startswith("hytt") or module.startswith("hyhy"): ## example module codename (that has N:N and N:2N Activation Type)
        t_df = t_df[(t_df['n_rows_f'] == t_df['n_rows_s']) | (t_df['n_rows_f'] * 2 == t_df['n_rows_s'])].copy()
    t_df['module'] = module
    not_df = pd.concat([not_df, t_df])

not_df['n_rows_total'] = not_df['n_rows_f'] + not_df['n_rows_s']

not_df.rename(columns={'n_wr_p_s': 'not_reliability'}, inplace=True)

not_df['not_reliability'] = not_df['not_reliability'].astype(float)
not_df['not_reliability'] = not_df['not_reliability']*200
not_df['not_reliability'] = not_df['not_reliability'].apply(lambda x: 100 if x > 100 else x)
not_df.sort_values(by=['n_rows_total'], inplace=True)

not_df['first_th0'] = not_df['subarray_size_f']/3
not_df['first_th1'] = not_df['subarray_size_f']*2/3
not_df['second_th0'] = not_df['subarray_size_f'] + not_df['subarray_size_s']/3
not_df['second_th1'] = not_df['subarray_size_f'] + not_df['subarray_size_s']*2/3

def check_proximity(row_list_f, row_list_s, first_th0, first_th1, first_th2, second_th0, second_th1, second_th2):
    proximity_f = ""
    proximity_s = ""
    row_list_f = row_list_f[1:-1].split(",")
    row_list_s = row_list_s[1:-1].split(",")
    proximity_list = []
    for row in row_list_f:
        row = int(row)
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
        row = int(row)
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

not_df['proximity'] = not_df.apply(lambda x: check_proximity(x['row_addr_f'], x['row_addr_s'], x['first_th0'], x['first_th1'], x['subarray_size_f'], x['second_th0'], x['second_th1'], x['subarray_size_f'] + x['subarray_size_s']), axis=1)

not_df.drop(['first_th0', 'first_th1', 'second_th0', 'second_th1'], axis=1, inplace=True)
df = not_df[not_df['proximity'] != "-1"].copy()
ddf = df.copy()
ddf['p_f'] = ddf['proximity'].apply(lambda x: x[0])
ddf['p_s'] = ddf['proximity'].apply(lambda x: x[1])
# sort p_f and p_s in the order of C M F
ddf['p_f'] = ddf['p_f'].astype('category')
ddf['p_f'].cat.reorder_categories(['F', 'M', 'C'], inplace=True)
ddf['p_s'] = ddf['p_s'].astype('category')
ddf['p_s'].cat.reorder_categories(['C', 'M', 'F'], inplace=True)

all_df = ddf.groupby(['p_f', 'p_s'])['not_reliability'].mean().reset_index()
heat_all = all_df.pivot('p_f', 'p_s', 'not_reliability')
heat_all.to_json('heat_not.json')

heat_all = pd.read_json('heat_not.json')

fig, ax = plt.subplots(figsize=(6, 4))

sns.heatmap(heat_all, annot=True, fmt=".2f", cmap='RdYlGn', cbar=True, square=False,
            # linecolor='black', linewidths=0.5, clip_on=False
            vmin=40, vmax=100, ax=ax, annot_kws={"size": 18},
            cbar_kws={'label': 'Success Rate (%)', 'location': 'right', 'ticks': [40, 60, 80, 100]})

ax.set_ylabel("Source", size=25, labelpad=10)
ax.set_xlabel("Destination", size=25, labelpad=10)
ax.tick_params(axis='both', which='major', labelsize=20, length=0, pad=5)

ax.set_xticklabels(["Close", "Middle", "Far"], size=20)
ax.set_yticklabels(["Far", "Middle", "Close"], size=20)

ax.collections[0].colorbar.set_label("Success Rate (%)", size=25, labelpad=20, rotation=270)

ax.collections[0].colorbar.set_ticklabels([40, 60, 80, 100], fontsize=18)

ax.set_title("")

for _, spine in ax.spines.items():
    spine.set_visible(True)
for _, spine in ax.collections[0].colorbar.ax.spines.items():
    spine.set_visible(True)

for axis in ['top','bottom','left','right']:
    ax.spines[axis].set_linewidth(4)
    ax.spines[axis].set_color('black')

plt.tight_layout()

plt.savefig(output_path + "heatmap_not_all.png", dpi=300, bbox_inches='tight')
plt.savefig(output_path + "heatmap_not_all.pdf", dpi=300, bbox_inches='tight')

