//index �������ݲ���
bool test_indexscan_LargeMount_dll();

//�����������������ͬ�����ϣ���֤���
bool test_indexMult_SameCol_dll();

//�����ȴ��������ٲ�������
bool test_indexCreatefirst_thenInsert();

//�����ڶ����ϸ�����scancondition
bool test_indexMultmethod_oneCol_dll();

//���Դ���Ψһ�����������������Ȼ��ͨ��gettype���type��
bool test_index_uniqe_01_dll();


extern ColumnInfo heap_colinfo;
extern ColumnInfo index_colinfo;
void form_heap_colinfo_uniqe_01_dll(ColumnInfo &colinfo);
void form_index_colinfo_uniqe_01_dll(ColumnInfo &colinfo);

