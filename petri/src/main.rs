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

#[derive(Debug)]
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

#[derive(Debug)]
struct BoatNetwork {
    initial: usize,
    empty_alpha: usize,
    empty_beta: usize,
    full: usize,
    gone: usize,

    gamma: (usize, usize, usize, usize), // (gamma_in, gamma_out, gamma_premove, gamma_release)
    gamma_move_unload: Option<usize>,
    gamma_move_load: Option<usize>,

    semaphores: Vec<usize>,

    alpha: Option<(usize, usize, usize, usize)>, // (alpha_in, alpha_out, alpha_loop, unload)
    beta: Option<(usize, usize, usize, usize)>, // (beta_in, beta_out, beta_loop, load)

    network: PetriNetwork
}

fn boat_network(clock_period: usize, gamma_period: usize, unload: bool, load: bool) -> BoatNetwork {
    let mut builder = PetriBuilder::new();

    let initial = builder.node(1);
    let empty_alpha = builder.node(0);
    let empty_beta = builder.node(0);
    let full = builder.node(0);
    let gone = builder.node(0);

    let semaphores = vec![Semaphore::new(&mut builder, 1), Semaphore::new(&mut builder, 1)];

    let mut alpha = None;
    let mut beta = None;
    let mut gamma_move_unload = None;
    let mut gamma_move_load = None;

    // == γ's network ==

    let (gamma_in, gamma_out) = clock(&mut builder, gamma_period, false);
    builder.label(Label::Node(gamma_in), "gamma_in");
    builder.label(Label::Node(gamma_out), "gamma_out");

    let gamma_ready = builder.node_with_label(0, "gamma_ready");
    let gamma_premove = builder.node_with_label(0, "gamma_premove");
    let gamma_release = builder.node_with_label(0, "gamma_release");
    // Become ready
    builder.transition(vec![gamma_out], vec![gamma_ready]);
    // Go to the "premove" step
    builder.transition(vec![gamma_ready, semaphores[0].index(), semaphores[1].index()], vec![gamma_premove]);

    // Abort: loop
    builder.transition(vec![gamma_premove], vec![gamma_release, gamma_in]);
    builder.transition(vec![gamma_release], vec![semaphores[0].index(), semaphores[1].index()]);

    if unload {
        let (alpha_in, alpha_out) = clock(&mut builder, clock_period, true);
        builder.label(Label::Node(alpha_in), "alpha_in");
        builder.label(Label::Node(alpha_out), "alpha_out");

        // == α's network to move from `initial` to `empty_alpha` ==
        builder.begin_branch(alpha_out)
            .step()
            .label("alpha_ready")
            .step_with_mod(semaphores[0].p())
            .label("unload")
            .step()
            .label("alpha_loop");

        let unload = builder.get_label("unload").unwrap().as_node().unwrap();
        let alpha_loop = builder.get_label("alpha_loop").unwrap().as_node().unwrap();
        alpha = Some((alpha_in, alpha_out, alpha_loop, unload));

        builder.transition(vec![alpha_loop], vec![alpha_in, semaphores[0].index()]);
        builder.transition(vec![unload, initial], vec![alpha_loop, empty_alpha, gamma_in]);

        // == Instruct γ to move from empty_alpha to empty_beta ==
        let gamma_move = builder.node_with_label(0, "gamma_move_unload");
        gamma_move_unload = Some(gamma_move);

        builder.transition(vec![gamma_premove, empty_alpha], vec![gamma_move]);
        builder.transition(vec![gamma_move], vec![empty_beta, gamma_release]);
    }

    if load {
        let (beta_in, beta_out) = clock(&mut builder, clock_period, true);
        builder.label(Label::Node(beta_in), "beta_in");
        builder.label(Label::Node(beta_out), "beta_out");

        // == α's network to move from `initial` to `empty_beta` ==
        builder.begin_branch(beta_out)
            .step()
            .label("beta_ready")
            .step_with_mod(semaphores[1].p())
            .label("load")
            .step()
            .label("beta_loop");

        let load = builder.get_label("load").unwrap().as_node().unwrap();
        let beta_loop = builder.get_label("beta_loop").unwrap().as_node().unwrap();
        beta = Some((beta_in, beta_out, beta_loop, load));

        builder.transition(vec![beta_loop], vec![beta_in, semaphores[1].index()]);
        builder.transition(vec![load, empty_beta], vec![beta_loop, full, gamma_in]);

        // == Instruct γ to move from full to gone ==
        let gamma_move = builder.node_with_label(0, "gamma_move_load");
        gamma_move_load = Some(gamma_move);

        builder.transition(vec![gamma_premove, full], vec![gamma_move]);
        builder.transition(vec![gamma_move], vec![gone, gamma_release]);
    }

    BoatNetwork {
        initial,
        empty_alpha,
        empty_beta,
        full,
        gone,

        gamma: (gamma_in, gamma_out, gamma_premove, gamma_release),
        gamma_move_unload,
        gamma_move_load,

        semaphores: semaphores.into_iter().map(|s| s.index()).collect(),

        alpha,
        beta,

        network: builder.build(),
    }
}

