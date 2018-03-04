import os
import sys

def script_dir():
    'return the path of this file'
    path = sys.path[0]
    if not os.path.isdir(path):
	path =os.path.dirname(path)
    if len(path) == 0:
	path += '.'

    return path

data_dir = os.environ['XMLDB_PATH'] + '/data'

top_dir = script_dir()
if len(top_dir) > 0:
    top_dir += '/../../../../../..'
else:
    top_dir =  '../../../../../..'

top_dir = os.path.abspath(top_dir) + '/'
top_dir.replace('\\','/')
print top_dir

exe_path = top_dir + 'dist/server/bin/testStorageEngineDLL'
print exe_path
case_args = '--run_test=t_init_func_/HEAP_MULTI_INSERT/'
arg_half = exe_path + ' ' + data_dir + ' ' + case_args
arg_case = ' -multi run'

#from items
cmd_line1 = arg_half + 'test_heap_multi_insert_frm_items_ShutDown_' + arg_case
print cmd_line1
os.system(cmd_line1)
cmd_line1_reboot = arg_half + 'test_heap_multi_insert_identify_' + arg_case
os.system(cmd_line1_reboot)

#from items with index
cmd_line1 = arg_half + 'test_heap_multi_insert_frm_items_ShutDown_index_' + arg_case
os.system(cmd_line1)
cmd_line1_reboot = arg_half + 'test_heap_multi_insert_identify_index_' + arg_case
os.system(cmd_line1_reboot)

#from entryset
cmd_line1 = arg_half + 'test_heap_multi_insert_frm_entrySet_ShutDown_' + arg_case
os.system(cmd_line1)
cmd_line1_reboot = arg_half + 'test_heap_multi_insert_identify_' + arg_case
os.system(cmd_line1_reboot)

#from entryset with index
cmd_line1 = arg_half + 'test_heap_multi_insert_frm_entrySet_ShutDown_index_' + arg_case
os.system(cmd_line1)
cmd_line1_reboot = arg_half + 'test_heap_multi_insert_identify_index_' + arg_case
os.system(cmd_line1_reboot)


File_Entry = data_dir + '/multi_insert.conf'
File_Data = data_dir + '/insert_data'
if(os.path.exists(File_Entry)):
    os.remove(File_Entry)

if(os.path.exists(File_Data)):
    os.remove(File_Data)
