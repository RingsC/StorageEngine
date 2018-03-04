import os
import sys

def getCurFileDir():
    path = sys.path[0]
    if not os.path.isdir(path):
        path = os.path.dirname(path)
    if len(path) == 0:
        path += '.'
    return path

xmldb_path = os.environ['XMLDB_PATH']
print xmldb_path

xmldb_data = xmldb_path + '/data'
print xmldb_data

cur_file_path = getCurFileDir()
trunk_dir = ''
if len(cur_file_path) > 0:
    trunk_dir = cur_file_path + '/../../../../..'
else:
    trunk_dir = cur_file_path + '../../../../../..'

trunk_dir = os.path.abspath(trunk_dir) + '/'
trunk_dir.replace('\\','/')
print trunk_dir

exe_path = trunk_dir + 'dist/server/bin/testTwoPhaseCommit'
print exe_path

case_args = '--run_test=t_init_func_/'
args_half = exe_path + ' ' + xmldb_data + ' ' + case_args
args_case = ' -twophase run'

case_list = [
    'test_twophase_heap_insert_prepare_',
    'test_twophase_heap_insert_commit_',
    'test_twophase_heap_insert_prepare_',
    'test_twophase_heap_insert_rollback_',
    'test_twophase_heap_insert_prepare_index_',
    'test_twophase_heap_insert_commit_index_',
    
    'test_twophase_heap_delete_prepare_',
    'test_twophase_heap_delete_commit_',
    'test_twophase_heap_delete_prepare_',
    'test_twophase_heap_delete_rollback_',
    'test_twophase_heap_delete_prepare_index_',
    'test_twophase_heap_delete_commit_index_',
    
    'test_twophase_heap_update_prepare_',
    'test_twophase_heap_update_commit_',
    'test_twophase_heap_update_prepare_',
    'test_twophase_heap_update_rollback_',
    'test_twophase_heap_update_prepare_index_',
    'test_twophase_heap_update_commit_index_',
    
    'test_twophase_heap_create_prepare_',
    'test_twophase_heap_create_rollback_'
    ]

for case in case_list:
    cmd_line = args_half + case + args_case
    os.system(cmd_line)
