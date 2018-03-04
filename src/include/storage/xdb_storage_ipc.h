#ifndef XDB_STORAGE_IPC_HPP
#define XDB_STORAGE_IPC_HPP

///bootstrap.h
typedef enum
{
	CheckerProcess,
	BootstrapProcess,
	StartupProcess,
	BgWriterProcess,
	WalWriterProcess,
	WalReceiverProcess,

	NUM_AUXPROCTYPES			/* Must be last! */
} AuxProcType;

#endif
