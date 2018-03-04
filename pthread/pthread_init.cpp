#include "windows.h"
#include "pthread.h"
#include "pthread_init.h"
#define MAX_SPECIFIC_KEY 64
SpecDes SpecDesArray[MAX_SPECIFIC_KEY];
int g_nNextSolt = 0;
pthread_mutex_t SpecMut;
void process_attach( void )
{
    memset(&SpecDesArray,0,sizeof(SpecDesArray));
	pthread_mutex_init(&SpecMut,NULL);
}

void process_detach( void )
{
	pthread_mutex_destroy(&SpecMut);
}

void pthread_attach_np( void )
{
}

void AddSpecDesctor(pthread_key_t key,KeyDestructor desc)
{
    pthread_mutex_lock(&SpecMut);
    SpecDesArray[g_nNextSolt].key = key;
	SpecDesArray[g_nNextSolt].desc = desc;
	++g_nNextSolt;
	pthread_mutex_unlock(&SpecMut);
}

void pthread_detach_np( void )
{
    for (int i = 0; i < MAX_SPECIFIC_KEY; ++i)
    {
		if (0 != SpecDesArray[i].key && NULL != SpecDesArray[i].desc)
		{
			LPVOID pValue = TlsGetValue(SpecDesArray[i].key);
			if (NULL != pValue)
			{
				SpecDesArray[i].desc(pValue);
			}
		}
    }
}
