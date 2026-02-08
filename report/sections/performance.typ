#import "@preview/note-me:0.6.0": *

== Experimental Setup

    To assess the scalability of the parallel Raven Roosting Algorithm implementations, a series of experiments where conducted on the HPC cluster. 
    
=== SpeedUp

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
    
    1. Serial Implementation used for baseline
    2. Initial implementation of MPI, basic data handling 
    3. Parallelize CSV ingestion and boundary logic and added MPI structs for data handling. Introduced a global, local struct division
    4. Final full MPI implementation
    5. OpenMP implementation
        
=== Parallelization Implementation

    The second implementation assesed the parallelization implementation prioritizing a more diverse dataset (256, 512, 1024 and 2048) and a simpler configuration of CPU's (16, 32, 64) and thread count (2,4,6). 
    Conditions such as memory or placement location where kept the same.
    
    While measuring conditions can be different since early stopping mechanisms are in place, the purpose of the experiment is to demostrate weak scalibility since strong scalibility.

=== Parallelization Implementation Evolution

    Two implementation where tested one with the full MPI product and the other one with a full OpenMP implementation.

== Results and Discussion 

    

