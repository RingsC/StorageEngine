#ifdef FOUNDER_XDB_SE

#include "postgres.h"
#include "access/xdb_common.h"
#include "utils/builtins.h"

FormData_pg_attribute text_attribute =
{ 0,
{ "text" }, 25, -1, -1, 1, 0, -1, -1, false, 'x', 'i', false, false, false,
true, 0 ,100};

FormData_pg_attribute index_data_text_attribute =
{ 0,
{ "text" }, 25, -1, -1, 2, 0, -1, -1, false, 'x', 'i', false, false, false,
true, 0 ,100};

const int single_attribute = 1;
TupleDesc single_attibute_tupDesc = NULL;
TupleDesc toast_attibute_tupDesc = NULL;
TupleDesc cluster_index_attrs_tupDesc = 0;

//create the global single attibute relation descriptor
void fdxdb_CreateSingeAttTupleDesc()
{
	single_attibute_tupDesc = CreateTemplateTupleDesc(single_attribute, false);
	*(single_attibute_tupDesc->attrs) = &text_attribute;
}

void fdxdb_CreateClusterIndexAttTupleDesc()
{
	cluster_index_attrs_tupDesc = CreateTemplateTupleDesc(2, false);
	cluster_index_attrs_tupDesc->attrs[0] = &text_attribute;
	cluster_index_attrs_tupDesc->attrs[1] = &index_data_text_attribute;
}

void fdxb_CreateToastAttTupleDesc()
{
    toast_attibute_tupDesc = CreateTemplateTupleDesc(single_attribute, false);
}
//form colum with one attribute;
HeapTuple fdxdb_heap_formtuple(const char *p, size_t len, uint32 nEids, Oid *pEids)
{
    Datum		dvalues[1];
	dvalues[0] = fdxdb_string_formdatum(p, len);
    TupleDesc tupDesc = single_attibute_tupDesc; 
    HeapTuple	tuple;			/* return tuple */
    bool	   boolNulls[1];

    boolNulls[0] = false;
    tuple = heap_form_tuple(tupDesc , dvalues, boolNulls, nEids, pEids);
	pfree(DatumGetPointer(dvalues[0]));   

    return tuple;
}
//deform a heaptuple to chars
char* fxdb_tuple_to_chars(HeapTuple tuple)
{
    Datum  values[1] ;
    bool   isnull[1] ;
    char * chars;
    heap_deform_tuple(tuple, single_attibute_tupDesc, values, isnull);
    chars=TextDatumGetCString(values[0]);
    return chars;
}

char* fdxdb_tuple_to_chars_with_len(HeapTuple tuple, int &len)
{
    Datum  values[1] ;
    bool   isnull[1] ;
    char * chars;
    heap_deform_tuple(tuple, single_attibute_tupDesc, values, isnull);
    chars = text_to_cstring_with_len((text *) DatumGetPointer(values[0]), len);

    return chars;
}

void fxdb_tuple_print(HeapTuple tuple)
{
    Datum  values;
    bool  isnull = false;
    char * chars;
    heap_deform_tuple(tuple, single_attibute_tupDesc, &values, &isnull);
    chars=TextDatumGetCString(values);
	int len = VARSIZE_ANY_EXHDR(DatumGetCString(values));
    int i = 0;
	while (i < len)
	{
		if(chars[i])
			printf("%c", chars[i]);
		i++;
	}
	printf("\n");
}
//form a datum
Datum fdxdb_string_formdatum(const char *p, size_t len)
{
	Datum		dvalues[1];
    char *values = (char *) palloc(len + VARHDRSZ);

    SET_VARSIZE(values, len + VARHDRSZ);
    memcpy(VARDATA(values), p, len);
	dvalues[0]=PointerGetDatum(values);
	return dvalues[0];
}

Datum fdxdb_uint32_formdatum(const uint32& value)
{
	Datum		dvalues[1];
    char *values = (char *) palloc(sizeof(uint32) + VARHDRSZ);

    SET_VARSIZE(values, sizeof(uint32) + VARHDRSZ);
    memcpy(VARDATA(values), &value, sizeof(uint32));
	dvalues[0]=PointerGetDatum(values);
	return dvalues[0];
}

Datum fdxdb_string_fortdatum_bulk(char *buf, const char *p, size_t len, int &cur)
{
	if (NULL == buf) {
		return 0;
	}
	Datum dvalues[1];
	char *values = buf + cur;
    SET_VARSIZE(values, len + VARHDRSZ);
    memcpy(VARDATA(values), p, len);
	dvalues[0]=PointerGetDatum(values);
	cur += (int)(len + VARHDRSZ);
	return dvalues[0];


}
#endif //FOUNDER_XDB_SE
