#let optimizations_content = [
  == Serial Optimizations

  === Early Stopping Mechanism

  The serial implementation includes an early stopping heuristic that terminates the lookout phase when a better solution is found. With probability $p = 0.1$, the algorithm breaks out of both the lookout and flight loops upon finding an improvement. This optimization reduces unnecessary computation when good solutions are discovered early, potentially saving

  $ (1 - p) dot.c L dot.c F $

  lookout operations per successful discovery, where $L$ is lookout steps and $F$ is flight steps.

  === Efficient Memory Access Patterns

  The implementation uses contiguous memory layouts for population data, enabling better cache locality, vectorization opportunities for modern compilers, and efficient memory operations. Population matrices are stored in row-major order with linear indexing:

  $ text("population")[i dot.c n + j] $

  for raven $i$ and dimension $j$, where $n$ is the number of features.

  === Bounds Checking

  A dedicated bounds checking function ensures solutions remain within feasible space without redundant checks. For each dimension $j$, positions are clamped:

  $ x_j = max(L_j, min(x_j, U_j)) $

  where $L_j$ and $U_j$ are the lower and upper bounds respectively.

  === Random Number Generation

  Uses PCG (Permuted Congruential Generator) algorithm @metaheuristics_survey which provides:
  - High statistical quality
  - Fast generation
  - Small state size
  - Good period length

  == Parallel-Specific Optimizations

  === Minimizing Communication

  The parallel implementation strategically places communication:
  - *Once per iteration*: Leader selection and broadcast
  - *Once at start*: Parameter broadcasting
  - *Once at end*: Timing reduction

  This design minimizes the frequency of collective operations, keeping most computation local.

  === Asynchronous Overlapping Potential

  While the current implementation uses blocking collectives, the structure allows future optimization through:
  - Non-blocking leader broadcast (`MPI_Ibcast`)
  - Overlapping computation of independent ravens with communication
  - Pipelined iteration execution

  === Local-First Processing

  Each process computes its local leader before global reduction, minimizing data transfer:
  - *Without local-first*: Transfer all fitness values ($P$ doubles)
  - *With local-first*: Transfer only local minimum (1 struct with rank and value)

  === Reduced Early Stopping in Parallel

  The parallel version conditionally disables early stopping when measuring speedup. When benchmarking mode is enabled, the probabilistic early termination is suppressed to ensure deterministic execution for fair performance comparisons across different process counts.

  === Memory Optimizations

  ==== Reusable Buffers

  The implementation uses reusable buffers for temporary computations:
  - `direction`, `n_candidate_position`, `prev_location`, `final_location`

  These are allocated once per process and reused across all iterations, avoiding repeated allocation overhead.

  ==== Structure Packing

  MPI communication of configuration uses byte-level broadcasting, transmitting the entire structure as a contiguous byte array. This avoids overhead of creating custom MPI datatypes for simple structs.

  == Algorithmic Optimizations

  === Follower Selection Strategy

  Followers are selected randomly with 20% of population. The number of followers is computed as:

  $ N_{text("followers")} = ceil(0.2 dot.c P - 1) $

  This balances exploration (non-followers search their own best) and exploitation (followers search near global leader).

  === Adaptive Step Sizing

  Step size is proportional to remaining distance. At each flight step $t$, the step size is:

  $ s_t = d_{text("remaining")} dot.c u $

  where $d_{text("remaining")} = ||bold(t)_i - bold(x)_(i,t)||$ is the distance to target and $u tilde cal(U)(0, 1)$. This provides large steps when far from target (exploration) and small steps when near target (exploitation).

  === Roosting Reset

  Ravens return to roosting site at the start of each iteration. For all ravens $i$:

  $ bold(x)_(i,0) arrow.l bold(r) $

  where $bold(r)$ is the roosting site. This reset mechanism prevents premature convergence, maintains diversity in search trajectories, and models natural raven behavior.

  == Performance Considerations

  === Computational Bottlenecks

  Profile analysis would likely show hotspots in:
  1. *Objective function evaluation* (`lookout_steps × flight_steps × pop_size` calls per iteration)
  2. *Distance calculations* (repeated square root operations)
  3. *Random number generation* (multiple calls per step)

  === Potential Further Optimizations

  Future improvements could include:
  - *Vectorization*: SIMD operations for distance calculations
  - *GPU acceleration*: Parallel evaluation of objective function
  - *Hybrid MPI+OpenMP*: Thread-level parallelism within each node
  - *Adaptive parameters*: Dynamic adjustment of flight_steps and lookout_steps based on convergence rate
  - *Communication aggregation*: Batch multiple MPI operations
]