fn main() {
    let network = truck_network(2, true, true);
    let graph = network.network.generate_graph();
    println!("{}", graph.iter().count());

    let mut file = std::fs::File::create("target/exported.dot").expect("Open target/exported.dot");
    graph.export_dot(&mut file);
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

#[test]
fn test_boat_network_unload() {
    for crane_period in 2..7 {
        for control_period in 2..7 {
            let network = boat_network(crane_period, control_period, true, false);
            let graph = network.network.generate_graph();

            // Assert that no semaphore got duplicated
            graph.assert_forall(|state| {
                for &semaphore in &network.semaphores {
                    if state[semaphore] > 1 {
                        return false
                    }
                }
                true
            });

            let (alpha_in, alpha_out, alpha_loop, unload) = network.alpha.unwrap();
            let (gamma_in, gamma_out, gamma_premove, gamma_release) = network.gamma;
            let gamma_move_unload = network.gamma_move_unload.unwrap();

            // Assert that `unload`, `gamma_premove`, `gamma_move_unload` and `gamma_release` are properly separated
            graph.assert_forall(|state| state[unload] + state[gamma_premove] + state[gamma_move_unload] + state[gamma_release] <= 1);

            // Verify that the different states `<= 1`
            graph.assert_forall(|state| state[network.initial] <= 1);
            graph.assert_forall(|state| state[network.empty_alpha] <= 1);
            graph.assert_forall(|state| state[network.empty_beta] <= 1);
            graph.assert_forall(|state| state[network.full] <= 1);
            graph.assert_forall(|state| state[network.gone] <= 1);

            // Verify that γ and α could loop before each other
            assert!(
                graph.reaches(|state| state[network.empty_alpha] == 1 && state[alpha_in] == 1).len() > 0,
                "No state where we looped back (empty+alpha_in) was reached!"
            );
            assert!(
                graph.reaches(|state| state[network.initial] == 1 && state[alpha_loop] == 1).len() > 0,
                "No state where we looped back (alpha_loop+initial) was reached!"
            );
            // γ's loop is harder because there isn't a node that indicates that it looped.
            // We know it does when `reaches(gamma_premove) == reaches(gamma_in)`: for either of them, a scenario where either are reached again is possible.
            // This silently tests the repeatability of γ: if `gamma_premove` cannot reach itself, then its subgraph would be different than `gamma_in`'s
            assert!(
                graph.reaches(|state| state[gamma_premove] == 1).is_subset(&graph.reaches(|state| state[gamma_in] == 1)),
                "The subgraph that reaches (gamma_premove) isn't a subset of the subgraph that reaches (gamma_in): gamma couldn't loop!"
            );
            assert!(
                graph.reaches(|state| state[gamma_premove] == 1).is_superset(&graph.reaches(|state| state[gamma_in] == 1)),
                "The subgraph that reaches (gamma_premove) isn't a superset of the subgraph that reaches (gamma_in): gamma couldn't loop!"
            );

            // Verify that we always reach the state with `empty_beta` + `semaphores[0]` + `semaphores[1]`
            graph.assert_always_reaches(|state|
                state[network.empty_beta] == 1
                && state[network.semaphores[0]] == 1
                && state[network.semaphores[1]] == 1
            );
        }
    }
}


#[test]
fn test_boat_network_load() {
    for crane_period in 2..7 {
        for control_period in 2..7 {
            let network = boat_network(crane_period, control_period, true, true);
            let graph = network.network.generate_graph();

            // Assert that no semaphore got duplicated
            graph.assert_forall(|state| {
                for &semaphore in &network.semaphores {
                    if state[semaphore] > 1 {
                        return false
                    }
                }
                true
            });

            let (alpha_in, alpha_out, alpha_loop, unload) = network.alpha.unwrap();
            let (beta_in, beta_out, beta_loop, load) = network.beta.unwrap();
            let (gamma_in, gamma_out, gamma_premove, gamma_release) = network.gamma;
            let gamma_move_unload = network.gamma_move_unload.unwrap();
            let gamma_move_load = network.gamma_move_load.unwrap();

            // Assert that `unload`, `load`, `gamma_premove`, `gamma_move_unload`, `gamma_move_load` and `gamma_release` are properly separated
            // Do note that `unlaod` and `load` can be reached safely at the same time
            graph.assert_forall(|state|
                state[unload]
                + state[gamma_premove]
                + state[gamma_move_unload]
                + state[gamma_move_load]
                + state[gamma_release]
                <= 1
            );
            graph.assert_forall(|state|
                state[load]
                + state[gamma_premove]
                + state[gamma_move_unload]
                + state[gamma_move_load]
                + state[gamma_release]
                <= 1
            );

            // Verify that the different states `<= 1`
            graph.assert_forall(|state| state[network.initial] <= 1);
            graph.assert_forall(|state| state[network.empty_alpha] <= 1);
            graph.assert_forall(|state| state[network.empty_beta] <= 1);
            graph.assert_forall(|state| state[network.full] <= 1);
            graph.assert_forall(|state| state[network.gone] <= 1);

            // Verify that γ and α could loop before each other
            assert!(
                graph.reaches(|state| state[network.empty_alpha] == 1 && state[alpha_in] == 1).len() > 0,
                "No state where we looped back (empty_alpha+alpha_in) was reached!"
            );
            assert!(
                graph.reaches(|state| state[network.initial] == 1 && state[alpha_loop] == 1).len() > 0,
                "No state where we looped back (alpha_loop+initial) was reached!"
            );

            assert!(
                graph.reaches(|state| state[network.empty_beta] == 1 && state[beta_in] == 1).len() > 0,
                "No state where we looped back (empty_beta+beta_in) was reached!"
            );
            assert!(
                graph.reaches(|state| state[network.full] == 1 && state[beta_loop] == 1).len() > 0,
                "No state where we looped back (beta_loop+full) was reached!"
            );

            // γ's loop is harder because there isn't a node that indicates that it looped.
            // We know it does when `reaches(gamma_premove) == reaches(gamma_in)`: for either of them, a scenario where either are reached again is possible.
            // This silently tests the repeatability of γ: if `gamma_premove` cannot reach itself, then its subgraph would be different than `gamma_in`'s
            assert!(
                graph.reaches(|state| state[gamma_premove] == 1).is_subset(&graph.reaches(|state| state[gamma_in] == 1)),
                "The subgraph that reaches (gamma_premove) isn't a subset of the subgraph that reaches (gamma_in): gamma couldn't loop!"
            );
            assert!(
                graph.reaches(|state| state[gamma_premove] == 1).is_superset(&graph.reaches(|state| state[gamma_in] == 1)),
                "The subgraph that reaches (gamma_premove) isn't a superset of the subgraph that reaches (gamma_in): gamma couldn't loop!"
            );

            // Verify that we always reach the state with `gone` + `semaphores[0]` + `semaphores[1]`
            graph.assert_always_reaches(|state|
                state[network.gone] == 1
                && state[network.semaphores[0]] == 1
                && state[network.semaphores[1]] == 1
            );
        }
    }
}
