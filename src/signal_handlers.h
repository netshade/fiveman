#ifndef SIGNAL_HANDLERS_H
#define SIGNAL_HANDLERS_H

void handle_sigchld(int sig);
void handle_sigint(int sig);
void reset_signal_handlers();
void install_ignore_sigint_handler();
void install_signal_handlers();

#endif
