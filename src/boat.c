#include "boat.h"
#include "assert.h"
#include "ulid.h"
#include <pthread.h>

boat_t new_boat(size_t destination, size_t n_cargo) {
    struct ulid_generator* generator = get_generator();
    boat_t res;

    passert_lt(size_t, "%zu", destination, N_DESTINATIONS);
    res.destination = destination;

    size_t n = 0;
    for (; n < n_cargo; n++) { // fill the n_cargo first elements with random destinations
        size_t dest = rand() % N_DESTINATIONS;
        if (dest == destination && N_DESTINATIONS > 1) {
            // (X ~> U[0; n[) + (Y ~> U[0; n[) ~> U[0; n[ in the finite field (ℕ mod n)
            dest = (dest + rand() % (N_DESTINATIONS - 1)) % N_DESTINATIONS;
        }
        res.containers[n] = new_container_holder(false, dest);
    }
    for (; n < BOAT_CONTAINERS; n++) { // fill the other elements with empty slots
        res.containers[n] = new_container_holder(true, 0);
    }

    char encoded[27];
    ulid_generate(generator, encoded);
    ulid_decode(res.ulid, encoded);

    return res;
}

void print_boat(const boat_t* boat, bool newline) {
    char encoded[27];
    ulid_encode(encoded, boat->ulid);

    printf(
        "Boat { destination = %s (%zu), ulid = %s, containers = [%s",
        DESTINATION_NAMES[boat->destination],
        boat->destination,
        encoded,
        newline ? "\n" : ""
    );

    for (size_t n = 0; n < BOAT_CONTAINERS; n++) {
        if (newline) printf("  ");
        print_container_holder(&boat->containers[n], false);
        if (n < BOAT_CONTAINERS - 1) printf(", %s", newline ? "\n" : "");
    }

    printf("%s] }%s", newline ? "\n" : "", newline ? "\n" : "");
}

boat_deque* new_boat_deque(size_t capacity) {
    passert_gt(size_t, "%zu", capacity, 0, "Capacity may not be zero, as to avoid undefined behavior.");

    boat_deque* res = (boat_deque*)malloc(sizeof(boat_deque));
    res->buffer = (boat_t*)malloc(capacity * sizeof(boat_t));
    res->capacity = capacity;
    res->length = 0;
    res->begin = 0;

    return res;
}

void free_boat_deque(boat_deque* queue) {
    free(queue->buffer);
    free(queue);
}

void boat_deque_resize(boat_deque* queue, size_t capacity) {
    boat_t* new_buffer = (boat_t*)malloc(capacity * sizeof(boat_t));
    passert_neq(boat_t*, "%p", new_buffer, NULL, "Couldn't allocate %zu bytes of memory", capacity * sizeof(boat_t));

    // Compiler plz optimize away
    for (size_t n = 0; n < queue->length; n++) {
        new_buffer[n] = queue->buffer[(queue->begin + n) % queue->capacity];
    }

    free(queue->buffer);
    queue->buffer = new_buffer;
    queue->capacity = capacity;
    queue->begin = 0;
}

bool boat_deque_pop_front(boat_deque* queue, boat_t* dest) {
    if (queue->length == 0) return false;
    *dest = queue->buffer[queue->begin];
    queue->begin = (queue->begin + 1) % queue->capacity;
    return true;
}

boat_t* boat_deque_get(boat_deque* queue, size_t n) {
    if (queue->length == 0) return NULL;
    return &queue->buffer[(queue->begin + n) % queue->capacity];
}

void boat_deque_push_back(boat_deque* queue, boat_t boat) {
    if (queue->length == queue->capacity) {
        boat_deque_resize(queue, queue->capacity * 2);
    }
    queue->buffer[(queue->begin + queue->length) % queue->capacity] = boat;
    queue->length += 1;
}

void boat_deque_print(boat_deque* queue, bool short_version) {
    if (short_version) {
        printf("BoatDeque { length = %zu, boats = [\n", queue->length);
        for (size_t n = 0; n < queue->length; n++) {
            printf("  (");
            boat_t* boat = boat_deque_get(queue, n);
            for (size_t o = 0; o < BOAT_CONTAINERS; o++) {
                if (boat->containers[o].is_empty) {
                    printf("-");
                } else if (boat->destination != boat->containers[o].container.destination) {
                    printf("x");
                } else {
                    printf("v");
                }
            }
            printf(") -> %s (%zu)", DESTINATION_NAMES[boat->destination], boat->destination);
            if (n < queue->length - 1) printf(",");
            printf("\n");
        }
        printf("] }\n");
    } else {
        printf("BoatDeque { length = %zu, boats = [\n", queue->length);
        for (size_t n = 0; n < queue->length; n++) {
            printf("  ");

            print_boat(boat_deque_get(queue, n), false);

            if (n < queue->length - 1) printf(",\n");
        }
        printf("\n] }\n");
    }
}

boat_lane_t new_boat_lane() {
    boat_lane_t res;
    res.queue = new_boat_deque(1);
    res.has_current_boat = false;

    pthread_mutexattr_t attributes;
    passert_eq(int, "%d", pthread_mutexattr_init(&attributes), 0);
    passert_eq(int, "%d", pthread_mutex_init(&res.mutex, &attributes), 0);
    passert_eq(int, "%d", pthread_mutexattr_destroy(&attributes), 0);

    return res;
}

void free_boat_lane(boat_lane_t* boat_lane) {
    free_boat_deque(boat_lane->queue);
    pthread_mutex_destroy(&boat_lane->mutex);
}

void boat_lane_lock(boat_lane_t* boat_lane) {
    passert_eq(int, "%d", pthread_mutex_lock(&boat_lane->mutex), 0);
}

void boat_lane_unlock(boat_lane_t* boat_lane) {
    passert_eq(int, "%d", pthread_mutex_unlock(&boat_lane->mutex), 0);
}
