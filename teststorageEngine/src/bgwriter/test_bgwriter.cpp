#include "time.h"

#include "test_fram.h"
#include "bgwriter/test_bgwriter.h"

int test_bgwriter_000()
{
	INTENT("运行该测例超过15秒，让bgwriter因为超时做checkpoint。"
				 "查看在超时情况下bgwriter能否正常工作。");
	time_t checkpoint_time = time(NULL);
	while(time(NULL) - checkpoint_time < 15/*超过5秒，保证checkpoint*/);
	return 1;
}