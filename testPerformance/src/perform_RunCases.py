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
    
    if(os.path.exists(data_cpy_dir)):
        shutil.copytree(data_cpy_dir,data_dir)
    assert(os.path.exists(data_dir))
def BackupStatHistory(filename):
    file_old = xmldb_env_path + filename
    print file_old
    str_cur_time = ''
    str_cur_time = filename + time.strftime('%Y-%m-%d_%H-%M-%S',time.localtime(time.time()))
    file_new = xmldb_env_path + '/' + str_cur_time
    print file_new
    if(os.path.exists(file_old)):
        #shutil.copy(file_old,file_new)
        os.rename(file_old,file_new)

#data count
data_count = sys.argv[1]
tuple_len = sys.argv[2]
run_times = sys.argv[3]

trunk_bin_path = '../../../../../dist/server/bin/testPerform'
args_prefix = '--run_test=t_init_func_/PERFORM_COMPARE/'

trunk_arg = trunk_bin_path + ' ' + data_dir + ' ' + args_prefix

#execute case trunk
case_list = [
            #'test_perform_heap_insert_',
            #'test_perform_heap_insert_index_',
            #'test_perform_heap_insert_concurrent_',
            #'test_perform_heap_insert_concurrent_index_',
            #'test_perform_heap_traverse_',
            #'test_perform_heap_traverse_index_',
            #'test_perform_heap_traverse_concurrent_',
            #'test_perform_heap_traverse_concurrent_index_',
            #'test_perform_heap_delete_',
            #'test_perform_heap_delete_index_',
            #'test_perform_heap_delete_concurrent_',
            #'test_perform_heap_delete_concurrent_index_',
            #'test_perform_heap_update_',
            #'test_perform_heap_update_index_',
            #'test_perform_heap_update_concurrent_',
            #'test_perform_heap_update_concurrent_index_',
            'test_perform_heap_insert_trans_',
            'test_perform_heap_insert_trans_index_'
        ]
#backup formal history file,add current time at filename end
BackupStatHistory('./trunk_test')
BackupStatHistory('./stat_log')

times = int(run_times)
for i in range(0,times):
    for case in case_list:
        cmd_line = trunk_arg + case + ' -test_count ' + data_count + ' -test_len ' + tuple_len
        print cmd_line
        #ClearDataDir()
        os.system(cmd_line)

#stat work
my_func.Write2File(data_dir + '/../trunk_test',data_dir + '/../stat_log')


