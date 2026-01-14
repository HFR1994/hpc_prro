#let introduction_content = [
  == Background

  Metaheuristic optimization algorithms @metaheuristics_survey inspired by natural phenomena have gained significant attention in computational optimization problems. The Raven Roosting Optimization (RRO) algorithm @raven_optimization mimics the intelligent foraging behavior of ravens, which are known for their problem-solving abilities and social coordination.

  == Algorithm Overview

  The RRO algorithm is based on three key behavioral patterns observed in ravens:

  - *Roosting Behavior*: Ravens return to a common roosting site where they rest and plan their next foraging expedition.
  - *Foraging Behavior*: Ravens explore the search space looking for food sources (optimal solutions) through a series of flight steps with exploratory lookouts.
  - *Social Hierarchy*: Ravens follow a leader-follower dynamic where the best-performing individual becomes the leader, and a subset of the population acts as followers who explore in the vicinity of the leader.

  == Problem Statement

  The algorithm aims to minimize an objective function $f(bold(x))$ where $bold(x) in bb(R)^n$ represents a position in the search space with $n$ dimensions (features). Each raven in the population represents a candidate solution, and the algorithm iteratively improves these solutions through exploration and exploitation phases.

  The algorithm maintains:
  - A population of ravens, each with a current position and discovered food source
  - Fitness values for each raven based on the objective function
  - A global leader (best solution found)
  - A roosting site (starting point for each iteration)

  == Motivation for Parallelization

  Given the computationally intensive nature of the algorithm, especially with large populations and high-dimensional search spaces, parallelization using MPI becomes essential for:
  - Reducing execution time through distributed computation
  - Handling larger problem instances
  - Scaling across multiple compute nodes in HPC environments
]
