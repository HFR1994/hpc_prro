#ifndef RRA_SERIAL_OBJECTIVE_FUNCTION_H
#define RRA_SERIAL_OBJECTIVE_FUNCTION_H

void objective_function(const double *X, double *fitness, int pop_size, int features);
int set_leader(double *leader, const double *fitness, const double *X, int pop_size, int features);

#endif //RRA_SERIAL_OBJECTIVE_FUNCTION_H