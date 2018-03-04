#include <stdlib.h>
#include <string.h>

#include "sequence/utils.h"
#include "utils/utils_dll.h"




//int wfh_compare(char *str1, int len1, char *str2, int len2);
void heap_split_char(RangeDatai& rangeData, const char*, int i, size_t len = 0);
void heap_split(RangeDatai& rangeData,const char*, int i, size_t len = 0);
void index_split(RangeDatai& rangeData,const char*, int i, size_t len = 0);


ColumnInfo heap_colinfo(false);
ColumnInfo index_colinfo(false);

void form_heap_colinfo_chars(ColumnInfo &colinfo)
{
    colinfo.keys = 1;
    colinfo.col_number = NULL;
    colinfo.split_function = heap_split_char;
    colinfo.rd_comfunction = (CompareCallbacki*)malloc(sizeof(CompareCallbacki));
    colinfo.rd_comfunction[0] = wfh_compare_chars;
}


void form_heap_colinfo_two(ColumnInfo &colinfo)
{
    colinfo.keys = 1;
    colinfo.col_number = NULL;
    colinfo.split_function = index_split;
    colinfo.rd_comfunction = (CompareCallbacki*)malloc(sizeof(CompareCallbacki));
    colinfo.rd_comfunction[0] = wfh_compare;
}

void form_heap_colinfo(ColumnInfo &colinfo)
{
    colinfo.keys = 1;
    colinfo.col_number = NULL;
    colinfo.split_function = heap_split;
    colinfo.rd_comfunction = (CompareCallbacki*)malloc(sizeof(CompareCallbacki));
    colinfo.rd_comfunction[0] = wfh_compare;
}

void form_index_colinfo_chars(ColumnInfo &colinfo)
{
    colinfo.keys = 1;
    size_t *p = (size_t*)malloc(sizeof(size_t));
    *p = 1;
    colinfo.col_number = p;
    colinfo.split_function = heap_split_char;
    colinfo.rd_comfunction = (CompareCallbacki*)malloc(sizeof(CompareCallbacki));
    colinfo.rd_comfunction[0] = wfh_compare_chars;
}

void form_index_colinfo_two(ColumnInfo &colinfo)
{
    colinfo.keys = 2;
    size_t *p = (size_t*)malloc(sizeof(size_t) * 2);
    *p = 1;
	*(p + 1) = 2;
    colinfo.col_number = p;
    colinfo.split_function = index_split;
    colinfo.rd_comfunction = (CompareCallbacki*)malloc(sizeof(CompareCallbacki) * 2);
    colinfo.rd_comfunction[0] = wfh_compare;
	colinfo.rd_comfunction[1] = wfh_compare;
}

void form_index_colinfo(ColumnInfo &colinfo)
{
    colinfo.keys = 1;
    size_t *p = (size_t*)malloc(sizeof(size_t));
    *p = 1;
    colinfo.col_number = p;
    colinfo.split_function = index_split;
    colinfo.rd_comfunction = (CompareCallbacki*)malloc(sizeof(CompareCallbacki));
    colinfo.rd_comfunction[0] = wfh_compare;
}

void free_colinfo(ColumnInfo &colinfo)
{
    if(colinfo.rd_comfunction) {
        free(colinfo.rd_comfunction);
        colinfo.rd_comfunction = NULL;
    }
}

int wfh_compare_chars(const char *str1, size_t len1, const char *str2, size_t len2)
{
    int len = len1 > len2 ? len2 : len1;
    return memcmp(str1, str2, len);
}

int wfh_compare(const char *str1, size_t len1, const char *str2, size_t len2)
{
    int a = *(int *)(str1);
    int b = *(int *)(str2);
    if (a > b ) return 1;
    if (a == b ) return 0;
    if(a < b) return -1;
}

void heap_split_char(RangeDatai& rang, const char*, int i, size_t len)
{
    switch(i) 
    { 
    case 1:{
                rang.start = 0;
              //  rang.len = (DATA_LEN * strlen(DATA) + INT_MAX);
                rang.len = (DATA_LEN * strlen(DATA) + 20);
              //  rang.len = (DATA_LEN * strlen(DATA) + INT_MAX);
                rang.len = len;//(DATA_LEN * strlen(DATA) + 20);
                break;
           }

    default :{
                return ;
             }

    }
}


void heap_split(RangeDatai& rang, const char*, int i, size_t len)
{
    switch(i) 
    { 
    case 1:{
                rang.start = 0;
                rang.len = 4;
                break;
           }
    case 2: {
                rang.start = 4;
                rang.len = 8;
                break;
            }
    case 3: {
                rang.start = 12;
                rang.len = 4;
                break;
            }
    case 4: {
                rang.start = 16;
                rang.len = 8;
                break;
            }
    default :{
                return ;
             }

    }
}

void index_split(RangeDatai& rang, const char*, int i , size_t len)
{
    switch(i) 
    { 
    case 1:{
                rang.start = 0;
                rang.len = 4;
                break;
           }
	case 2: {
                rang.start = 4;
                rang.len = 4;
                break;			
			}
   
    default :{
                return ;
             }

    }
}
