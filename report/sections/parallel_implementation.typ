#import "@preview/cetz:0.3.2"

#let parallel_content = [
  == Parallelization Strategy

  The parallel implementation in the `parallel/` directory uses MPI @mpi_standard to distribute the raven population across multiple processes. The key parallelization approach is *domain decomposition* @parallel_metaheuristics, where each MPI process manages a subset of the total population.

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
    box(width: 100%, fill: rgb("#f5f5f5"), inset: 10pt)[
      ```
      Algorithm: Raven Roosting Optimization (Parallel MPI)
      Input: Same as serial + MPI context
      Output: Best solution found

      1. MPI_Init()
      2. rank ← MPI_Comm_rank()
      3. size ← MPI_Comm_size()

      4. if rank == 0 then
      5.   Parse command-line arguments
      6. end if
      7. MPI_Bcast(parameters, root=0)

      8. local_rows ← pop_size / size
      9. Initialize local population segment
      10. Evaluate local fitness values
      11. Set roosting_site (shared across all processes)

      12. MPI_Barrier()  // Synchronize before timing
      13.
      14. // Find global leader using MPI_Allreduce
      15. local_best ← argmin(local_fitness)
      16. global_best ← MPI_Allreduce(local_best, op=MINLOC)
      17. MPI_Bcast(leader_position, root=global_best.rank)
      18.
      19. // Determine followers (distributed)
      20. num_local_followers ← 0.2 × local_rows
      21. local_followers ← random_select(num_local_followers)
      22.
      23. for iter = 1 to iterations do
      24.   for i = 1 to local_rows do
      25.     // Same flight logic as serial version
      26.     current_position[i] ← roosting_site
      27.
      28.     if is_follower[i] then
      29.       target ← random_point_near(leader, r_pcpt)
      30.     else
      31.       target ← food_source[i]
      32.     end if
      33.
      34.     for step = 1 to flight_steps do
      35.       direction ← normalize(target - current_position[i])
      36.       remaining ← distance(current_position[i], target)
      37.       step_size ← uniform(0, remaining)
      38.       current_position[i] ← current_position[i] +
      39.                              step_size × direction
      40.       enforce_bounds(current_position[i])
      41.
      42.       for lookout = 1 to lookout_steps do
      43.         candidate ← random_point_near(current_position[i],
      44.                                        r_pcpt)
      45.         enforce_bounds(candidate)
      46.
      47.         if f(candidate) < fitness[i] then
      48.           food_source[i] ← candidate
      49.           fitness[i] ← f(candidate)
      50.         end if
      51.       end for
      52.     end for
      53.   end for
      54.
      55.   // Update global leader
      56.   local_best ← argmin(local_fitness)
      57.   global_best ← MPI_Allreduce(local_best, op=MINLOC)
      58.   MPI_Bcast(leader_position, root=global_best.rank)
      59.
      60.   // Reshuffle followers locally
      61.   local_followers ← random_select(num_local_followers)
      62. end for
      63.
      64. // Gather timing statistics
      65. local_time ← MPI_Wtime() - start_time
      66. max_time ← MPI_Reduce(local_time, op=MPI_MAX, root=0)
      67.
      68. MPI_Finalize()
      69. return food_source[global_best.local_index] on root
      ```
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
]
