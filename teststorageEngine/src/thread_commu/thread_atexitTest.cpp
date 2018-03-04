#include "thread_commu/thread_atexit_text.h"
#include "port/thread_commu.h"
#include "utils/util.h"
#include "thread_commu/TestThreadCommon.h"
#include "test_fram.h"
static pthread_t thrd1;
static bool bExit1Called = false;
void Exit1( void )
{
    bExit1Called = true;
	pthread_t self = pthread_self();
    CHECK_BOOL(self == thrd1);
	printf("%d call exit func.\n",self);
}

void* func1(void* p)
{
	printf("begin thread Id:%d.\n",pthread_self());
    ThreadAtExit(Exit1);
	pthread_t self = pthread_self();
	CHECK_BOOL(self == thrd1);
	//Sleep(2000);
	return NULL;

}
bool test_thread_normal_exit( void )
{
	INTENT("test_thread_normal_exit");
    pthread_create(&thrd1,NULL,func1,NULL);
	int err = pthread_join(thrd1,NULL);
	CHECK_BOOL(bExit1Called == true);
	printf("err code:%d\n",err);
	return true;
}