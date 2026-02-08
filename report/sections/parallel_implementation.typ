#import "@preview/cetz:0.3.2"
#import "@preview/lovelace:0.3.0": *

= Parallelization Strategy with MPI

    The following extra terminology is introduced:
    
    - *$"WR"$*: An MPI process responsible for managing a subset of the total population.
    - *$"WS"$*: Number of total MPI processes.

    Using parallelization we are able to achieve *domain decomposition* @parallel_metaheuristics, where the problem is broken down into smaller tasks called ranks. Each *rank* ($"WR"$) is an MPI process, which is in charge of managing a subset of the total population. Ranks are a list of numbers $bold(x) in [0, infinity]$.

    Using this approach we are able to achieve faster convergence and exploration of the search space, since the program can run multiple calculations in parallel. In contrast, the serial version needs to iterate each calculation sequentially, which can be time-consuming for large populations. During the implementation of this algorithm, the same aspects were taken into account as if the program was running sequentially.
    Each rank receives a set of global variables such as the *leader* ($L (t)$) that impact the algorithm in the same way as it was running sequentially.

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

=== MPI Parallel Algorithm Pseudocode

    #[
        #show emph: it => {
          text(blue, it.body)
        }
        
        The following algorithm in @parallel shows the approach used to run the algorithm in a parallel manner using MPI. In order to facilitate reading, statements in _this color_ symbolize that the process is shared across all $"WS"$.

        The lifecycle of MPI is defined in its initialization and closing arguments. Inside the algorithm it can be assumed multiple calls to MPI are made to *Reduce*, *Scatter* and *Broadcast* the results of the global variables.   
        #figure(pseudocode-list(stroke: none, title: smallcaps[Raven Roosting Optimization (Parallel MPI)])[
          + MPI Initialization
          + Compute local indices
          + Initialize local population with clamping
          + Evaluate local best food source with Objective Function
          + _Set roosting site and current leader_
          + _Define followers (excluding leader)_
          +
          + *for* iter = 1 *to* iterations *do*
            + *for* i = 1 *to* local_rows *do*
              + Set its current position to the roosting site
              + Determine its destination based on if it is a follower
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
            + _Set current local and global leader_
            + _Define followers (excluding leader)_
          + *end for*
          +
          + *return* current leader
          + Close MPI
        ],
        caption: [Pseudocode for parallel MPI RRA implementation]
        ) <parallel>
    ]

=== MPI Key Implementation Details

#v(0.3cm)

