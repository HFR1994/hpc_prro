#import "@preview/note-me:0.6.0": *

== Experimental Setup

    To assess the scalability of the parallel Raven Roosting Algorithm implementations, a series of experiments where conducted on the HPC cluster. 
    
== SpeedUp

    The first implementation looked to asses the speedup using a single dataset and multiple CPU counts (1, 2, 4, 16, 32, and 64) and thread count (1,2,4).
    A unified RAM configuration (4GB) was used and two different placement strategies *pack:excl* and *scatter:excl*.
    
    #v(0.3cm)
    
    #note[
    #set par(first-line-indent: 0pt)
    The pack:excl consolidates processes onto the minimum number of nodes possible, whereas scatter:excl distributes chunks across separate nodes. Both strategies ensure exclusive node access during execution.
    ]
       
    #v(0.3cm)

    For this implementation, conditions such as early stop where remove to keep data consitency. 
    
=== SpeedUp Evolution

    A total of 5 experiments where executed in this phase across three different trials to ensure reproducibility. To fully take advantage and understand how each module contributes to the speedup, the following phases where tested:
    
    1. Serial Implementation used for baseline (Serial Implementation)
    2. Initial implementation of MPI, basic data handling (Trial 1)
    3. Parallelize CSV ingestion and boundary logic and added MPI structs for data handling. Introduced a global, local struct division (Trial 2)
    4. Final full MPI implementation (Trial 3)
    5. OpenMP implementation (Trial 4)
        
