use petri_network::*;

fn main() {
    let network = PetriNetwork::new(
        vec![1, 0, 0, 0, 0],
        vec![
            PetriTransition::new(vec![0], vec![1]),
            PetriTransition::new(vec![0], vec![2]),
            PetriTransition::new(vec![1], vec![3]),
            PetriTransition::new(vec![2], vec![4]),
            PetriTransition::new(vec![3], vec![4]),
            PetriTransition::new(vec![4], vec![]),
        ]
    );

    assert_eq!(network.generate_graph(), PetriGraph::from([
        (vec![1, 0, 0, 0, 0], vec![vec![0, 1, 0, 0, 0], vec![0, 0, 1, 0, 0]]),
        (vec![0, 1, 0, 0, 0], vec![vec![0, 0, 0, 1, 0]]),
        (vec![0, 0, 1, 0, 0], vec![vec![0, 0, 0, 0, 1]]),
        (vec![0, 0, 0, 1, 0], vec![vec![0, 0, 0, 0, 1]]),
        (vec![0, 0, 0, 0, 1], vec![vec![0, 0, 0, 0, 0]]),
        (vec![0; 5], vec![vec![0; 5]]),
    ]));
}
