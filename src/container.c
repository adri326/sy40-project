#include "container.h"
#include <string.h>
#include "assert.h"
#include "ulid.h"

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
void print_container(const container_t* container, bool newline) {
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
void print_container_holder(const container_holder_t* holder, bool newline) {
    if (holder->is_empty) {
        printf("(Empty)%s", newline ? "\n" : "");
    } else {
        printf("(");
        print_container(&holder->container, false);
        printf(")%s", newline ? "\n" : "");
    }
}
