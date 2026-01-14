#import "@preview/charged-ieee:0.1.4": ieee

#show: ieee.with(
  title: [Raven Roosting Optimization with MPI: A Parallel Metaheuristic Approach],
  abstract: [
    This paper presents the implementation and parallelization of the Raven Roosting Optimization (RRO) algorithm @raven_optimization, a nature-inspired metaheuristic optimization technique based on the foraging behavior of ravens. We describe both a serial implementation in C and a parallel version using Message Passing Interface (MPI) @mpi_standard for distributed memory systems. The algorithm simulates raven behavior including roosting, foraging, and social hierarchy with leader-follower dynamics. Our parallel implementation distributes the population across multiple processes, achieving significant speedup through domain decomposition and collective communication patterns. We present detailed pseudocode for both implementations, discuss key optimizations including early stopping mechanisms and efficient bound checking, and analyze the communication overhead and scalability characteristics of the MPI-based approach.
  ],
  authors: (
    (
      name: "Author Name",
      department: [Department of Computer Science],
      organization: [University of Trento],
      location: [Trento, Italy],
      email: "author@unitn.it"
    ),
  ),
  index-terms: ("MPI", "Parallel Computing", "Metaheuristic Optimization", "Raven Roosting"),
  bibliography: bibliography("references.bib"),
)

// Import section content
#import "sections/introduction.typ": introduction_content
#import "sections/serial_implementation.typ": serial_content
#import "sections/parallel_implementation.typ": parallel_content
#import "sections/optimizations.typ": optimizations_content
#import "sections/conclusion.typ": conclusion_content

// Main content
= Introduction
#introduction_content

= Serial Implementation
#serial_content

= Parallel Implementation with MPI
#parallel_content

= Optimizations
#optimizations_content

= Conclusion
#conclusion_content
