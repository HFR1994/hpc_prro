


== Objective Function

    The Griewank function is a multimodal optimization benchmark function widely used to test the performance of optimization algorithms. It is particularly challenging due to its many local minima that can trap optimization algorithms, while having a single global minimum. This function serves as the objective function in the Raven Roost Algorithm (RRA) implementation, providing a rigorous test case for evaluating the algorithm's exploration and exploitation capabilities @raven_optimization.
    
    The mathematical formulation of the Griewank function is:
    
    $ f(bold(x)) = 1 + 1/4000 sum_(i=1)^n x_i^2 - product_(i=1)^n cos(x_i / sqrt(i)) $
    
    where $bold(x) = (x_1, x_2, ..., x_n) in bb(R)^n$ represents a point in the n-dimensional search space.

=== Function Characteristics

    - *Domain*: The function is typically evaluated on the hypercube $x_i in [-600, 600]$ for all dimensions $i = 1, ..., n$ @benchmark_functions_survey.
    - *Global Minimum*: The unique global minimum occurs at $bold(x)^* = (0, 0, ..., 0)$ with $f(bold(x)^*) = 0$.
    - *Multimodality*: The function exhibits many widespread local minima that are regularly distributed across the search space. The cosine component creates numerous local optima that can trap gradient-based methods, while the quadratic term creates a parabolic basin structure. This dual nature makes the Griewank function particularly effective for testing an algorithm's balance between exploration (escaping local minima) and exploitation (converging to the global optimum) @benchmark_functions_survey.

== Movement of ravens

    The movement of ravens in the algorithm is defined by the position update equation $bold(p)_(i,t) = bold(p)_(i,t-1) + bold(d)_(i,t)$, where $bold(d)_(i,t)$ represents the distance vector for raven $i$ at iteration $t$. The distance vector $bold(d)_(i,t)$ is constructed through two key steps in the implementation. First, the direction is determined by converting the previous direction vector ($bold(p)_(i,t-1)$) into a magnitud using norm:

    $ $||bold(v)|| = d(bold(x)) = sqrt(sum_(i=1)^n (x_i)^2) $

    where $v$ holds the $n$ number of parameters (dimensions) in the search space. This is later used in the computation of a unit vector scaled using:

    $ hat(bold(v)) = bold(v) / (||bold(v)||) $

    This vector represents the exploration direction for the current iteration, influenced by the previous movement patterns. 

    Second, the step size is calculated adaptively based on the remaining distance to the target location. The implementation computes the Euclidean distance between the current position and the best known location:

    $ d(bold(p)_i, bold(p)_"best") = sqrt(sum_(j=1)^n (p_(i,j) - p_("best",j))^2) $

    then generates a random step size $s_t$ as a fraction of this remaining distance using a uniform random value. This creates a Lévy flight-like behavior where ravens take larger steps when far from the target and smaller steps when approaching it. The actual displacement vector is therefore $bold(d)_(i,t) = "direction_unit_vector" times s_t$, resulting in an adaptive movement strategy that balances exploration through randomness with exploitation through proximity-based step sizing. The final position update follows $bold(p)_(i,t) = bold(p)_(i,t-1) + ("direction" times s_t)$, where the direction is influenced by historical movement and the magnitude adapts dynamically to the distance from the target.
    
    
    


