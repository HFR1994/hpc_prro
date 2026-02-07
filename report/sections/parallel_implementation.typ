#import "@preview/cetz:0.3.2"
#import "@preview/lovelace:0.3.0": *

= Parallelization Strategy with MPI

    The following extra terminilogy is introduced:
    
    - *$"WR"$*: An MPI process responsible for managing a subset of the total population.
    - *$"WS"$*: Number of total MPI processes.

    Using parallelization we are able to achieve *domain decomposition* @parallel_metaheuristics, where the problem is broken down into smaller tasks called ranks. Each *rank* ($"WR"$) is an MPI process, who is in charge of managing a subset of the total population. Ranks are list a list of numbers $bold(x) in [0, infinity]$.
    
    Using this approach we are able to achieve faster convergence and exploration of the search space, since the program can run multiple calculations in parallel. In contrast the serial needs to iterate each calculation sequentially, which can be time-consuming for large populations. During the implementation of this algorithm, the same aspects where taken into as if the program was running sequentially. 
    Each rank, recieves a set of global variables such as the *leader* ($L (t)$) that impact the algorithm in the same way as it was running sequentially.

=== MPI Data Distribution <mpi_data>

    As previously mentioned, each process handles a local subset of ravens:
        - Process 0 reads parameters and broadcasts to all processes
        - Population is divided among processes
        - Each process maintains local copies of: food sources, positions, fitness values
        - Global leader information is shared across all processes

    Where each process is divided using:
    #v(0.3cm)
    $ F S_("WR")(t) = P / "WS" $ 
    #v(0.3cm)
    $ "extra" = P mod "WS" $
    #v(0.3cm)
    $ 
      "number_of_rows" = cases(
        F S_("WR")(t) + 1 quad "WR" < "extra",
        F S_("WR")(t) quad "WR" >= "extra"
      )
    $

=== Parallel Algorithm Pseudocode

    #[
        #show emph: it => {
          text(blue, it.body)
        }
        
        The following algorithm in @parallel shows the approach used to run the algorithm in a parallel manner using MPI. In order to facilitate reading, statments in _this color_ symbolize that the process is shared across all $"WS"$.
        
        The lifecycle of MPI is defined in it's initialization and closing arguments. Inside the algorithm it can be assumed multiple calls to MPI are made to *Reduce*, *Scatter* and *Broadcast* the results of the global variables.   
        #figure(pseudocode-list(stroke: none, title: smallcaps[Raven Roosting Optimization (Parallel MPI)])[
          + MPI Initialization
          + Initialize local population with clamping
          + Evaluate local best food source with Objective Function
          + _Set roosting site and current leader_
          + _Define followers (excluding leader)_
          +
          + *for* iter = 1 *to* iterations *do*
            + *for* i = 1 *to* local_rows *do*
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
            + _Set current global leader_
            + _Define followers (excluding leader)_
          + *end for*
          +
          + *return* current leader
          + Close MPI
        ],
        caption: [Pseudocode for parallel MPI RRO implementation]
        ) <parallel>
    ]

=== Key Implementation Details

#v(0.3cm)

==== Leader Selection

    Finding the global leader requires collective communication. Each process first evaluates the quality of it's solutions locally and determines its local best candidate by computing the minimum fitness value (#link(<OB>)[Objective Function]):
    
    $ text("local_best")_p = op("min")("local_ravens") $
    
    Then a global reduction determines the overall minimum across all processes, followed by broadcasting the leader position from the owning process to all others. This requires $O(log space "WS")$ communication time for $"WS"$ processes.

#v(0.3cm)

==== Timing Reduction

To accurately measure the total execution time of the parallel algorithm, the maximum execution time across all participating processes is computed using a global reduction operation. Each process records its own elapsed execution time $T_p$ and the overall wall-clock time is then obtained as:

$ T_{text("max")} = max_(p in {0,...,"WS"-1}) T_p $

This determines the wall-clock time as the slowest process duration.

=== Scalability Considerations

Strong scalability and weak scalability describe how well a parallel algorithm performs as you change the number of processors, both with static or dynamic population sizes. This speedup is denotaed by Amdahl’s Law @Amdahl1967 where:
#v(0.3cm)
$ S(p) = frac{1}{\,s + \dfrac{1-s}{p}\,} $
#v(0.3cm)
Where $p$ denotes the number of parallel workers (processes, threads, or cores) that are employed to execute the program. The term $s$ represents the proportion of the total execution time that is strictly serial—that is, the part of the code that cannot be divided among multiple processors and must run on a single core.

#v(0.3cm)
*Strong Scaling*:

Assumuing a static problem size (i.e., population size and dimensionality) but the number of processes increases, the achievable speedup is constrained by several algorithm-specific factors:

- Collective communication overhead becomes more pronounced, particularly during global leader selection and the subsequent broadcast of the leader’s position vector. 
- Load imbalance may arise when the population cannot be evenly partitioned across processes, resulting in some ranks owning fewer individuals and spending proportionally more time waiting at synchronization points. 
- Global synchronization operations, such as barriers and collective reductions, introduce idle time that grows with the number of processes, further limiting strong scaling efficiency.

*Weak Scaling*:

Assuming a linear increment betwee the problem size and the number of processes can the computational workload should remain balanced as relative impact of communication overhead stays stable.
This allows the algorithm to preserve a high level of parallel efficiency, as most operations are performed locally and collective communications scale logarithmically with the number of processes.

= Parallelization Strategy with OpenMP

    MPI allows distribute memory into different process (ranks) allowing it to execute lower volumes of data so then a master process can join it back to together. However, further breakdown can be achieved using OpenMP.
    
    When an OpenMP instruction is specified using the *$"#pragma omp parallel"$* instruction. Iterations blocks or sections are broken down among team members according to a specified schedule. Each thread executes only the part of the loop that was mapped to it, so a single loop is “broken down” into as many independent chunks, this is partically usefull among independent members such as @thibault2007efficient:
    
    #[
        #figure(pseudocode-list(stroke: none, title: smallcaps[Parallel OpenMP Implementation])[
          + int seeds = malloc(nthreads \* sizeof(unsigned int));
          + \#pragma omp for
          + *for* iter = 1 *to* iterations *do*
            + int tid = omp_get_thread_num();
            + seeds[tid] = time(NULL) ^ (tid \* 0x9e3779b9); 
          + *}*
        ],
        caption: [OpenMP example to initialize seeds]
        )
    ]

    
=== OpenMP Data Distribution

    OpenMP doesn't segment information as MPI. Instead of physically diving data as shown in #link(<mpi_data>)[MPI Data distribution] it creates threas inside the same ejecution, by taking advantage of the of the number of threads inside each core. So for each instruction you parallelize under OpenMP, each thread is goign to executed as many "threads" define during the exeuction.
    
    Each chunk is can be divided equally using:
    #v(0.3cm)
    $ "chunk_size" = min("rows" / "threads", 1) $ 
    #v(0.3cm)
    
    However, we can do more complex scenarios can arrise. Using the *dynamic* scheduling we can provided smaller chunk sizes (ej: 5) and have the process call multiple times the queue to get continous segements of 5 chunks. This is partically useful when dealing with memory contrains as you can process less information in a given point in time. 


