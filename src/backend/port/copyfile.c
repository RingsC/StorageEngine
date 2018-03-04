/*
* Copy a (real) file to another (real) file.
* file can be a regular file or a simlink
*/
#include "postgres.h"
#include <stdio.h>
#include <sys/stat.h>

#ifdef _WIN32
static const int ERR_COPY_FILE = 2;
#else
static const int ERR_COPY_FILE = 256;
#endif
int copyfile(const char *source, const char *target)
{
	struct stat src_stat;

	if (lstat(source, &src_stat) != 0)
	{
		return ERR_COPY_FILE;
	}

	//if (lstat(target,&dst_stat) == 0)
	//{
	//	return EEXIST;
	//}

	if (S_ISREG(src_stat.st_mode))
	{
		FILE  *src, *dst;
		int   rsize;
		char  buf[1024];
		bool  hasError = false;
		
		if (NULL == (src = fopen(source, "r")))
		{
			return ERR_COPY_FILE;
		}

		if (NULL == (dst = fopen(target, "w")))
		{
			fclose(src);
			return ERR_COPY_FILE;
		}

		while (!feof(src))
		{		
			rsize = (int)fread(buf,1, sizeof(buf),src);
			if(ferror(src) != 0)
			{
				hasError = true;
				perror("src");
				break;
			}

			int wsize = 0;
			while (rsize > 0)
			{
				wsize = (int)fwrite(buf,1, rsize, dst);
				if(ferror(dst) != 0)
				{
					hasError = true;
					break;
				}
				rsize -= wsize >= 0 ? wsize : 0;
			}

			if (hasError)
				break;
		}

		fclose(src);
		fclose(dst);

		return hasError ? ERR_COPY_FILE : 0;
	}
	else 
	{
		return ERR_COPY_FILE;
	}
}