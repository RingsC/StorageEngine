#include "time.h"

#include "test_fram.h"
#include "bgwriter/test_bgwriter.h"

int test_bgwriter_000()
{
	INTENT("���иò�������15�룬��bgwriter��Ϊ��ʱ��checkpoint��"
				 "�鿴�ڳ�ʱ�����bgwriter�ܷ�����������");
	time_t checkpoint_time = time(NULL);
	while(time(NULL) - checkpoint_time < 15/*����5�룬��֤checkpoint*/);
	return 1;
}