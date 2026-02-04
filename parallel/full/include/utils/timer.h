//
// Created by vscode on 1/8/26.
//

#ifndef RRA_PARALLEL_TIMER_H
#define RRA_PARALLEL_TIMER_H

void start_timer();

// Log elapsed time since timer_start (or program start if timer_start wasn't called).
// If label is non-NULL it will be printed with the elapsed time.
void log_stage_time(const char *label);

double get_elapsed_time();

#endif //RRA_PARALLEL_TIMER_H