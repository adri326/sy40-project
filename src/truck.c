#include "truck.h"
#include "assert.h"
#include "ulid.h"

truck_t new_truck(size_t destination) {
    struct ulid_generator* generator = get_generator();
    truck_t res;

    passert_lt(size_t, "%zu", destination, N_DESTINATIONS);
    res.destination = destination;

    // Compute a uniform destination among [0; N_DESTINATIONS[ \ {destination}
    size_t dest = rand() % N_DESTINATIONS;
    if (dest == destination && N_DESTINATIONS > 1) {
        dest = (dest + rand() % (N_DESTINATIONS - 1)) % N_DESTINATIONS;
    }
    res.container = new_container_holder(false, dest);

    res.loading = false;

    char encoded[27];
    ulid_generate(generator, encoded);
    ulid_decode(res.ulid, encoded);

    return res;
}

void print_truck(truck_t* truck, bool newline) {
    char encoded[27];
    ulid_encode(encoded, truck->ulid);

    printf(
        "Truck { destination = %s (%zu), ulid = %s, loading = %s, container =%s",
        DESTINATION_NAMES[truck->destination],
        truck->destination,
        encoded,
        truck->loading ? "true" : "false",
        newline ? "\n" : " "
    );

    print_container_holder(&truck->container, true);

    printf("}%s", newline ? "\n" : "");
}

truck_lane_t new_truck_lane() {
    truck_lane_t res;
    res.trucks = NULL;
    return res;
}

void truck_lane_push(truck_lane_t* lane, truck_t* truck) {
    struct truck_ll* entry = malloc(sizeof(struct truck_ll));

    entry->next = lane->trucks;
    entry->truck = truck;

    lane->trucks = entry;
}

void free_truck_lane(truck_lane_t* truck_lane) {
    struct truck_ll* current = truck_lane->trucks;
    while (current != NULL) {
        struct truck_ll* tmp = current;
        current = tmp->next;
        free(tmp);
    }
}

void truck_lane_print(truck_lane_t* lane, bool short_version) {
    printf("TruckLane [\n");

    struct truck_ll* current = lane->trucks;
    while (current != NULL) {
        truck_t* truck = current->truck;
        printf("  ");
        if (short_version) {
            if (truck->container.is_empty) printf("(-)");
            else if (truck->container.container.destination == truck->destination) {
                printf("(v)");
            } else {
                printf("(x)");
            }
            printf(" -> %s (%zu),\n", DESTINATION_NAMES[truck->destination], truck->destination);
        } else {
            print_container_holder(&truck->container, false);
            printf(",\n");
        }
        current = current->next;
    }

    printf("]\n");
}