=== SpeedUp Results and Discussion

    The following tables demostrate the results for each configuration (not all configuration are available) across each phase.
    
    #figure(
      table(
        columns: (auto, 1fr, 1fr),
        inset: 8pt,
        align: center + horizon,
        // Header and Index column styling
        fill: (x, y) => if y == 0 or x == 0 { gray.lighten(80%) },
        stroke: 0.5pt + gray,
        
        // Header Row
        table.header(
          [*NP*], [*Pack* \ _(Total Time)_], [*Scatter* \ _(Total Time)_],
        ),
    
        // Data Rows
        [*1*],  [195.00], [195.33],
        [*2*],  [192.00], [195.00],
        [*4*],  [191.33], [196.33],
        [*16*], [193.00], [210.00],
        [*32*], [210.33], [210.67],
      ),
      caption: [Average Total in Serial Implementation],
      kind: table,
    ) <serial>
    
    In this implementation the total time remains relatively flat (around 191–196 seconds) regardless of the number of processes. This concides with the implementation, since the process doesn't attempt to split up anything.
    
    #figure(
      table(
        columns: (auto, 1fr, 1fr, 1fr, 1fr),
        inset: 8pt,
        align: center + horizon,
        // Header styling
        fill: (x, y) => if y < 2 or x == 0 { gray.lighten(80%) },
        stroke: 0.5pt + gray,
        
        // Header Rows
        table.header(
          table.cell(rowspan: 2, align: center + horizon)[*NP*],
          table.cell(colspan: 2)[*Pack*],
          table.cell(colspan: 2)[*Scatter*],
          [*Total*], [*Comp*], [*Total*], [*Comp*],
        ),
    
        // Data Rows
        [*1*],   [235.72], [235.70], [213.17], [213.16],
        [*2*],   [214.21], [214.19], [179.50], [179.48],
        [*4*],   [214.61], [214.59], [190.69], [190.67],
        [*16*],  [242.63], [242.62], [214.25], [214.24],
        [*32*],  [245.40], [245.39], [228.14], [228.12],
        [*64*],  [219.50], [219.48], [259.98], [259.97],
      ),
      caption: [Average Total and Computation Times in Trial 0],
      kind: table,
    ) <trial0>
    
    Results here represent worse timing that the baseline, since total times are consistently higher than the baseline (e.g., 235s vs 195s at np=1). 
    
    This is expected since of the added overhead. Intrestingly, the computation_time is almost identical to total_time, this suggest that the setup/teardown of the MPI environment is adding a fixed cost that the parallel logic.

    #figure(
      table(
        columns: (auto, 1fr, 1fr, 1fr, 1fr),
        inset: 8pt,
        align: center + horizon,
        // Header styling
        fill: (x, y) => if y < 2 or x == 0 { gray.lighten(80%) },
        stroke: 0.5pt + gray,
        
        // Multi-level Header
        table.header(
          table.cell(rowspan: 2, align: center + horizon)[*NP*],
          table.cell(colspan: 2)[*Pack*],
          table.cell(colspan: 2)[*Scatter*],
          [*Total*], [*Comp*], [*Total*], [*Comp*],
        ),
    
        // Data Rows
        [*1*],  [240.53], [240.52], [214.07], [214.04],
        [*2*],  [214.73], [214.71], [217.26], [217.25],
        [*4*],  [200.80], [200.77], [223.41], [223.41],
        [*16*], [211.60], [211.59], [235.09], [235.06],
        [*32*], [246.44], [246.43], [262.96], [262.92],
        [*64*], [244.29], [244.36], [261.30], [261.27],
      ),
      caption: [Average Total and Computation Times in Trial 1],
      kind: table,
    ) <trial1>
    
    This trial is similar to Trial 0 but begins to highlight the impact of parallelization. In here there is a high variability between pack and scatter. At np=32, pack is faster (246s) than scatter (262s), this results are expected since pack reduces the comunication overhead as where placeing nodes together. 
    
    Within this implementation, the algorithm is still bound by a serial bottleneck, since it's able to initialize in a distributed way but there is still a serial implementation when processing each row. 

    #figure(
      table(
        columns: (auto, 1fr, 1fr, 1fr, 1fr),
        inset: 8pt,
        align: center + horizon,
        // Header and Index column styling
        fill: (x, y) => if y < 2 or x == 0 { gray.lighten(80%) },
        stroke: 0.5pt + gray,
        
        // Multi-level Header
        table.header(
          table.cell(rowspan: 2, align: center + horizon)[*NP*],
          table.cell(colspan: 2)[*Pack*],
          table.cell(colspan: 2)[*Scatter*],
          [*Total*], [*Comp*], [*Total*], [*Comp*],
        ),
    
        // Data Rows
        [*1*],  [236.98], [236.95], [181.71], [181.69],
        [*2*],  [99.47],  [99.44],  [96.38],  [96.36],
        [*4*],  [47.09],  [47.07],  [46.37],  [46.34],
        [*16*], [13.17],  [13.15],  [13.63],  [13.61],
        [*32*], [6.79],   [6.77],   [7.25],   [7.23],
        [*64*], [3.24],   [3.19],   [4.12],   [4.09],
      ),
      caption: [Average Total and Computation Times in Trial 2],
      kind: table,
    ) <trial2>
    
    With this implementation there is a evident implementation of *Strong Scaling*. The execution time tends to drops linearly as np increases. When doubling the processes from np=2 (99s) to np=4 (47s) results are almost a perfect 2x speedup, proving the strong scaling exist. 
    
    With this implementation, the algorithm is 60x faster than the baseline (np=64).
    
    #figure(
      table(
        columns: (auto, 1fr, 1fr, 1fr, 1fr, 1fr),
        inset: 6pt,
        align: center + horizon,
        fill: (x, y) => if y < 2 or x < 2 { gray.lighten(80%) },
        stroke: 0.5pt + gray,
        
        // Header
        table.header(
          table.cell(rowspan: 2)[*NP*],
          table.cell(rowspan: 2)[*Threads*],
          table.cell(colspan: 2)[*Pack*],
          table.cell(colspan: 2)[*Scatter*],
          [*Total*], [*Comp*], [*Total*], [*Comp*],
        ),
    
        // NP 1
        table.cell(rowspan: 3)[*1*], [1], [253.76], [253.71], [215.73], [215.72],
        [2], [208.00], [207.91], [170.76], [170.75],
        [4], [188.53], [188.46], [139.80], [139.79],
    
        // NP 2
        table.cell(rowspan: 3)[*2*], [1], [113.98], [113.93], [108.75], [108.73],
        [2], [99.29], [99.23], [95.72], [95.71],
        [4], [101.32], [101.27], [106.97], [106.93],
    
        // NP 4
        table.cell(rowspan: 3)[*4*], [1], [58.09], [58.04], [59.66], [59.66],
        [2], [40.03], [40.00], [36.75], [36.74],
        [4], [28.49], [28.45], [26.51], [26.49],
    
        // NP 16
        table.cell(rowspan: 3)[*16*], [1], [16.82], [16.74], [16.29], [16.28],
        [2], [15.62], [15.54], [14.82], [14.79],
        [4], [11.59], [11.47], [20.67], [20.64],
    
        // NP 32
        table.cell(rowspan: 3)[*32*], [1], [7.48], [7.36], [9.19], [9.16],
        [2], [6.33], [6.25], [5.86], [5.78],
        [4], [15.48], [15.44], [35.44], [35.36],
    
        // NP 64
        table.cell(rowspan: 3)[*64*], [1], [4.52], [4.34], [5.12], [5.04],
        [2], [22.74], [22.66], [7.56], [7.50],
        [4], [84.55], [84.16], [4.64], [4.22],
    
      ),
      caption: [Average Total and Computation Times in Trial 3],
      kind: table,
    ) <trial3>
    
    Results with OpenMP, are scattered. At a lower np count, adding threads is highly effective, as denoted in np=1, 4 threads reduce the time from 253s to 188s. This tends to be the tendency until np=64.
    
    When this state is reached we start to see a inflexion point, were negative scaling starts to occur using multiple threads, specially in a pack configuration (84s with 4 threads vs 4.5s with 1 thread). 
    
