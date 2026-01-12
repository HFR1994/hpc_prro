#ifndef RRA_PARALLEL_OBJECTIVE_FUNCTION_H
#define RRA_PARALLEL_OBJECTIVE_FUNCTION_H

double objective_function(const double *X, int features);
int set_leader(double *leader, const double *fitness, const double *X, int pop_size, int features);

#endif //RRA_PARALLEL_OBJECTIVE_FUNCTION_H