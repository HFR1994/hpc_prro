Throughout this paper the following notation is used:

- $t$ represents the current iteration number and $T$ is the maximum number of iterations;
- $P$ is the total population of Ravens and $F$ the number of features for each Raven;
- $X(t)$ is the position vector of ravens in iteration $t$;
- $R S(t)$ is the roosting site across all $T$;
- $D(t)$ is direction where the raven is moving on $t+1$;
- $L (t)$ Represent the leader, with the best location of source food (global);
- $F S (t)$ is the best local food source know to a raven. These will be replace by the current positions $X (t)$ if they yield better fitness values;
- $R (t)$ is the remaining distance to get the final destination represented as a scalar. Which can be $L (t)$ or $F S (t)$, see bellow.
- $U(a, b)$ denotes a random number drawn uniformly in the range $[a, b]$ based on PCG;
- $U B$ and $L B$ are the upper and lower bounds of the search space;
- $r_i$ are random numbers drawn from $U(0, 1)$;
- $R$ is the defined looking radius (in a hypersphere) from a specific position $X(t)$. Usually is equal to the upper bound $U B$;

== PCG Implementation

    PCG (Permuted Congruential Generator) is a family of simple, fast, space-efficient, and statistically good random number generators developed by O'Neill @pcg_paper. Unlike traditional linear congruential generators (LCGs), PCG applies a permutation function to the output of an LCG to improve statistical quality while maintaining computational efficiency.

    The basic PCG algorithm uses a linear congruential generator as its core:

    $ s_(t+1) = a times s_t + c mod m $

    where $s_t$ is the internal state, $a$ is the multiplier, $c$ is the increment, and $m$ is the modulus. The output is then passed through a permutation function that scrambles the bits to eliminate the statistical weaknesses inherent in LCGs. This permutation typically involves bit shifts, rotations, and XOR operations, which are computationally inexpensive but dramatically improve the randomness quality.

    PCG generators offer several advantages: they pass stringent statistical test suites (including TestU01's BigCrush), have small state sizes, are extremely fast, and provide strong statistical properties suitable for scientific simulations and optimization algorithms. In this implementation, PCG is used to generate the uniform random numbers $U(a, b)$ that drive the stochastic behavior of the Raven Roost Algorithm.

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

    The movement of ravens in the algorithm is defined by the position update equation $bold(p)_(i,t) = bold(p)_(i,t-1) + bold(d)_(i,t)$, where $bold(d)_(i,t)$ represents a random distance vector for raven $i$ at iteration $t$. The distance vector $bold(d)_(i,t)$ is constructed through two key steps in the implementation. A direction, and a random amplifier:
    
    $ bold(d)_(i,t) = bold(s)_(i,t) times D(t) $
     
    To mimic the real behavoir, $D(t)$ is the direction of each bird to find the best food source. At the start of each $t$, each raven decides if he is going to pursue his best food source point or follow the leader and scout the area.
    
    $
      arrow(d) = cases(
        L (t) - X(t) quad U(0, 1) <= 0.2,
        F S (t) - X(t) quad U(0, 1) > 0.2
      )
    $
         
    First we calculate $arrow(d)$ into a magnitud using norm:

    $ ||bold(v)|| = sqrt(sum_(i=1)^n (x_i)^2) $

    where $v$ is any given vector ($arrow(d)$ in this case) that holds the $n$ number of parameters (dimensions) in the search space. This is later used the computation of a unit vector using:

    $ D(t) = hat(bold(v)) = bold(v) / (||bold(v)||) $

    This vector represents the exploration direction for the current iteration, influenced by the previous movement patterns. 

    Second, we calculated a random modifier based on the remaining distance to the target location using *Euclidean distance* to get a scalar value between the final destination $F S (t)$ or $L (t)$ and the current position $X(t)$:

    $ d(bold(a), bold(b)) = sqrt(sum_(i=1)^n (a - b)^2) $

    $a$ and $b$ represent any vector of $n$ number of parameters (dimensions) in the search space. We multiply this distance to random step size $r_i$ to get fraction of this remaining distance using a uniform distribution value, giving the following formula:
    
    $ bold(s)_(i,t) = r_i times R (T) $ 
        
    This creates a Lévy flight-like behavior, where ravens take larger steps when they are far from the target and smaller steps as they approach to the target. The actual displacement vector is therefore $bold(d)_(i,t) = s_t times D(t)$, resulting in an adaptive movement strategy that balances exploration through randomness with exploitation through proximity-based step sizing.
    
== Lookout function

   As mentioned earlier, raven's move a fraction of the total remaining distance governed by $bold(d)_(i,t) = s_t times D(t)$. This so that each point they stop they can scout the area with a Hypersphere vision across $n$ dimensions. The looking radii ($r$) is governed by:  
   
   $ r = R / (3.6 times P^(1/F)) $
   
   When a raven finds a promising food source, he updates it and decides to continue or report it back to the rooster immediately.
   
    $
      bold(p)_(i,t+1) = cases(
        p_(i,t) + d_(i,t) quad U(0, 1) <= 0.1,
        R S(t) quad U(0, 1) > 0.1
      )
    $
   



