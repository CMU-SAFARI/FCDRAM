
import subprocess
import numpy as np
import itertools
import os
import pandas as pd
import random
import warnings
warnings.simplefilter(action='ignore', category=FutureWarning)

def b2i(binary):
    return int(binary, 2)

def write_to_file(arr,file_name):
    with open(file_name, 'w') as fp:
        for item in arr:
            # write each item on a new line
            fp.write("%s\n" % item)

def read_result_file(file_name):
    res = []
    with open(file_name, 'r') as fp:
        for item in fp.read().split(','):
            res.append(float(item))
    return res


def one_pattern_creator(one_bits, n_rows):
    zero_bits = n_rows - one_bits
    value_0 = [0 for aa in range(zero_bits)]
    value_1 = [4294967295 for aa in range(one_bits)]
    value = value_0 + value_1
    value = [str(i) for i in value]
    return value


def ref_patt_generator(operation,ref_indices,r_first,input_location):
    if operation == 'AND':
        #ref_val = 1
        ref_pattern = [b2i('1'*32) for _ in range(len(ref_indices)-1)]
        ref_pattern.append(b2i('1'*32))
    else: #means OR
        ref_pattern = [b2i('0'*32) for _ in range(len(ref_indices)-1)]
        ref_pattern.append(b2i('0'*32))
                
    return ref_pattern