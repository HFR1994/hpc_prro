#import "@preview/cetz:0.3.2"

#let serial_content = [
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
    box(width: 100%, fill: rgb("#f5f5f5"), inset: 10pt)[
      ```
      Algorithm: Raven Roosting Optimization (Serial)
      Input: pop_size, features, iterations, flight_steps,
             lookout_steps, bounds, radius
      Output: Best solution found

      1. Initialize population from dataset
      2. Evaluate fitness for all ravens
      3. Set roosting_site as centroid
      4. current_leader ← argmin(fitness)
      5. followers ← select 20% of population (excluding leader)

      6. for iter = 1 to iterations do
      7.   for i = 1 to pop_size do
      8.     current_position[i] ← roosting_site
      9.
      10.    if is_follower[i] then
      11.      target ← random_point_near(leader, r_pcpt)
      12.    else
      13.      target ← food_source[i]
      14.    end if
      15.
      16.    for step = 1 to flight_steps do
      17.      direction ← normalize(target - current_position[i])
      18.      remaining ← distance(current_position[i], target)
      19.      step_size ← uniform(0, remaining)
      20.      current_position[i] ← current_position[i] +
      21.                             step_size × direction
      22.      enforce_bounds(current_position[i])
      23.
      24.      for lookout = 1 to lookout_steps do
      25.        candidate ← random_point_near(current_position[i], r_pcpt)
      26.        enforce_bounds(candidate)
      27.
      28.        if f(candidate) < fitness[i] then
      29.          food_source[i] ← candidate
      30.          fitness[i] ← f(candidate)
      31.
      32.          if random() < 0.1 then
      33.            break  // Early stopping
      34.          end if
      35.        end if
      36.      end for
      37.    end for
      38.  end for
      39.
      40.  current_leader ← argmin(fitness)
      41.  followers ← random_select(20%, excluding leader)
      42. end for
      43.
      44. return food_source[current_leader]
      ```
    ],
    caption: [Pseudocode for serial RRO implementation]
  )

  === Key Implementation Details

  ==== Initialization Phase

  The initialization phase loads the population from a CSV dataset, evaluates initial fitness values, and calculates the roosting site as a reference point. The looking radius parameter $r_{text("pcpt")}$ is computed based on the specified radius and search space bounds.

  ==== Distance Calculation

  Euclidean distance is computed between positions in the search space. For two positions $bold(a), bold(b) in bb(R)^n$, the distance is calculated as:

  $ d(bold(a), bold(b)) = sqrt(sum_(j=1)^n (b_j - a_j)^2) $

  ==== Leader Selection

  The leader is determined as the raven with the minimum fitness value, representing the best solution found so far:

  $ text("leader") = op("arg min")_(i in {1,...,P}) f(bold(x)_i) $

  ==== Flight Behavior

  Each raven performs a series of flight steps toward its target. The movement follows:

  $ bold(x)_(i,t+1) = bold(x)_(i,t) + s_t dot.c bold(hat(d)) $

  where

  $ bold(hat(d)) = frac(bold(t)_i - bold(x)_(i,t), ||bold(t)_i - bold(x)_(i,t)||) $

  is the normalized direction toward target $bold(t)_i$, and

  $ s_t tilde cal(U)(0, ||bold(t)_i - bold(x)_(i,t)||) $

  is the step size uniformly sampled from the remaining distance. At each step, the raven performs lookout steps to search for better food sources within radius $r_{text("pcpt")}$.

  === Memory Management

  The serial implementation carefully manages memory for:
  - Population matrices (`food_source`, `current_position`)
  - Fitness array
  - Temporary vectors (`direction`, `n_candidate_position`, `prev_location`, `final_location`)
  - Leader position and follower flags

  === Time Complexity

  For a population of size $P$, $N$ features, $I$ iterations, $F$ flight steps, and $L$ lookout steps:

  - *Overall*: $O(I dot.c P dot.c F dot.c L dot.c N)$
  - *Objective function evaluations*: $O(I dot.c P dot.c F dot.c L)$
]
