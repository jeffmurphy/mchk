#include "lock.h"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_t       heldBy = 0;
