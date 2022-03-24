#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sync_var.h"

typedef struct {
    int code;
    int index;
    int optional;
} Command;

typedef union {
    unsigned long int i;
    double d;
} Data;

static volatile int producerRunning = 1;
static volatile int consumersRunning = 1;
static pthread_t producerThread;
static pthread_t bonifacyThread;
static pthread_t randomThread;
static SyncVar* command;
static SyncVar* buffer[4];

static unsigned long fibonacci(unsigned long n) {
    const double a = sqrt(5.0);
    const double b = 1.0 / a;
    return b*pow((1.0 + a)/2, n) - b*pow((1.0 - a)/2, n);
}

static void* producerMain(void* args) {
    while(producerRunning) {
        Command* c = syncVarGet(command);
        if(!c) {
            printf("[prod] This should never happen\n");
            continue;
        }

        printf("[prod] Recv cmd %i, %i, %i\n", c->code, c->index, c->optional);

        Data* data = malloc(sizeof(Data));

        if(c->code == 0) {
            data->i = fibonacci(c->optional);
        }
        else if(c->code == 1) {
            data->d = (double)rand() / (double)RAND_MAX;
        }

        syncVarSet(buffer[c->index], data);
        free(c);
    }
    
    printf("[prod] Exit\n");
    return NULL;
}

static void* bonifacyMain(void* args) {
    while(consumersRunning) {
        Command* c = malloc(sizeof(Command));
        c->code = 0;
        c->index = rand() % 4;
        c->optional = rand() % 100;
        syncVarSet(command, c);

        Data* data = syncVarGet(buffer[c->index]);

        while(data == NULL) {
            printf("[boni] This should never happen\n");
            data = syncVarGet(buffer[c->index]);
        };
        
        printf("[boni] Got value: %lu\n", data->i);
        free(data);

        sleep(1);
    }
    
    printf("[boni] Exit\n");
    return NULL;
}

static void* randomMain(void* args) {
    while(consumersRunning) {
        Command* c = malloc(sizeof(Command));
        c->code = 1;
        c->index = rand() % 4;
        syncVarSet(command, c);

        Data* data = syncVarGet(buffer[c->index]);

        while(data == NULL) {
            printf("[rand] This should never happen\n");
            data = syncVarGet(buffer[c->index]);
        };
        
        printf("[rand] Got value: %f\n", data->d);
        free(data);

        sleep(2);
    }
    
    printf("[rand] Exit\n");
    return NULL;
}

static void signalHandler(int sig) {
    printf("Exiting...\n");
    consumersRunning = 0;
    pthread_join(bonifacyThread, NULL);
    pthread_join(randomThread, NULL);
    producerRunning = 0;
    if(syncVarIsAbsent(command)) {
        Command* c = malloc(sizeof(Command));
        c->code = 0;
        c->index = rand() % 4;
        syncVarSet(command, c);
    }
    pthread_join(producerThread, NULL);
    syncVarDestroy(command);
    for(int i = 0; i < 4; i++) {
        syncVarDestroy(buffer[i]);
    }
    printf("Exit\n");
    exit(0);
}

int main(void) {
    signal(SIGINT, signalHandler);
    srand(time(NULL));

    if(!(command = syncVarCreate())) {
        return EXIT_FAILURE;
    }

    for(int i = 0; i < 4; i++) {
        if(!(buffer[i] = syncVarCreate())) {
            return EXIT_FAILURE;
        }
    }

    pthread_create(&producerThread, NULL, producerMain, NULL);
    pthread_create(&bonifacyThread, NULL, bonifacyMain, NULL);
    pthread_create(&randomThread, NULL, randomMain, NULL);
    pthread_join(producerThread, NULL);
    pthread_join(bonifacyThread, NULL);
    pthread_join(randomThread, NULL);
    return EXIT_SUCCESS;
}
