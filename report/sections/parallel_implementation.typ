#import "@preview/cetz:0.3.2"
#import "@preview/lovelace:0.3.0": *

== Parallelization Strategy

    The parallel implementation in the `parallel/` directory uses MPI to distribute the raven population across multiple processes. The key parallelization approach is *domain decomposition* @parallel_metaheuristics, where each MPI process manages a subset of the total population.

=== MPI Context and Configuration

    The implementation initializes an MPI context structure that tracks process rank and world size, communicator, and global configuration parameters broadcast to all processes. The context is initialized at program startup to establish the distributed computing environment.

=== Data Distribution

    Each process handles a local subset of ravens:
        - Process 0 reads parameters and broadcasts to all processes
        - Population is divided among processes
        - Each process maintains local copies of: food sources, positions, fitness values
        - Global leader information is shared across all processes

=== Parallel Algorithm Pseudocode

    #figure(
    pseudocode-list[
      + *Algorithm:* Raven Roosting Optimization (Parallel MPI)
      + *Input:* Same as serial + MPI context
      + *Output:* Best solution found
      +
      + MPI_Init()
      + rank ← MPI_Comm_rank()
      + size ← MPI_Comm_size()
      +
      + *if* rank == 0 *then*
        + Parse command-line arguments
      + *end if*
      + MPI_Bcast(parameters, root=0)
      +
      + local_rows ← pop_size / size
      + Initialize local population segment
      + Evaluate local fitness values
      + Set roosting_site (shared across all processes)
      +
      + MPI_Barrier() // Synchronize before timing
      +
      + // Find global leader using MPI_Allreduce
      + local_best ← argmin(local_fitness)
      + global_best ← MPI_Allreduce(local_best, op=MINLOC)
      + MPI_Bcast(leader_position, root=global_best.rank)
      +
      + // Determine followers (distributed)
      + num_local_followers ← 0.2 × local_rows
      + local_followers ← random_select(num_local_followers)
      +
      + *for* iter = 1 *to* iterations *do*
        + *for* i = 1 *to* local_rows *do*
          + // Same flight logic as serial version
          + current_position[i] ← roosting_site
          +
          + *if* is_follower[i] *then*
            + target ← random_point_near(leader, r_pcpt)
          + *else*
            + target ← food_source[i]
          + *end if*
          +
          + *for* step = 1 *to* flight_steps *do*
            + direction ← normalize(target - current_position[i])
            + remaining ← distance(current_position[i], target)
            + step_size ← uniform(0, remaining)
            + current_position[i] ← current_position[i] + step_size × direction
            + enforce_bounds(current_position[i])
            +
            + *for* lookout = 1 *to* lookout_steps *do*
              + candidate ← random_point_near(current_position[i], r_pcpt)
              + enforce_bounds(candidate)
              +
              + *if* f(candidate) < fitness[i] *then*
                + food_source[i] ← candidate
                + fitness[i] ← f(candidate)
              + *end if*
            + *end for*
          + *end for*
        + *end for*
        +
        + // Update global leader
        + local_best ← argmin(local_fitness)
        + global_best ← MPI_Allreduce(local_best, op=MINLOC)
        + MPI_Bcast(leader_position, root=global_best.rank)
        +
        + // Reshuffle followers locally
        + local_followers ← random_select(num_local_followers)
      + *end for*
      +
      + // Gather timing statistics
      + local_time ← MPI_Wtime() - start_time
      + max_time ← MPI_Reduce(local_time, op=MPI_MAX, root=0)
      +
      + MPI_Finalize()
      + *return* food_source[global_best.local_index] on root
    ],
    caption: [Pseudocode for parallel MPI RRO implementation]
    )

=== Communication Patterns

    ==== Leader Selection

        Finding the global leader requires collective communication. Each process first finds its local minimum:
        
        $ text("local_best")_p = op("arg min")_(i in text("local_ravens")_p) f(bold(x)_i) $
        
        Then a global reduction determines the overall minimum across all processes, followed by broadcasting the leader position from the owning process to all others. This requires $O(log space k)$ communication time for $k$ processes.

==== Parameter Broadcasting

Configuration is broadcast from rank 0 to all processes. The global parameter structure is transmitted as a byte buffer, distributing algorithm configuration uniformly across the distributed system in $O(log space k)$ time.

==== Timing Reduction

Maximum execution time across all processes is gathered using a reduction operation:

$ T_{text("max")} = max_(p in {0,...,k-1}) T_p $

This determines the wall-clock time as the slowest process duration.

=== Synchronization Points

The parallel implementation uses several synchronization barriers:
- Before starting computation
- After initialization
- Before timing measurements

=== Load Balancing

Population distribution aims for balanced workload. Ravens are divided as evenly as possible with each process handling

$ P_{text("local")} = floor(P / k) $

ravens (where $P$ is total population and $k$ is number of processes). Each process handles the same number of iterations, and computational work scales linearly with local population size.

=== Scalability Considerations

*Strong Scaling*: For fixed problem size, speedup is limited by:
- Communication overhead (leader updates, broadcasts)
- Load imbalance if population doesn't divide evenly
- Synchronization barriers

*Weak Scaling*: Increasing problem size proportionally with processes maintains efficiency better, as communication-to-computation ratio remains favorable.

=== Random Number Generation

Each process uses an independent random number generator seeded with rank-dependent values. The seed for process $p$ is computed as

$ s_p = s_{text("base")} + p $

where $s_{text("base")}$ is the initial seed. This ensures reproducibility while maintaining statistical independence across processes.

