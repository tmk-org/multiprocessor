/*
 * backtrace for signals by everyone.
 */

#ifndef _DCL_SIGNAL_H
#define _DCL_SIGNAL_H

#include <signal.h>
#include <locale.h>

void signal_thread_init(int *p_exit_flag);
void signal_thread_destroy(void);
void set_signal_reactions(void);

#endif //_DCL_SIGNAL_H
