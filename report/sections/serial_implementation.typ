#import "@preview/lovelace:0.3.0": *

== Algorithm Structure

  The serial implementation follows a straightforward execution model where a single process handles the entire population of ravens.

=== Main Parameters

  The algorithm accepts the following parameters:
  - *Dataset Path*: CSV file containing initial population positions (NxM matrix)
  - *Lower/Upper Bounds*: Search space boundaries for each dimension
  - *Iterations*: Number of complete optimization cycles
  - *Flight Steps*: Number of steps each raven takes toward its destination
  - *Lookout Steps*: Number of exploratory searches per flight step
  - *Radius*: Looking radius for exploratory searches ($r_{text("pcpt")}$)

=== Core Algorithm Pseudocode

  #figure(
    pseudocode-list[
      + *Algorithm:* Raven Roosting Optimization (Serial)
      +
      + Initialize population with clamping
      + Evaluate best food source with Objective Function
      + Set roosting site and current leader
      + Define followers (excluding leader)
      +
      + *for* t= 1 *to* iterations *do*
        + *for* n = 1 *to* pop_size *do*
          + Set it's current position to the roosting site
          + Determine it's destination based on if is he a follower          
          +
          + *for* step = 1 *to* flight steps *do*
            + Move to $bold(p)_(i,t+1)$ with clamping
            +
            + *for* lookout = 1 *to* lookout steps *do*
              + Pick a random spot with clamping <= radii
              +
              + *if* finds a better food source *then*
                + Set it as personal best
                +
                + *if* p < 0.1 *then*
                  + Stop early and go back to the roosting site
                + *end if*
              + *end if*
            + *end for*
          + *end for*
        + *end for*
        +
        + Set current new leader
        + Define followers (excluding leader)
      + *end for*
      +
      + *return* current leader
    ],
    caption: [Pseudocode for serial RRO implementation]
  )

=== Key Implementation Details

==== Leader Selection

  The leader is determined as the raven with the minimum fitness value, representing the best solution found so far:

  $ text("leader") = op("min") (F S (t)) $

=== Time Complexity

  For a iteration of size $I$, population of size $P$ with $F$ features. Each on taking $F$ flight steps and $L$ lookout steps:

  - *Overall*: $O(I dot.c P dot.c F dot.c F dot.c L)$
  - *Objective function evaluations*: $O(I dot.c P dot.c F dot.c L)$

  Note: The early stopping mechanism (p < 0.1) can reduce the number of objective function evaluations by terminating lookout steps prematurely when a significantly better solution is found, potentially reducing evaluations by up to 10% in practice.
