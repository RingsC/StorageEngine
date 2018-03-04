import os

data_dir = os.environ['XMLDB_PATH'] + '/data '
exe_path = '../../../../../dist/server/bin/testPerform.exe '
exe_args = '--run_test=t_init_func_/PERFORM_COMPARE/test_perform_sort_index_prepare_ -test_count 20000000 -test_len 32'
cmd_line = exe_path + data_dir + exe_args

print cmd_line

os.system(cmd_line)
