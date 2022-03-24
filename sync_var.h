#pragma once
#include <stdbool.h>

struct SyncVar;
typedef struct SyncVar SyncVar;

SyncVar* syncVarCreate(void);
void syncVarDestroy(SyncVar* var);
void syncVarSet(SyncVar* var, void* value);
void* syncVarGet(SyncVar* var);
bool syncVarIsAbsent(SyncVar* var);