==== Leader Selection

    Finding the global leader requires collective communication. Each process first evaluates the quality of its solutions locally and determines its local best candidate by computing the minimum fitness value (#link(<OB>)[Objective Function]):
    
    $ text("local_best")_p = op("min")("local_ravens") $
    
    Then a global reduction determines the overall minimum across all processes, followed by broadcasting the leader position from the owning process to all others. This requires $O(log space "WS")$ communication time for $"WS"$ processes.
    
    #v(0.3cm)

==== Timing Reduction

    To accurately measure the total execution time of the parallel algorithm, the maximum execution time across all participating processes is computed using a global reduction operation. Each process records its own elapsed execution time $T_p$ and the overall wall-clock time is then obtained as:
    
    $ T_{text("max")} = max_(p in {0,...,"WS"-1}) T_p $
    
    This determines the wall-clock time as the slowest process duration.

=== Scalability Considerations

    Strong scalability and weak scalability describe how well a parallel algorithm performs as you change the number of processors, both with static or dynamic population sizes. This speedup is denoted by Amdahl's Law @Amdahl1967 where:
    #v(0.3cm)
    $ S(p) = 1 / ((1 - p) + p / N) $
    #v(0.3cm)
    Where $p$ represents the the fraction of the program that is parallelizable. $N$ denotes the number of parallel workers (processes, threads, or cores) that are employed to execute the program and $S(N)$ be the speedup with $N$ processors.
    
    #v(0.3cm)
    *Strong Scaling*:
    #v(0.3cm)
    Assuming a static problem size (i.e., population size and dimensionality) but the number of processes increases, the achievable speedup is constrained by several algorithm-specific factors:
    
    - Collective communication overhead becomes more pronounced, particularly during global leader selection and the subsequent broadcast of the leader’s position vector. 
    - Load imbalance may arise when the population cannot be evenly partitioned across processes, resulting in some ranks owning fewer individuals and spending proportionally more time waiting at synchronization points. 
    - Global synchronization operations, such as barriers and collective reductions, introduce idle time that grows with the number of processes, further limiting strong scaling efficiency.
    
    *Weak Scaling*:
    #v(0.3cm)
    
    Assuming a linear increment between the problem size and the number of processes, the computational workload should remain balanced as the relative impact of communication overhead stays stable.
    This allows the algorithm to preserve a high level of parallel efficiency, as most operations are performed locally and collective communications scale logarithmically with the number of processes.

= Parallelization Strategy with OpenMP

    MPI allows distributing memory into different processes (ranks) allowing it to execute lower volumes of data so that a master process can join it back together. However, further breakdown can be achieved using OpenMP.

    When an OpenMP instruction is specified using the *$"#pragma omp parallel"$* instruction, iteration blocks or sections are broken down among team members according to a specified schedule. Each thread executes only the part of the loop that was mapped to it, so a single loop is "broken down" into as many independent chunks; this is particularly useful among independent members such as @thibault2007efficient:
    
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

    OpenMP doesn't segment information as MPI does. Instead of physically dividing data as shown in #link(<mpi_data>)[MPI Data distribution], it creates threads inside the same execution, by taking advantage of the number of threads inside each core. So for each instruction you parallelize under OpenMP, each thread is going to execute as many "threads" defined during the execution.
    
    Each chunk can be divided equally using:
    #v(0.3cm)
    $ "chunk_size" = min("rows" / "threads", 1) $ 
    #v(0.3cm)
    
    However, more complex scenarios can arise. Using the *dynamic* scheduling we can provide smaller chunk sizes (e.g., 5) and have the process call the queue multiple times to get continuous segments of 5 chunks. This is particularly useful when dealing with memory constraints as you can process less information at a given point in time. 

=== OpenMP Parallel Algorithm Pseudocode

     #[
        #show emph: it => {
          text(blue, it.body)
        }
                
        OpenMP statements are usually encapsulated around *for* and block variables. Only 3 locations were picked to implement OpenMP.
        #v(0.3cm)
        1. Evaluating the #link(<OB>)[Objective Function]
        2. Set roosting site
        3. Compute local rows
        #v(0.3cm)
        These locations were picked because they are isolated processes that can be computed individually. So the implementation of the parallelization doesn't affect other members.
        #v(0.3cm)
        The following algorithm in @openmp shows the approach used to run the algorithm in a parallel manner using OpenMP. In order to facilitate reading, statements in _this color_ symbolize the key structures where OpenMP was implemented. MPI structures are still taken into account, but omitted for visibility purposes.
        
        #figure(pseudocode-list(stroke: none, title: smallcaps[Raven Roosting Optimization (Parallel OpenMP)])[
          + Initialize local population with clamping
          + _Evaluate local best food source with Objective Function_
          + _Set roosting site and current leader_
          +
          + *for* iter = 1 *to* iterations *do*
            + _*for* i = 1 *to* local_rows *do*_
              + Set its current position to the roosting site
              + Determine its destination based on if it is a follower
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
            + _Set current local_ and global leader
            + Define followers (excluding leader)
          + *end for*
          +
          + *return* current leader
        ],
        caption: [Pseudocode for parallel OpenMP RRA implementation]
        ) <openmp>
     ]
      
=== OpenMP Key Implementation Details

    #v(0.3cm)

==== Instruction indications
    
    The decision to implement parallelization when computing local rows instead of parallelizing the entire iterations is due to the fact that *global* computations have to be done prior to updating the iteration.

    When starting a new iteration, the previous global leader's position has to be known. This makes it particularly difficult since you have data dependencies between iterations. Instead, an alternative is parallelizing the computation of the current iteration to find the best value for the Objective Function. This process is an excellent candidate for parallelization since you have to await for all "ravens" to compute the value, but each raven can achieve it individually. 
    
    #v(0.3cm)

==== Timing Reduction

    Similar to MPI, OpenMP can measure given
    #v(0.3cm)
    $ T_"OMP"(p) = t_"end" - t_"start" $
    #v(0.3cm)
    Compared to a specific "serial" block of code. However, similarly to MPI, the process is divided into execution processes (p), where each process computes the same work. This allows execution into multiple $R$ rounds according to a chunk size $C$ capacity. 
    Each thread $t$ executes a portion of the work during round $r$, with execution time $T_(p,r)$. The duration of a round is determined by the slowest participating thread:
    #v(0.3cm)
    $ T_r = max_(t in {0,...,p-1}) T_(p,r) $
    #v(0.3cm)
    So the total OpenMP execution time is the sum of all round durations:
    
    $ T_"OMP"(p, C) = sum_(r=0)^(R-1) max_(t in {0,...,p-1}) T_(p,r) $

=== Scalability Considerations

    This type of reduction is dependent on the total size of rows. Serial implementation can lead to better process timings given the overhead it takes to split and join threads. 
 
