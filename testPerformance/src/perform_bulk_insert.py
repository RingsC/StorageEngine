import os
import sys
import Run_Cases_Funcs as my_func

xmldb_env_path = os.environ['XMLDB_PATH']
print xmldb_env_path
data_dir = xmldb_env_path + '/data'

data_count = sys.argv[1]
tuple_len = sys.argv[2]
run_times = int(sys.argv[3])

def WriteSizeToFile(Ring_Size):
    File_Name = data_dir + '/RingSize.conf'
    print File_Name
    File = open(File_Name,'w')
    assert(File)
    File.write(Ring_Size)
    File.close()

def FileReName(src,dist):
    if(os.path.exists(dist)):
        os.remove(dist)
    if(os.path.exists(src)):
        os.rename(src,dist)
        
trunk_bin_path = '../../../../../dist/server/bin/testPerform'
args_prefix = '--run_test=t_init_func_/PERFORM_COMPARE/'

trunk_arg = trunk_bin_path + ' ' + data_dir + ' ' + args_prefix

#execute case trunk
case_list = [
    'test_perform_normal_insert_',
    #'test_perform_normal_insert_index_',
    'test_perform_bulk_insert_',
    #'test_perform_bulk_insert_index_'
        ]

file_stat = data_dir + '/../trunk_test'
file_sum = data_dir + '/../stat_log'
if(os.path.exists(file_stat)):
    os.remove(file_stat)

if(os.path.exists(file_sum)):
    os.remove(file_sum)
    
for case in case_list:
    cmd_line = trunk_arg + case + ' -test_count ' + data_count + ' -test_len ' + tuple_len
        
    for time in range(0,run_times):
        os.system(cmd_line)
            
my_func.Write2File(file_stat,file_sum)



##################
trunk_bin_path = '../../../../../dist/server/bin/testPerform'

trunk_arg = trunk_bin_path + ' ' + data_dir + ' ' + args_prefix

#execute case trunk
case_list = [
    'test_perform_bulk_insert_',
    #'test_perform_bulk_insert_index_'
        ]

file_stat = data_dir + '/../trunk_test_bulk'
file_sum = data_dir + '/../stat_log_bulk'
if(os.path.exists(file_stat)):
    os.remove(file_stat)

if(os.path.exists(file_sum)):
    os.remove(file_sum)
    
for case in case_list:
    cmd_line = trunk_arg + case + ' -test_count ' + data_count + ' -test_len ' + tuple_len
        
    for time in range(0,run_times):
        os.system(cmd_line)
            
my_func.Write2File(file_stat,file_sum)

