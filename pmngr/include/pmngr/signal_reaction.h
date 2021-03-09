#ifndef _SIGNAL_H
#define _SIGNAL_H

void signal_thread_init(int *exit_flag);
void signal_thread_destroy(void);
void set_signal_reactions(void);

#endif //_SIGNAL_H
