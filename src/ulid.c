#include "ulid.h"
#include <ulid.h>
#include <pthread.h>
#include "assert.h"

static pthread_key_t ulid_key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

/// Must be called once per thread to initialize the ulid generator for that thread
void thread_container_init() {
    passert_eq(int, "%d", pthread_key_create(&ulid_key, NULL), 0);
}

/// Returns a ulid_generator unique to the current thread.
struct ulid_generator* get_generator() {
    pthread_once(&key_once, thread_container_init);
    struct ulid_generator* res = (struct ulid_generator*)pthread_getspecific(ulid_key);

    if (res == NULL) {
        passert_neq(void*, "%p", res = malloc(sizeof(struct ulid_generator)), NULL);
        ulid_generator_init(res, ULID_RELAXED); // We don't mind if 1 is returned or the ULIDs overflow
        passert_eq(int, "%d", pthread_setspecific(ulid_key, res), 0);
    }

    return res;
}
