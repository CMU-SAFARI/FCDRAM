
import subprocess
import numpy as np
import itertools
from smc_scripts import SoftMC_Infra
import os
import pandas as pd
import warnings
warnings.simplefilter(action='ignore', category=FutureWarning)


def check_subarray_rows(subarrays, random_subarray_pairs):
    for i in range(len(random_subarray_pairs)):
        if len(subarrays[random_subarray_pairs[i]]) < 500:
            return 0
    return 1


def create_random_subarray_csv(s_df, random_subarray_pairs, csv_name):
    s_df.iloc[[random_subarray_pairs[0]]].to_csv(csv_name)
    for s_id in random_subarray_pairs:
        if s_id == random_subarray_pairs[0]:
            continue
        s_df.iloc[[s_id]].to_csv(csv_name, mode='a', header=False)

def create_subarray_list(df):
    subarrays = []
    for i in range(len(df)):
        a = df.iloc[[i]]['rows'][i]
        a = a[1:-1].split(',')
        subarrays.append([int(ii) for ii in a])
    return subarrays




