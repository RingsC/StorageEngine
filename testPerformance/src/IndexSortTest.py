import os

data_dir = os.environ['XMLDB_PATH'] + '/data '

conf_file_sort = data_dir + '../SortIndex'
if(os.path.exists(conf_file_sort)):
    os.remove(conf_file_sort)

conf_file_unsort = data_dir + '../UnSortIndex'
if(os.path.exists(conf_file_unsort)):
    os.remove(conf_file_unsort)

times = 4
exe_args = '--run_test=t_init_func_/PERFORM_COMPARE/test_perform_multi_insert_index_1_ -test_count 20000 -test_len %d' % times

### index sort
exe_path = '../../../../../dist/server/bin/testPerform.exe '
cmd_line_sort = exe_path + data_dir + exe_args

### index unsort
exe_path_unsort = '../../../../../dist/server/bin/testPerform.exe '
cmd_line_unsort = exe_path_unsort + data_dir + exe_args

#for i in range(0,times + 1):
 #   os.system(cmd_line_sort)
 #   os.system(cmd_line_unsort)

query_args = '--run_test=t_init_func_/PERFORM_COMPARE/test_perform_sort_index_query_'

cmd_sort_query = exe_path + data_dir + query_args
os.system(cmd_sort_query)

cmd_unsort_query = exe_path_unsort + data_dir + query_args
#os.system(cmd_unsort_query)

bitmap_args = '--run_test=t_init_func_/PERFORM_COMPARE/test_perform_bitmap_scan_'

cmd_sort_bitmap = exe_path + data_dir + bitmap_args
#os.system(cmd_sort_bitmap)

cmd_unsort_bitmap = exe_path_unsort + data_dir + bitmap_args
#os.system(cmd_unsort_bitmap)
