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

not_df.drop(columns=['subarray_size_f', 'subarray_size_s', 'wr_p_f', 's_start', 'row_addr_f', 'row_addr_s'], inplace=True)
not_df.rename(columns={'n_wr_p_s': 'not_reliability'}, inplace=True)

not_df['not_reliability'] = not_df['not_reliability'].astype(float)
not_df['not_reliability'] = not_df['not_reliability']*200
not_df['not_reliability'] = not_df['not_reliability'].apply(lambda x: 100 if x > 100 else x)
not_df.sort_values(by=['n_rows_total'], inplace=True)

df_nn = not_df[not_df['n_rows_total'].isin([2, 4, 8, 16, 32])].copy()
df_n2n = not_df[not_df['n_rows_total'].isin([3, 6, 12, 24, 48])].copy()


sns.set(font='sans-serif', style='white')
sns.set_theme(style="ticks", rc={'xtick.bottom': True,'ytick.left': True})
colors = ["#8475c0", "#47b89c", "#c5d46b", "#d28196"]
sns.set_palette(sns.color_palette(colors))

fig, ax = plt.subplots(1, 1, figsize=(8, 3))

sns.boxplot(x='n_rows_s', y='not_reliability', data=not_df, ax=ax, 
            capprops={"linewidth":1.5, "color":"black"},
            boxprops={"linewidth":1.5, "edgecolor":"black"},
            whiskerprops={"linewidth":1.5, "color":"black"},
            medianprops={"linewidth":1.5, "color":"black"},
            showmeans=True, meanprops={"marker":"o","markerfacecolor":"white", "markeredgecolor":"black", "markersize":"10"})

ax.set_ylabel("Success Rate (%)", size=21)
ax.set_xlabel("Number of Destination Rows", size=25)
ax.tick_params(axis='both', which='major', labelsize=22)
ax.tick_params(axis='x', which='major', length=0, pad=5)

ax.set_yticks([0, 12.5, 25, 37.5, 50, 62.5, 75, 87.5, 100], [0, "", 25, "", 50, "", 75, "", 100])

ax.set_ylim([-5, 105])
ax.set_title("", )

for axis in ['top','bottom','left','right']:
    ax.spines[axis].set_linewidth(4)
    ax.spines[axis].set_color('black')

ax.grid(True, axis="y", alpha=0.5)


plt.savefig(output_path + "not_vs_dest_rows.pdf", dpi=300, bbox_inches='tight')
plt.savefig(output_path + "not_vs_dest_rows.png", dpi=300, bbox_inches='tight')

sns.set(font='sans-serif', style='white')
sns.set_theme(style="ticks", rc={'xtick.bottom': True,'ytick.left': True})
colors = ["#8475c0", "#47b89c", "#c5d46b", "#d28196", "#8475c0"]
sns.set_palette(sns.color_palette(colors))


fig, axs = plt.subplots(1, 2, figsize=(12, 4), sharey=True)

if not df_nn.empty:
    sns.boxplot(x='n_rows', y='not_reliability', data=df_nn, ax=axs[0],
                capprops={"linewidth":1.5, "color":"black"},
                boxprops={"linewidth":1.5, "edgecolor":"black"},
                whiskerprops={"linewidth":1.5, "color":"black"},
                medianprops={"linewidth":1.5, "color":"black"},
                palette=colors[:5],
                showmeans=True, meanprops={"marker":"o","markerfacecolor":"white", "markeredgecolor":"black", "markersize":"10"})
if not df_n2n.empty:
    sns.boxplot(x='n_rows', y='not_reliability', data=df_n2n, ax=axs[1], 
                capprops={"linewidth":1.5, "color":"black"},
                boxprops={"linewidth":1.5, "edgecolor":"black"},
                whiskerprops={"linewidth":1.5, "color":"black"},
                medianprops={"linewidth":1.5, "color":"black"},
                palette=colors[1:],
                showmeans=True, meanprops={"marker":"o","markerfacecolor":"white", "markeredgecolor":"black", "markersize":"10"})


for ax in axs.flat:
    ax.set(xlabel='', ylabel='')
    ax.set_ylim([-5, 105])
    ax.set_ylabel("", size=25)
    ax.set_xlabel("", size=25)
    ax.tick_params(axis='y', which='both', length=0)
    ax.tick_params(axis='x', which='major', length=0, pad=5)
    for axis in ['top','bottom','left','right']:
        ax.spines[axis].set_linewidth(4)
        ax.spines[axis].set_color('black')


axs[0].set_ylabel("Success Rate (%)", size=24)
axs[0].set_xlabel(r'$N_{RF} = N_{RL}$', size=24)
axs[1].set_xlabel(r'$2\times N_{RF} = N_{RL}$', size=24)
fig.text(0.5, -0.15, r'$N_{RF} : N_{RL}$', ha='center', size=25)

axs[0].set_yticks([0, 12.5, 25, 37.5, 50, 62.5, 75, 87.5, 100], [0, "", 25, "", 50, "", 75, "", 100], size=22)
axs[0].set_xticklabels(["1:1", "2:2", "4:4", "8:8", "16:16"], fontsize=22)
axs[1].set_xticklabels(["1:2", "2:4", "4:8", "8:16", "16:32"], fontsize=22)
axs[0].tick_params(axis='y', which='both', length=5)
axs[0].grid(True, axis="y", alpha=0.5)
axs[1].grid(True, axis="y", alpha=0.5)

fig.subplots_adjust(wspace=0.0, hspace=0.0)

plt.savefig(output_path + "not_n_rows.png", dpi=300, bbox_inches='tight')
plt.savefig(output_path + "not_n_rows.pdf", dpi=300, bbox_inches='tight')