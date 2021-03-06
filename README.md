# sy40-project
Project for the SY40 class ("Architecture of operating systems")

## Installation

First, clone this repository:

```sh
git clone https://github.com/adri326/sy40-project
cd sy40-project
```

Then, retrieve the submodules:

```sh
git submodule update --init
```

Finally, run `make` to compile the project and run the binary in `./build/sy40_project`:

```sh
make -j
./build/sy40_project
```

## Design

The constraints set by the project are as follows:

- two cranes operate on a multimodal platform
- there is a *bidirectional* boat lane, with a maximum of `2` boats stationned
- there is a *unidirectional* train lane, with a maximum of `2` trains present
- there is a road lane, with a maximum of `M` trucks
- containers must be unloaded from their medium and loaded on another medium once there is enough room for it
- orientation of boats, trains and trucks, alongside with the position of the cranes matter
- trains have at most `N` wagons
- concurrency must be properly handled
- cranes cannot cross each other or reach past each other

Given this, we may take the following liberties:

- we may split the platform in two halves and restrict each crane to either of the halves
- we may add a control tower, which will spawn new trains, trucks and boats and handle situations where the two cranes would fight over the ownership of a particular resource
- we may define the boat lane as going in the opposite direction to the train
- we may specialize the two cranes to only do a specific set of operations
- we may make the trucks park and only transfer an ownership token (which would model one of the [pagers](https://en.wikipedia.org/wiki/Pager) that I've seen used in truck handling)
- containers will have a destination on them (a `uint32_t`) and boats/trucks/trains will also do
- we may add a queue of boats waiting to be loaded or unloaded; only one can be at bay so we will still only be able to interact with one, although we may tell a boat to go back in queue; as with solitaire, we might only do this if we ever feel stuck

### Organisation

There are three agents, who share a set of shared resources:

```
??: the first crane
??: the second crane
??: the control tower

A: the boat lane
B: the train lane
C: the road lane

A/??, A/??: (Option<Boat>, Mutex<Queue<Boat>>)
B/??, B/??: Mutex<Vec<Wagon>>
B/??: Vec<Vec<WagonState>>
C/??, C/??, C/??: Vec<Truck>
```

These may be visualized in the [design](./design.png).

### How the train lane works

The train lane is governed by the control tower: the control tower remembers where each wagon is (whether it is in `??` or `??`), and whether they are done being unloaded/loaded.
When it receives a message saying that a wagon is unloaded, it locks both mutexes (`B/??` and `B/??`) and transfers a wagon from `??` to `??`.
When it receives a message saying that a wagon is loaded, it stores that information. If all of the wagons of the train were finished loading, it locks `B/??` and removes them all, it then locks `B/??` and spawns a new train.

The rest of the time, `??` loads containers on the wagons and notifies `??` when it any of them become full.
`??` unloads containers from the wagons and notifies `??` when the head wagon was unloaded.

It is important that `B/??` and `B/??` don't get locked for a long time, and that only the control tower may require both `B/??` and `B/??`.

### How the boat lane works

The boat lane works in a similar way to the train lane, except that the list of boats isn't accessed as often.

One boat is stationned at the shore of either cranes and does not need a mutex to be accessed - it is exclusive to the crane.

`??` unloads containers from boats. Once a boat is empty, it messages `??` about it and `??` can add the boat to `??`'s queue. It then pops a boat from its queue, if there are any.

`??` loads containers onto boats. Once a boat is full, it messages `??` about it and `??` can spawn a new boat for `??`.

### How the traffic lane works

The traffic lane works differently: all of the trucks are instructed to sit on a parking and are given a pager.
The paired pager is given to either `??`, `??` or `??`, it represents the exlusive ownership of that truck; this pager can only be given to another process by its owner.

Once a truck is unloaded, its pager is passed to the other crane.
Once a truck is loaded, its pager is passed to `??` to send the truck away and generate a new truck at its place.

### How communication works

`??` is a "sleeping" agent: it needs to be woken up with a monitor to then operate on the messages it receives.
For an agent `??` to notify `??`, we need a message queue `Q(??)`, alongside a mutex `S(??)` and a monitor `M(??)`:

```
Function send_??(message) in ??:
    S(??).P() // Deadlock warning: S(??) must be unlocked at this instruction
    Q(??).send(message)
    M(??).signal(S(??))

Loop in ??:
    M(??).wait()
    message = Q(??).read()
    S(??).V()
    // Handle message
```

`??` and `??` never sleep, so communication with them is easier. They each have a mutex `S(??)` and `S(??)` and a message queue `Q(??)` and `Q(??)`.

```
Function send(message, ??: ??|??) in ??:
    S(??).P() // Deadlock warning: S(??) must be unlocked at this instruction
    Q(??).send(message)
    S(??).V()

Loop in ??:
    S(??).P() // Safe to assume that S(??) is likely to have been previously locked in the current thread
    message = Q(??).read()
    S(??).V()
    // Handle message
```
