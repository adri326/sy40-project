#include "train.h"
#include "assert.h"
#include "ulid.h"
#include <pthread.h>

wagon_t new_wagon(const train_t* train, size_t n_cargo) {
    struct ulid_generator* generator = get_generator();
    wagon_t res;

    res.destination = train->destination;
    res.train = train;

    size_t n = 0;
    for (; n < n_cargo && n < WAGON_CONTAINERS; n++) { // fill the n_cargo first elements with random destinations
        size_t dest = rand() % N_DESTINATIONS;
        if (dest == train->destination && N_DESTINATIONS > 1) {
            // (X ~> U[0; n[) + (Y ~> U[0; n[) ~> U[0; n[ in the finite field (ℕ mod n)
            dest = (dest + rand() % (N_DESTINATIONS - 1)) % N_DESTINATIONS;
        }
        res.containers[n] = new_container_holder(false, dest);
    }
    for (; n < WAGON_CONTAINERS; n++) { // fill the other elements with empty slots
        res.containers[n] = new_container_holder(true, 0);
    }

    char encoded[27];
    ulid_generate(generator, encoded);
    ulid_decode(res.ulid, encoded);

    return res;
}

void print_wagon(wagon_t* wagon, bool newline) {
    char encoded[27];
    ulid_encode(encoded, wagon->ulid);

    printf(
        "Wagon { destination = %s (%zu), ulid = %s, train = %p, containers = [%s",
        DESTINATION_NAMES[wagon->destination],
        wagon->destination,
        encoded,
        wagon->train,
        newline ? "\n" : ""
    );

    for (size_t n = 0; n < WAGON_CONTAINERS; n++) {
        if (newline) printf("  ");
        print_container_holder(&wagon->containers[n], false);
        if (n < WAGON_CONTAINERS - 1) printf(", %s", newline ? "\n" : "");
    }

    printf("%s] }%s", newline ? "\n" : "", newline ? "\n" : "");
}

bool wagon_is_full(wagon_t* wagon) {
    for (size_t n = 0; n < WAGON_CONTAINERS; n++) {
        if (wagon->containers[n].is_empty) return false;
    }
    return true;
}

bool wagon_is_empty(wagon_t* wagon) {
    for (size_t n = 0; n < WAGON_CONTAINERS; n++) {
        if (!wagon->containers[n].is_empty) return false;
    }
    return true;
}

size_t wagon_loaded(wagon_t* wagon) {
    size_t res = 0;
    for (size_t n = 0; n < WAGON_CONTAINERS; n++) {
        if (!wagon->containers[n].is_empty) res += 1;
    }
    return res;
}

container_holder_t* wagon_first_empty(wagon_t* wagon) {
    for (size_t n = 0; n < WAGON_CONTAINERS; n++) {
        if (wagon->containers[n].is_empty) return &wagon->containers[n];
    }
    return NULL;
}

train_t* new_train(size_t destination, size_t n_wagons) {
    train_t* res = malloc(sizeof(train_t));

    passert_lt(size_t, "%zu", destination, N_DESTINATIONS);
    res->destination = destination;
    res->n_wagons = n_wagons <= TRAIN_WAGONS ? n_wagons : TRAIN_WAGONS;

    for (size_t n = 0; n < n_wagons && n < TRAIN_WAGONS; n++) {
        res->wagons[n] = new_wagon(res, rand() % WAGON_CONTAINERS);
        res->wagon_full[n] = false;
    }

    return res;
}

void free_train(train_t* train) {
    free(train);
}

train_lane_t new_train_lane() {
    train_lane_t res;
    for (size_t n = 0; n < LANE_WAGONS; n++) {
        res.wagons[n] = NULL;
    }
    res.n_wagons = 0;

    pthread_mutexattr_t attributes;
    passert_eq(int, "%d", pthread_mutexattr_init(&attributes), 0);
    passert_eq(int, "%d", pthread_mutex_init(&res.mutex, &attributes), 0);
    passert_eq(int, "%d", pthread_mutexattr_destroy(&attributes), 0);

    return res;
}

void free_train_lane(train_lane_t* train_lane) {
    pthread_mutex_destroy(&train_lane->mutex);
}

void train_lane_lock(train_lane_t* train_lane) {
    pthread_mutex_lock(&train_lane->mutex);
}
void train_lane_unlock(train_lane_t* train_lane) {
    pthread_mutex_unlock(&train_lane->mutex);
}

void train_lane_shift(train_lane_t* train_lane, size_t shift_by) {
    for (size_t n = shift_by; n < train_lane->n_wagons; n++) {
        train_lane->wagons[n - shift_by] = train_lane->wagons[n];
    }
    if (train_lane->n_wagons > shift_by) train_lane->n_wagons -= shift_by;
    else train_lane->n_wagons = 0;
}

void train_lane_append(train_lane_t* train_lane, wagon_t* wagon) {
    passert_lt(size_t, "%zu", train_lane->n_wagons, LANE_WAGONS, "No more space left to add wagons in the lane!");

    train_lane->wagons[train_lane->n_wagons] = wagon;
    train_lane->n_wagons++;
}

void train_lane_print(train_lane_t* train_lane, bool short_version) {
    printf("TrainLane { n_wagons = %zu, wagons = [\n", train_lane->n_wagons);

    for (size_t n = 0; n < train_lane->n_wagons; n++) {
        printf("  ");
        if (short_version) {
            wagon_t* wagon = train_lane->wagons[n];
            printf("(");
            for (size_t o = 0; o < WAGON_CONTAINERS; o++) {
                if (wagon->containers[o].is_empty) {
                    printf("-");
                } else if (wagon->destination != wagon->containers[o].container.destination) {
                    printf("x");
                } else {
                    printf("v");
                }
            }
            printf(") -> %s (%zu),\n", DESTINATION_NAMES[wagon->destination], wagon->destination);
        } else {
            print_wagon(train_lane->wagons[n], false);
        }
    }
}

wagon_t* train_lane_accepts(train_lane_t* train_lane, size_t destination) {
    for (size_t n = 0; n < train_lane->n_wagons; n++) {
        wagon_t* wagon = train_lane->wagons[n];

        if (wagon->destination == destination && !wagon_is_full(wagon)) return wagon;
    }

    return NULL;
}
