#ifndef _CRASH_HANDLE_H_
#define _CRASH_HANDLE_H_

void crash_handler_register(void);
void crash_handler(int signum);
void backtrace_print(void);

#endif // _CRASH_HANDLE_H_