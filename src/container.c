#include "container.h"
#include <string.h>
#include <ulid.h>
#include <pthread.h>
#include "assert.h"

static pthread_key_t ulid_key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

/// Should be called once per thread to destroy the ulid generator for that thread
void thread_container_destroy() {
    struct ulid_generator* res = (struct ulid_generator*)pthread_getspecific(ulid_key);

    if (res != NULL) {
        free(res);
        pthread_setspecific(ulid_key, NULL);
    }
}

/// Must be called once per thread to initialize the ulid generator for that thread
void thread_container_init() {
    pthread_key_create(&ulid_key, thread_container_destroy);
}

/// Returns a ulid_generator unique to the current thread.
struct ulid_generator* get_generator() {
    pthread_once(&key_once, thread_container_init);
    struct ulid_generator* res = (struct ulid_generator*)pthread_getspecific(ulid_key);

    if (res == NULL) {
        res = malloc(sizeof(struct ulid_generator));
        ulid_generator_init(res, ULID_RELAXED);
        pthread_setspecific(ulid_key, res);
    }

    return res;
}

/// Creates a new container; a thread-specific ulid_generator is implicitely created
container_t new_container(size_t destination) {
    struct ulid_generator* generator = get_generator();

    passert_lt(size_t, "%zu", destination, N_DESTINATIONS);
    container_t res;
    res.destination = destination;
    char encoded[27];
    ulid_generate(generator, encoded);
    ulid_decode(res.ulid, encoded);

    return res;
}

/// Used for debugging
void print_container(container_t* container, bool newline) {
    char encoded[27];
    ulid_encode(encoded, container->ulid);
    printf(
        "Container { destination = %s (%zu), ulid = %s }%s",
        DESTINATION_NAMES[container->destination],
        container->destination,
        encoded,
        newline ? "\n" : ""
    );
}

/// Creates a new container holder. If `is_empty` is true, all of the fields of the container hold are set to zero.
container_holder_t new_container_holder(bool is_empty, size_t destination) {
    container_holder_t res;

    res.is_empty = is_empty;

    if (is_empty) {
        res.container.destination = 0;
        memset(&res.container.ulid, 0, 16);
    } else {
        res.container = new_container(destination);
    }

    return res;
}

/// Used for debugging
void print_container_holder(container_holder_t* holder, bool newline) {
    if (holder->is_empty) {
        printf("(Empty)%s", newline ? "\n" : "");
    } else {
        printf("(");
        print_container(&holder->container, false);
        printf(")%s", newline ? "\n" : "");
    }
}
