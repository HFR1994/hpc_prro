#let introduction_content = [
  == Background

  Metaheuristic optimization algorithms have emerged as powerful tools for solving complex, NP-hard optimization problems where traditional exact methods become computationally intractable. These nature-inspired algorithms offer several key advantages: they require no gradient information, can escape local optima through stochastic mechanisms, and provide near-optimal solutions with significantly less computational effort than classical approaches. Their versatility and problem-agnostic nature have led to widespread applications across diverse domains including production scheduling, engineering design, and resource optimization @metaheuristics_survey. The Raven Roosting Optimization (RRO) algorithm represents a nature-inspired metaheuristic, mimicking the intelligent foraging behavior and social coordination of ravens.

  == Algorithm Overview

  The RRO algorithm is based on three key behavioral patterns observed in ravens @raven_optimization:

  - *Roosting Behavior*: Ravens return to a common roosting site where they rest and plan their next foraging expedition.
  - *Foraging Behavior*: Ravens explore the search space looking for food sources (optimal solutions) through a series of flight steps with exploratory lookouts.
  - *Social Hierarchy*: Ravens follow a leader-follower dynamic where the best-performing individual becomes the leader, and a subset of the population acts as followers who explore in the vicinity of the leader.

  In real life, ravens has shown a social behaviour mainly through communal roosting that functions as an information‑sharing system about food. At night, juvenile, non‑breeding ravens gather to successful foragers to scout for short‑lived carcasses as described by the Information Center Hypothesis. This social system is flexible: different individuals are knowledgeable at different times, and ravens combine socially acquired information with their own private knowledge when deciding whether to follow others or search independently @wright_raven_roosts_europe.


  == Problem Statement

  The algorithm aims to minimize an objective function $f(bold(x))$ where $bold(x) in bb(R)^n$ represents a position in the search space with $n$ dimensions (features). Each raven in the population represents a candidate solution, and the algorithm iteratively improves these solutions through exploration and exploitation phases.

  The algorithm maintains:
  - A population of ravens, each with a current position and discovered food source
  - Fitness values for each raven based on the objective function
  - A global leader (best solution found)
  - A roosting site that is kept throught each iteration.

  == Motivation for Parallelization

  Given the computationally intensive nature of the algorithm, especially with large populations and high-dimensional search spaces, parallelization using MPI becomes essential for:
  - Reducing execution time through distributed computation
  - Handling larger problem instances
  - Scaling across multiple compute nodes in HPC environments
]
