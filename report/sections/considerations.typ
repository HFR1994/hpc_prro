== Considerations

== Dataset

    In order to evaluate the speedup of the implementation, a controlled dataset was used to measure both a baseline and an implementation difference. Additionally, additional computation-intensive datasets were used to test the parallelization limits. Each dataset is defined by a dynamic number of rows (population size) and fixed number of columns (feature dimensionality), except for the controlled set. All the tests mentioned in the following pages are tested with the same dataset configurations while varying MPI ranks and OpenMP threads.
    
    The following table shows the configurations used:
    
    #figure(
      block(width: 80%, table(
        columns: (1fr, 1fr, 1fr, 1fr),
        inset: 4pt,
        align: horizon,
        fill: (x, y) => if y == 0 { gray.lighten(80%) },
        [*Set ID*], [*Rows*], [*Columns*], [*Controlled?*],
        [1], [128], [100], [Yes],
        [2], [256], [200], [No],
        [3], [512], [200], [No],
        [4], [1024], [200], [No],
        [5], [2048], [200], [No]
      )),
      caption: [Configuration Table],
    )

== Efficient Memory Access Patterns

  The implementation uses a contiguous memory layout to enable better cache locality and efficient memory operations. Population matrices are stored in row-major order with linear indexing:

  $ text("population")[i dot.c n + j] $

  for raven $i$ and dimension $j$, where $n$ is the number of features.

== Reducing overhead by minimizing communication

  The parallel implementation is designed to minimize overhead by limiting communication to essential phases of the algorithm. By broadcasting parameters only during initialization and performing timing reductions at the end, it ensures minimum communication. 
  
  A "local-first" model is employed to reduce the volume of data transmitted across the network. Instead of transferring a complete set of fitness values from every process for global evaluation, each process first identifies its own local leader. This only requires a single reduction to communicate the result. This optimization reduces the data transfer and avoids local minima as the search space is more thoroughly examined compared to a global implementation where a single leader is followed.
  
== Memory Considerations

  The algorithm uses a list of matrices to store global and local information of ravens' whereabouts, food sources and fitness values. However, the total memory consumption is dominated by the two matrices, as they are dependent on their population and features. The following table shows the composition size:
  #v(0.3cm)
  #table(
    columns: (2.5fr, 2fr, 4fr),
    inset: 4pt,
    align: horizon,
    fill: (x, y) => if y == 0 { gray.lighten(50%) },
    
    table.header(
      [*Data Structure*], [*Size (Bytes)*], [*Algorithmic Role*]
    ),
  
    // --- Population Matrices ---
    table.cell(colspan: 3, fill: gray.lighten(90%))[*Population Matrices (Global State)*],
    
    [food_source], 
    [$"pop_size" times "features" times 8$], 
    [Primary coordinate with best food location per raven],
    
    [current_position], 
    [$"pop_size" times "features" times 8$], 
    [Raven's location after each iteration.],
  
    // --- Global Vectors ---
    table.cell(colspan: 3, fill: gray.lighten(90%))[*Global Vectors (Shared Reference)*],
    
    [leader], 
    [$"features" times 8$], 
    [Best raven position.],
    
    [roosting_site], 
    [$"features" times 8$], 
    [Convergence point where ravens gather.],
    
    [fitness], 
    [$"pop_size" times 8$], 
    [The objective scores to each raven.],
  
    // --- Auxiliary Search Vectors ---
    table.cell(colspan: 3, fill: gray.lighten(90%))[*Auxiliary Search Vectors (Scratchpad)*],
    
    [prev_location], 
    [$"features" times 8$], 
    [Sub-step location for each raven.],
    
    [final_location], 
    [$"features" times 8$], 
    [Final destionation they want to reach],
    
    [n\_candidate\_ position], 
    [$"features" times 8$], 
    [Step size direction correction.],
    
    [direction], 
    [$"features" times 8$], 
    [Unit vector for the next iterative step.],
  )
  #v(0.3cm)

  Each section has a specific role when parallelizing. *Global Vectors (Shared Reference)* are shared across MPI; instead of having `total_population_sizes` allocated, the array collapses into smaller `local_population_sizes` where only the dominant leader is shared. This means that the communication never exceeds $"features" times 8$ where 8 is the bytes required to send a double number.

  In contrast, OpenMP has to allocate separate *Auxiliary Search Vectors (Scratchpad)* as these memory references are not thread-safe. This yields a higher memory increase since you are allocating $p$ arrays instead of a single reusable instance.  

==== Structure Packing

  MPI communication of configuration uses byte-level broadcasting, transmitting the entire structure as a contiguous byte array. This avoids overhead of creating custom MPI datatypes for simple structs.