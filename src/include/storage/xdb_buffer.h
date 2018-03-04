#ifndef XDB_BUFFER_HPP
#define XDB_BUFFER_HPP

///excutor/instrument.h
typedef struct BufferUsage
{
	long		shared_blks_hit;	/* # of shared buffer hits */
	long		shared_blks_read;		/* # of shared disk blocks read */
	long		shared_blks_written;	/* # of shared disk blocks written */
	long		local_blks_hit; /* # of local buffer hits */
	long		local_blks_read;	/* # of local disk blocks read */
	long		local_blks_written;		/* # of local disk blocks written */
	long		temp_blks_read; /* # of temp blocks read */
	long		temp_blks_written;		/* # of temp blocks written */
} BufferUsage;

extern PGDLLIMPORT BufferUsage pgBufferUsage;

#endif