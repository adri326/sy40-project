#include "boat.h"
#include "assert.h"
#include "ulid.h"

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
        "Boat { destination = %s (%zu), ulid = %s, boats = [%s",
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
