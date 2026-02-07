#import "@preview/charged-ieee:0.1.4": ieee

#show: ieee.with(
  title: [Raven Roosting Optimization with MPI: A Parallel Metaheuristic Approach],
  abstract: [
    This paper presents the implementation and parallelization of the Raven Roosting Optimization (RRO) algorithm @raven_optimization, a nature-inspired metaheuristic optimization technique based on the foraging behavior of ravens. We describe both a serial implementation in C and a parallel version using Message Passing Interface (MPI) for distributed memory systems. The algorithm simulates raven behavior including roosting, foraging, and social hierarchy with leader-follower dynamics. Our parallel implementation distributes the population across multiple processes, achieving significant speedup through domain decomposition and collective communication patterns. We present detailed pseudocode for both implementations, discuss key optimizations including early stopping mechanisms and efficient bound checking, and analyze the communication overhead and scalability characteristics of the MPI-based approach.
  ],
  authors: (
    (
      name: "Hector Carlos Flores Reynoso",
      department: [Department of Computer Science],
      organization: [University of Trento],
      location: [Student ID 266140],
      email: "hector.floresreynoso@unitn.it"
    ),
  ),
  index-terms: ("MPI", "Parallel Computing", "Metaheuristic Optimization", "Raven Roosting"),
  bibliography: bibliography("references.bib"),
)

#show list: it => [
  #v(0.3cm)
  #it
  #v(0.3cm)
]

#show heading: it => {
  if it.level >= 3 {
    set par(first-line-indent: 0pt)
    block(above: 0.3cm, below: 0.3cm, it)
  } else {
    it
  }
}

// Main content
= Introduction
#include "sections/introduction.typ"

// Formulas Discussion
= Terminology Discussion
#include "sections/terms.typ"

= Serial Implementation
#include "sections/serial_implementation.typ"


#include "sections/parallel_implementation.typ"

= Optimizations
#include "sections/optimizations.typ"

= Conclusion
#include "sections/conclusion.typ"