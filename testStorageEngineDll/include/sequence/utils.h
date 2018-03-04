#ifndef SE_UTILS_HPP
#define SE_UTILS_HPP

#include "PGSETypes.h"
#include "EntrySet.h"
#include "StorageEngine.h"

using namespace FounderXDB::StorageEngineNS;

extern ColumnInfo heap_colinfo;
extern ColumnInfo index_colinfo;


extern void form_heap_colinfo(ColumnInfo &colinfo);
extern void form_heap_colinfo_chars(ColumnInfo &colinfo);

extern void form_heap_colinfo_two(ColumnInfo &colinfo);
extern void form_index_colinfo_two(ColumnInfo &colinfo);

extern void form_index_colinfo(ColumnInfo &colinfo);
extern void form_index_colinfo_chars(ColumnInfo &colinfo);
extern void free_colinfo(ColumnInfo &colinfo);

extern int wfh_compare(const char *str1, size_t len1, const char *str2, size_t len2);
extern int wfh_compare_chars(const char *str1, size_t len1, const char *str2, size_t len2);

extern bool testseq_SeqFactoryMethod();
extern bool testseq_SeqMethod();
extern bool testseq_CreateSeqConcurrency();
extern bool testseq_GetValueConcurrency();

#endif //SE_UTILS_HPP