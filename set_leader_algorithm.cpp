int set_leader(prro_state_t * local, const prra_cfg_t global) {
  struct min_result result = { local->fitness[0], 0 };
  int i;

#pragma omp parallel for reduction(min_fit:result)
  for (i = 0; i < local->local_rows; i++) {
    if (local->fitness[i] < result.fitness) {
      result.fitness = local->fitness[i];
      result.index = i;
    }
  }

  log_main("Min fitness: %.15f at index %d", result.fitness, result.index);

  // Set leader to the position with the lowest fitness
#pragma omp parallel for num_threads(global.max_threads) if(global.features > 100)
  for (i = 0; i < global.features; i++) {
    local->leader[i] = local->food_source[result.index * global.features + i];
  }

  // Update best fitness tracking
  local->best_idx = result.index;
  local->best_fitness = local->fitness[result.index];

  return result.index;
}