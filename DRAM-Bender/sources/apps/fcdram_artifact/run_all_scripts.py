import subprocess
import os,sys


### specify the DRAM-Bender's apps full path
## example = '/home/<user>/FCDRAM/DRAM-Bender/sources/apps/'
apps_path = '/home/ismail/FCDRAM/DRAM-Bender/sources/apps/'


## module name -- command line argument (e.g., python3 run_all_scripts.py my_module0)
module = sys.argv[1]


experiment_lst = ['RowClone','Not', 'NotReliability', 'NandNorReliability']


## if you have temperature make the temp = 1
temp = 0

### specify the result's full path
## example = '/home/<user>/FCDRAM/analysis/experimental_data/'
result_path = f'/home/ismail/FCDRAM/experimental_data/{module}/'

### specify the post-processing scripts' full path
## example = '/home/<user>/FCDRAM/analysis/'
scripts_path = '/home/ismail/FCDRAM/analysis/'


if not os.path.exists(result_path):
    os.makedirs(result_path)

if temp == 1:
    temperature_lst = [50,60,70,80]
else:
    temperature_lst = [50]



for experiment in experiment_lst:
    if experiment == 'NotReliability' or experiment == 'NandNorReliability':
        if experiment == 'NotReliability':
            which_half_analysis =f'cd {scripts_path} && python3 process_which_half.py {module}'
            sp = subprocess.run([which_half_analysis], shell=True, check=True)
        for temperature in temperature_lst:
            cmd = f'cd {apps_path}{experiment} && make && python3 run_script.py --module {module} --temperature {temperature} --apps_path {apps_path} --result_path {result_path}'
            print(cmd)
            try:
                print(f'Experiment {experiment} at {temperature}C is ongoing')
                os.system(f'{apps_path}ResetBoard/full_reset.sh')
                sp = subprocess.run([cmd], shell=True, check=True)
            except:
                print('SOMETHING IS WRONG')
                print(f'Experiment {experiment} at {temperature}C failed')
                exit(0)
    ## Reverse Engineering
    else:
        cmd = f'cd {apps_path}{experiment} && make && python3 run_script.py --module {module} --apps_path {apps_path} --result_path {result_path}'
        try:
            print(f'Experiment {experiment} at {temperature}C is ongoing')
            os.system(f'{apps_path}ResetBoard/full_reset.sh')
            sp = subprocess.run([cmd], shell=True, check=True)
        except:
            print('SOMETHING IS WRONG')
            print(f'Experiment {experiment} at {temperature}C failed')
            exit(0)
   
    print(f'Experiment {experiment} at {temperature}C is done')

print('#'*15)
print('All experiments are done')
print('#'*15)