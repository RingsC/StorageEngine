import os
import time

def GetCurTimeStr():
    return time.strftime('%Y-%m-%d %H:%M:%S',time.localtime(time.time()))

def GetAverage(file_path):
    my_dict = {}
    my_list = []
    list_key = []
    
    my_file = open(file_path,'r')

    assert(my_file)

    #traverse file to get each line string ,use a dictionary to storage a line string, add to a list
    for eachline in my_file:
        line_str = eachline.strip('\n')
        
        str_list = line_str.split(' ')
        
        str_key = str_list[0]
        #insert key to list only once
        insert_flag = 0
        for key in list_key:
            if(key == str_key):
                insert_flag += 1
        assert(1 == insert_flag or 0 == insert_flag)
        if(0 == insert_flag):
            list_key.append(str_key)
            
        str_value = str_list[1]
        
        my_dict[str_key] = str_value
        dict = {str_key:str_value}
        
        my_list.append(dict)

    #traverse list to get average value, return a dict
    #print my_dict
    for key in my_dict:
        #print key
        total_value = 0.0
        flag = 0
        for dict in my_list:
            if(key in dict):
                total_value += float(dict[key])
                flag += 1
        #print flag
        value = float(total_value/flag)
        my_dict[key] = '%.6f' % value

    list_ret = []
    list_ret.append(list_key)
    list_ret.append(my_dict)
    return list_ret

def GetWriteString(trunk_path):
    list_trunk = GetAverage(trunk_path)
    #print list_trunk
    list_key = list_trunk[0]
    dict = list_trunk[1]
    line_len = 30
    str_write = 'TestType                     Time(seconds)\n'

    for key in list_key:
        #print (key,dict[key])
        blank_space = ''
        for i in range(0,line_len - len(key)):
            blank_space += ' '
            
        str_line = key + blank_space + dict[key]
        str_write += str_line + '\n\n'

    return str_write

def Write2File(trunk_path, write_path):
    #if(os.path.exists(write_path)):
        #os.remove(write_path)

    str_write = GetWriteString(trunk_path)
    str_time = 'stat date and time : ' + GetCurTimeStr()
    str_write += str_time
    
    my_file = open(write_path,'w')
    my_file.write(str_write)
    my_file.close()
