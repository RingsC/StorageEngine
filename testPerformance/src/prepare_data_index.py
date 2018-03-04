import os
import subprocess
import sys
import shutil
import time
import Run_Cases_Funcs as my_func

#init xmldb data dir every execute
xmldb_env_path = os.environ['XMLDB_PATH']
print xmldb_env_path
data_dir = xmldb_env_path + '/data'
data_cpy_dir = xmldb_env_path + '/data_copy'

#init xmldb data dir every execute
def ClearDataDir():
    print 'data_dir : ' + data_dir
    if(os.path.exists(data_dir)):
        shutil.rmtree(data_dir)
    assert(os.path.exists(data_dir) == False)

    assert(os.path.exists(data_cpy_dir))
    if(os.path.exists(data_cpy_dir)):
        shutil.copytree(data_cpy_dir,data_dir)
    assert(os.path.exists(data_dir))

#data count
data_count = sys.argv[1]
tuple_len = sys.argv[2]

trunk_bin_path = '../../../../../dist/server/bin/testPerform'
args_prefix = '--run_test=t_init_func_/PERFORM_COMPARE/'

trunk_arg = trunk_bin_path + ' ' + data_dir + ' ' + args_prefix

#execute case trunk
case_list = [
            'test_perform_heap_prepare_data_index_',
            ]

for case in case_list:
    cmd_line = trunk_arg + case + ' -test_count ' + data_count + ' -test_len ' + tuple_len
    print cmd_line
    ClearDataDir()
    os.system(cmd_line)

#stat work
my_func.Write2File(data_dir + '/../trunk_test',data_dir + '/../stat_log')


