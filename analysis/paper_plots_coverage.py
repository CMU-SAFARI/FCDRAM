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

not_df.drop(columns=['s_start', 'row_addr_f', 'row_addr_s'], inplace=True)
not_df.rename(columns={'wr_p_f': 'success_first'}, inplace=True)
not_df.rename(columns={'n_wr_p_s': 'success_second'}, inplace=True)

not_df['success_first'] = not_df['success_first'].astype(float)
not_df['success_first'] = not_df['success_first']*100
not_df['success_first'] = not_df['success_first'].apply(lambda x: 100 if x > 100 else x)

not_df['success_second'] = not_df['success_second'].astype(float)
not_df['success_second'] = not_df['success_second']*200
not_df['success_second'] = not_df['success_second'].apply(lambda x: 100 if x > 100 else x)
not_df.sort_values(by=['n_rows_total'], inplace=True)


not_df['total_num_pairs'] = not_df['subarray_size_s'] * not_df['subarray_size_f']
not_df.drop(columns=['success_first', 'success_second', 'n_rows_s', 'n_rows_f', 'r_first', 'r_second', 'subarray_size_s', 'subarray_size_f'], inplace=True)
not_df['dummy'] = 1
not_df['num_pairs'] = not_df.groupby(['module', 'bank_id', 'f_start', 'n_rows_total', 'total_num_pairs', 'n_rows'])['dummy'].transform('count')
not_df.drop(columns=['dummy'], inplace=True)
not_df.drop_duplicates(inplace=True)

not_df['fraction_of_pairs'] = not_df['num_pairs'] / not_df['total_num_pairs'] * 100

df_nn = not_df[not_df['n_rows_total'].isin([2, 4, 8, 16, 32])].copy()
df_n2n = not_df[not_df['n_rows_total'].isin([3, 6, 12, 24, 48])].copy()

for n_rows in not_df['n_rows_total'].unique():
    avg_coverage = not_df[not_df['n_rows_total'] == n_rows]['fraction_of_pairs'].mean()
    print("Avg coverage for " + str(n_rows) + " rows : " + str(avg_coverage))

sns.set(font='sans-serif', style='white')
sns.set_theme(style="ticks", rc={'xtick.bottom': True,'ytick.left': True})
colors = ["#8475c0", "#47b89c", "#c5d46b", "#d28196"]
sns.set_palette(sns.color_palette(colors))

fig, axs = plt.subplots(1, 2, figsize=(12, 4), sharey=True)

if not df_nn.empty:
    sns.boxplot(x='n_rows', y='fraction_of_pairs', data=df_nn, ax=axs[0],
                capprops={"linewidth":1.5, "color":"black"},
                boxprops={"linewidth":1.5, "edgecolor":"black"},
                whiskerprops={"linewidth":1.5, "color":"black"},
                medianprops={"linewidth":1.5, "color":"black"},
                showmeans=True, meanprops={"marker":"o","markerfacecolor":"white", "markeredgecolor":"black", "markersize":"10"})
if not df_n2n.empty:
    sns.boxplot(x='n_rows', y='fraction_of_pairs', data=df_n2n, ax=axs[1], 
                capprops={"linewidth":1.5, "color":"black"},
                boxprops={"linewidth":1.5, "edgecolor":"black"},
                whiskerprops={"linewidth":1.5, "color":"black"},
                medianprops={"linewidth":1.5, "color":"black"},
                showmeans=True, meanprops={"marker":"o","markerfacecolor":"white", "markeredgecolor":"black", "markersize":"10"})


for ax in axs.flat:
    ax.set(xlabel='', ylabel='')
    ax.set_ylim([-1, 41])
    ax.set_ylabel("", size=25)
    ax.set_xlabel("", size=25)
    ax.tick_params(axis='y', which='both', length=0)
    ax.tick_params(axis='x', which='major', length=0, pad=5)
    for axis in ['top','bottom','left','right']:
        ax.spines[axis].set_linewidth(4)
        ax.spines[axis].set_color('black')


axs[0].set_ylabel("Coverage (%)", size=24)
# fig.text(0.5, -0.05, r'$N_{RF} : N_{RL}$', ha='center', size=25)
axs[0].set_xlabel(r'$N_{RF} = N_{RL}$', size=24)
axs[1].set_xlabel(r'$2\times N_{RF} = N_{RL}$', size=24)
fig.text(0.5, -0.15, r'$N_{RF} : N_{RL}$', ha='center', size=25)

axs[0].set_yticks([0, 10, 20, 30, 40], [0, 10, 20, 30, 40], size=22)
axs[0].set_xticklabels(["1:1", "2:2", "4:4", "8:8", "16:16"], fontsize=22)
axs[1].set_xticklabels(["1:2", "2:4", "4:8", "8:16", "16:32"], fontsize=22)
axs[0].tick_params(axis='y', which='both', length=5)
axs[0].grid(True, axis="y", alpha=0.5)
axs[1].grid(True, axis="y", alpha=0.5)

fig.subplots_adjust(wspace=0.0, hspace=0.0)

plt.savefig(output_path + "coverage.png", dpi=300, bbox_inches='tight')
plt.savefig(output_path + "coverage.pdf", dpi=300, bbox_inches='tight')