use petri_network::*;
use petri_network::builder::*;
use petri_network::builder::structures::Semaphore;

fn semaphore_ring(builder: &mut PetriBuilder, nodes: usize) -> Vec<Semaphore> {
    let mut res = Vec::with_capacity(nodes);
    for n in 0..nodes {
        res.push(Semaphore::new(builder, if n == 0 {1} else {0}));
    }
    res
}

fn clock(builder: &mut PetriBuilder, cycle_length: usize, active: bool) -> (usize, usize) { // (clock_in, clock_out)
    let first = builder.node(if active {1} else {0});
    if cycle_length == 1 {
        return (first, first);
    }

    let mut last = first;
    for _n in 1..cycle_length {
        let next = builder.node(0);
        builder.transition(vec![last], vec![next]);
        last = next;
    }
    builder.transition(vec![last], vec![first]);

    (first, last)
}

struct TruckNetwork {
    initial: usize,
    empty: usize,
    full: usize,
    semaphores: Vec<usize>,
    alpha: Option<(usize, usize, usize)>,
    beta: Option<(usize, usize, usize)>,
    network: PetriNetwork
}

fn truck_network(clock_period: usize, unload: bool, load: bool) -> TruckNetwork {
    let mut builder = PetriBuilder::new();

    let initial = builder.node(1);
    let empty = builder.node(0);
    let full = builder.node(0);

    let semaphores = semaphore_ring(&mut builder, 3);

    let mut alpha = None;
    let mut beta = None;

    // Unload
    if unload {
        let (alpha_in, alpha_out) = clock(&mut builder, clock_period, true);
        builder.label(Label::Node(alpha_in), "alpha_in");
        builder.label(Label::Node(alpha_out), "alpha_out");

        builder.begin_branch(alpha_out)
            .step()
            .step_with_mod(semaphores[0].p())
            .label("unload")
            .step_with_mod(semaphores[0].v()).label("alpha_loop");

        let unload = builder.get_label("unload").unwrap().as_node().unwrap();
        let alpha_loop = builder.get_label("alpha_loop").unwrap().as_node().unwrap();
        alpha = Some((alpha_in, alpha_out, alpha_loop));

        builder.transition(vec![alpha_loop], vec![alpha_in]);
        builder.transition(vec![unload, initial], vec![empty, alpha_loop, semaphores[1].index()]);
    }

    // Load
    if load {
        let (beta_in, beta_out) = clock(&mut builder, clock_period, true);
        builder.label(Label::Node(beta_in), "beta_in");
        builder.label(Label::Node(beta_out), "beta_out");

        builder.begin_branch(beta_out)
            .step()
            .step_with_mod(semaphores[1].p())
            .label("load")
            .step_with_mod(semaphores[1].v()).label("beta_loop");

        let load = builder.get_label("load").unwrap().as_node().unwrap();
        let beta_loop = builder.get_label("beta_loop").unwrap().as_node().unwrap();
        beta = Some((beta_in, beta_out, beta_loop));

        builder.transition(vec![beta_loop], vec![beta_in]);
        builder.transition(vec![load, empty], vec![full, beta_loop, semaphores[2].index()]);
    }

    TruckNetwork {
        initial,
        empty,
        full,
        alpha,
        beta,
        semaphores: semaphores.iter().map(|s| s.index()).collect(),
        network: builder.build()
    }
}

fn main() {
    let network = truck_network(2, true, true);

    let mut file = std::fs::File::create("target/exported.dot").expect("Open target/exported.dot");
    network.network.export_dot(&mut file);
    println!("Successfully wrote network into `target/exported.dot`");
}

#[test]
fn test_truck_network_unload() {
    for period in 2..5 {
        let network = truck_network(period, true, false);
        let graph = network.network.generate_graph();

        // Assert that only one semaphore is active at a time
        graph.assert_forall(|state| {
            let mut sum = 0u8;
            for &semaphore in &network.semaphores {
                sum += state[semaphore];
            }
            sum <= 1
        });

        // Assert that initial <= 1, empty <= 1, initial + empty <= 1
        graph.assert_forall(|state|
            state[network.initial] <= 1
            && state[network.empty] <= 1
            && state[network.initial] + state[network.empty] <= 1
        );

        let alpha_in = network.alpha.unwrap().0;
        let alpha_loop = network.alpha.unwrap().2;

        // Verify that we looped
        assert!(
            graph.reaches(|state| state[network.empty] == 1 && state[alpha_in] == 1).len() > 0,
            "No state where we looped back (empty+alpha_in) was reached!"
        );
        assert!(
            graph.reaches(|state| state[network.initial] == 1 && state[alpha_loop] == 1).len() > 0,
            "No state where we looped back (alpha_loop+initial) was reached!"
        );

        // Verify that we always reach the state with (empty + semaphores[1])
        graph.assert_always_reaches(|state|
            state[network.empty] == 1
            && state[network.semaphores[1]] == 1
        );
    }
}


#[test]
fn test_truck_network_load() {
    for period in 2..5 {
        let network = truck_network(period, true, true);
        let graph = network.network.generate_graph();

        // Assert that only one semaphore is active at a time
        graph.assert_forall(|state| {
            let mut sum = 0u8;
            for &semaphore in &network.semaphores {
                sum += state[semaphore];
            }
            sum <= 1
        });

        // Assert that initial <= 1, empty <= 1, full <= 1, initial + empty + full <= 1
        graph.assert_forall(|state|
            state[network.initial] <= 1
            && state[network.empty] <= 1
            && state[network.full] <= 1
            && state[network.initial] + state[network.empty] + state[network.full] <= 1
        );

        let alpha_in = network.alpha.unwrap().0;
        let alpha_loop = network.alpha.unwrap().2;

        let beta_in = network.beta.unwrap().0;
        let beta_loop = network.beta.unwrap().2;

        // Verify that we looped
        assert!(
            graph.reaches(|state| state[network.empty] == 1 && state[alpha_in] == 1).len() > 0,
            "No state where we looped back (empty+alpha_in) was reached!"
        );
        assert!(
            graph.reaches(|state| state[network.initial] == 1 && state[alpha_loop] == 1).len() > 0,
            "No state where we looped back (alpha_loop+initial) was reached!"
        );
        assert!(
            graph.reaches(|state| state[network.full] == 1 && state[beta_in] == 1).len() > 0,
            "No state where we looped back (full+beta_in) was reached!"
        );
        assert!(
            graph.reaches(|state| state[network.empty] == 1 && state[beta_loop] == 1).len() > 0,
            "No state where we looped back (beta_loop+empty) was reached!"
        );

        // Verify that we always reach the state with (full + semaphores[2])
        graph.assert_always_reaches(|state|
            state[network.full] == 1
            && state[network.semaphores[2]] == 1
        );
    }
}
