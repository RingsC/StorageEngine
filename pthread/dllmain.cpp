// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
#include <windows.h>
#include "pthread/pthread_init.h"
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		process_attach();
		break;
	case DLL_THREAD_ATTACH:
		pthread_attach_np();
		break;
	case DLL_THREAD_DETACH:
		pthread_detach_np();
		break;
	case DLL_PROCESS_DETACH:
		process_detach();
		break;
	}
	return TRUE;
}

