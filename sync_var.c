#include "sync_var.h"
#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

struct SyncVar {
    void* value;
    sem_t* present;
    sem_t* absent;
    sem_t* mutex;
#if __APPLE__
    char* presentName;
    char* absentName;
    char* mutexName;
#endif
};

SyncVar* syncVarCreate(void) {
    errno = 0;
    SyncVar* var = malloc(sizeof(SyncVar));
    var->value = NULL;

#ifdef __APPLE__
    static int id = 0;
    var->presentName = malloc(20);
    var->absentName = malloc(20);
    var->mutexName = malloc(20);
    sprintf(var->presentName, "/%i-present", id);
    sprintf(var->absentName, "/%i-absent", id);
    sprintf(var->mutexName, "/%i-mutex", id);
    var->present = sem_open(var->presentName, O_CREAT, 0644, 1);
    var->absent = sem_open(var->absentName, O_CREAT, 0644, 0);
    var->mutex = sem_open(var->mutexName, O_CREAT, 0644, 1);
    id++;
#else
    sem_init(var->present, 0, 1);
    sem_init(var->absent, 0, 0);
    sem_init(var->mutex, 0, 1);
#endif

    if(errno) {
        perror("syncVarCreate failed");
        return NULL;
    }

    return var;
}

void syncVarDestroy(SyncVar* var) {
#ifdef __APPLE__
    sem_close(var->present);
    sem_close(var->absent);
    sem_close(var->mutex);
    sem_unlink(var->presentName);
    sem_unlink(var->absentName);
    sem_unlink(var->mutexName);
    free(var->presentName);
    free(var->absentName);
    free(var->mutexName);
#else
    sem_destroy(var->present);
    sem_destroy(var->absent);
    sem_destroy(var->mutex);
#endif
    if(var->value) {
        free(var->value);
    }
    free(var);
}

void syncVarSet(SyncVar* var, void* value) {
    sem_wait(var->present);
    // sem_wait(var->mutex);
    var->value = value;
    // sem_post(var->mutex);
    sem_post(var->absent);
}

void* syncVarGet(SyncVar* var) {
    sem_wait(var->absent);
    // sem_wait(var->mutex);
    void* v = var->value;
    var->value = NULL;
    // sem_post(var->mutex);
    sem_post(var->present);
    return v;
}

bool syncVarIsAbsent(SyncVar* var) {
    return var->value == NULL;
}
