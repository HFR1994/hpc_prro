#let terms_content = [
  == Objective Function

  The Griewank function is a multimodal optimization benchmark function widely used to test the performance of optimization algorithms. It is particularly challenging due to its many local minima that can trap optimization algorithms, while having a single global minimum. This function serves as the objective function in the Raven Roost Algorithm (RRA) implementation, providing a rigorous test case for evaluating the algorithm's exploration and exploitation capabilities @raven_optimization.

  The mathematical formulation of the Griewank function is:

  $ f(bold(x)) = 1 + 1/4000 sum_(i=1)^n x_i^2 - product_(i=1)^n cos(x_i / sqrt(i)) $

  where $bold(x) = (x_1, x_2, ..., x_n) in bb(R)^n$ represents a point in the n-dimensional search space.

  === Function Characteristics

  *Domain*: The function is typically evaluated on the hypercube $x_i in [-600, 600]$ for all dimensions $i = 1, ..., n$ @benchmark_functions_survey.

  *Global Minimum*: The unique global minimum occurs at $bold(x)^* = (0, 0, ..., 0)$ with $f(bold(x)^*) = 0$.

  *Multimodality*: The function exhibits many widespread local minima that are regularly distributed across the search space. The cosine component creates numerous local optima that can trap gradient-based methods, while the quadratic term creates a parabolic basin structure. This dual nature makes the Griewank function particularly effective for testing an algorithm's balance between exploration (escaping local minima) and exploitation (converging to the global optimum) @benchmark_functions_survey.

]
