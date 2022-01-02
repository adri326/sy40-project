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
α: the first crane
β: the second crane
γ: the control tower

A: the boat lane
B: the train lane
C: the road lane

A/α, A/β: (Option<Boat>, Mutex<Queue<Boat>>)
B/α, B/β: Mutex<Vec<Wagon>>
B/γ: Vec<Vec<WagonState>>
C/α, C/β, C/γ: Vec<Truck>
```

These may be visualized in the [design](./design.png).

### How the train lane works

The train lane is governed by the control tower: the control tower remembers where each wagon is (whether it is in `α` or `β`), and whether they are done being unloaded/loaded.
When it receives a message saying that a wagon is unloaded, it locks both mutexes (`B/α` and `B/β`) and transfers a wagon from `β` to `α`.
When it receives a message saying that a wagon is loaded, it stores that information. If all of the wagons of the train were finished loading, it locks `B/α` and removes them all, it then locks `B/β` and spawns a new train.

The rest of the time, `α` loads containers on the wagons and notifies `γ` when it any of them become full.
`β` unloads containers from the wagons and notifies `γ` when the head wagon was unloaded.

It is important that `B/α` and `B/β` don't get locked for a long time, and that only the control tower may require both `B/α` and `B/β`.

### How the boat lane works

The boat lane works in a similar way to the train lane, except that the list of boats isn't accessed as often.

One boat is stationned at the shore of either cranes and does not need a mutex to be accessed - it is exclusive to the crane.

`α` unloads containers from boats. Once a boat is empty, it messages `γ` about it and `γ` can add the boat to `β`'s queue. It then pops a boat from its queue, if there are any.

`β` loads containers onto boats. Once a boat is full, it messages `γ` about it and `γ` can spawn a new boat for `α`.

### How the traffic lane works

The traffic lane works differently: all of the trucks are instructed to sit on a parking and are given a pager.
The paired pager is given to either `α`, `β` or `γ`, it represents the exlusive ownership of that truck; this pager can only be given to another process by its owner.

Once a truck is unloaded, its pager is passed to the other crane.
Once a truck is loaded, its pager is passed to `γ` to send the truck away and generate a new truck at its place.

### How communication works

`γ` is a "sleeping" agent: it needs to be woken up with a monitor to then operate on the messages it receives.
For an agent `σ` to notify `γ`, we need a message queue `Q(γ)`, alongside a mutex `S(γ)` and a monitor `M(γ)`:

```
Function send_γ(message) in σ:
    S(γ).P() // Deadlock warning: S(σ) must be unlocked at this instruction
    Q(γ).send(message)
    M(γ).signal(S(γ))

Loop in γ:
    M(γ).wait()
    message = Q(γ).read()
    S(γ).V()
    // Handle message
```

`α` and `β` never sleep, so communication with them is easier. They each have a mutex `S(α)` and `S(β)` and a message queue `Q(α)` and `Q(β)`.

```
Function send(message, τ: α|β) in σ:
    S(τ).P() // Deadlock warning: S(σ) must be unlocked at this instruction
    Q(τ).send(message)
    S(τ).V()

Loop in τ:
    S(τ).P() // Safe to assume that S(τ) is likely to have been previously locked in the current thread
    message = Q(τ).read()
    S(τ).V()
    // Handle message
```
