#include "heap/compress.h"
#include "postgres.h"
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include "access/genam.h"
#include "access/heapam.h"
#include "access/tuptoaster.h"
#include "access/xact.h"
#include "catalog/catalog.h"
#include "utils/fmgroids.h"
#include "utils/pg_lzcompress.h"
#include "utils/pg_lzcompress.h"
#include "utils/toast_test_utils.h"
static const int NDATAITEMCNT = 20;
bool TestLZCompress( void )
{
	PGLZ_Header* pHeader = (PGLZ_Header*)malloc(1 << 25);
	char *pDest = (char*)malloc(1 << 25);

	for (int i = 0; i < NDATAITEMCNT; ++ i)
	{
		std::string s;
		RandomGenString(s,1 << 24);

	    if(pglz_compress(s.c_str(),s.length(),pHeader,NULL))
		{
			pglz_decompress(pHeader,pDest);

			if (0 != memcmp(s.c_str(),pDest,s.length()))
			{
				std::cout<<"Error: ";
				for (int nPos = 0;nPos < s.length();++nPos)
				{
					if (s.at(nPos) != pDest[nPos])
					{
						std::cout<<"("<<s.at(nPos)<<","<<pDest[nPos]<<")";
					}
				}
				std::cout<<std::endl;
			}
		}
		else
		{
			std::cout<<"can not compress!"<<std::endl;
		}


		
	}
	delete pDest;
	delete pHeader;
    return true;
}