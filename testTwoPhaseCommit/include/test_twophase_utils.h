#ifndef TWOPHASE_UTILS_H
#define TWOPHASE_UTILS_H
#include "Configs.h"
#include <stdlib.h>
#include <iostream>
#include <vector>
#include "PGSETypes.h"
#include "Transaction.h"
#include "EntrySet.h"

using namespace FounderXDB::StorageEngineNS;

int str_compare(const char *str1, size_t len1, const char *str2, size_t len2);
int start_engine_();
int stop_engine_();
storage_params *get_param( void );
storage_params* GetStorageParam(std::string& strConfigFile);

void twophase_prepare(char* gid);
void twophase_finish(char* gid,bool isCommit = true);
FXGlobalTransactionId GetGlobalTxid();
bool TwophaseDropEntrySet(const EntrySetID& entrySetId);
#endif//TWOPHASE_UTILS_H