=== Further Analysis

    Even though evidence exist that by implementing parallelism regardless of the implementation (MPI or OpenMPI) the results compared to the baseline are always better. However as suggested in the literature Strong Scaling Saturation @lange2013achieving can occur in instances where the sub-problems become so small that thread management and cache contention dominate the runtime. This can be seen better using the following graph.
    
    #figure(
      image("../assets/speedup_analysis.png", width: 100%),
      caption: [SpeedUp tendency],
    ) <plot_speedup> 
 
    As mentioned above, strong scaling occurs as the runtime falls as the number of compute resources (cores, MPI ranks, threads…) increases, however, this only true when $"np" = 1 → 4, "threads" <= 4$, since each iteration has a ~30% reduction when doubling cores.
    This is consistent except for np=2, threads=4, where a saturation start to show as we fall and have greater results compared with 2 thread case, most probably a bad run do to some network constraint. The behavoir then deviates back to $"np" = 16 → 32, "threads" <= 2$, where speedup still occurs even at a more inclined rate, but only last to for 1 iteration of threads since the tendency continues to grow at 1 thread which indicates OpenMP is not longer in use.
    
    This results showcase the limits of the execution. For the implementation of Raven Roosting Algorithm, the best configuration can be achieved with either 64 cores at 4 threads, further analysis have to be done for account for any systems flukes or with 32 CPUS and 2 threads. In there we can see a tendency that as the number of CPUS and threads increase so those the speedup. This a consistent indicator of Strong Scaling because the time tends to go down instead of remaining flat showing the code can successfully decompose a fixed problem.

== Parallelization Implementation

    Now that we have proven that the code is able to achieve Strong Scaling. The second implementation asseses parallelization prioritizing a more diverse dataset (256, 512, 1024 and 2048) and a simpler configuration of CPU's (16, 32, 64, 128) and thread count (2,4,6). 
    Conditions such as memory or placement location where kept the same.
    
    While measuring conditions can be different since early stopping mechanisms are in place, the purpose of the experiment is to demostrate weak scalibility since strong scalibility.

=== Parallelization Implementation Evolution

    Two implementation where tested one with the full MPI product and the other one with a full OpenMP implementation. With this implementation we prioritize convergence results, at a greater number of process we expect to see a much larger convergence rate.

=== Parallelization Implementation Results and Discussion

    The following convergence data denotates the time it takes to convergence agaisnt the total dataset size.    
    #figure(
      image("../assets/mpi_conv.png", width: 100%),
      caption: [Convergence of MPI processes],
    ) <mpi_conv> 
     
    As expect the greater the amount of data, the longer it takes to convergence, however the NP size does affect the time it takes to convergence. We can still reductions of almost 0.6 seconds between np=16 and np=128 meaning confirming again Strong Scaling. 
    
    However, if we analyze the implementation of OpenMP @openmp_conv, we can see a different result. When using a $"thread"=2$. 128 threads take a while to convergence, this behavoir seems to be constant regarless of of the total number of threads. Meaning that at np=128 we start to see saturation. 

    Intrestingly enough, conversations time are much larger when implementing threading. In the pure MPI implementation, the slowest convergence peaked at 0.7 seconds while the slowest on the OpenMP is at 26 minutes. This indicates a massive overhead in the OpenMP implementation, likely due to thread synchronization, memory locking, or "false sharing", however it's hard to tell since the experiment conditions are not the same. 
    
    #figure(
      image("../assets/openmp_conv.png", width: 100%),
      caption: [Convergence of OpenMP processes],
    ) <openmp_conv> 
    
    In this configuration, the most stable graph is show to be a "2 threads", showing a generally expected trend where larger populations take longer to converge. In all the configurations, NP=32 demostrates to be consistent, yielding the most realiable results as it it remains the lowest when $"thread"=2$ and low on $"np"=16, 64$, suggesting that is the better balance when combining both tecnologies.
    
=== Further Analysis

    While the pure MPI implementation demonstrates weak scaling, mantaining constant execution times of 0.12s as population size and process count are increase. OpenMP implementation suffers from severe performance degradation, with execution times jumping between order of magnitudes. A possible analysis could be the solution is bottlenecked by thread management overhead that outperformes the parallel benefit, making pure MPI a better alternative.