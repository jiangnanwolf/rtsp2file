#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <cxxabi.h>

#include <iostream>
#include <string>

#include "crash_handler.h"
#include "global.h"

using namespace std;

#define BT_BUF_SIZE 100

void crash_handler_register(void)
{
	char self[4096] = {0};
	ssize_t len = readlink("/proc/self/exe", self, sizeof(self)-1);
	if (len != -1) {
        self[len] = '\0';
	}
    string selfpath(self);
	cout << "self: " << selfpath << endl;

    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    signal(SIGFPE, crash_handler);
    signal(SIGILL, crash_handler);
    signal(SIGBUS, crash_handler);
    signal(SIGPIPE, crash_handler);
}

void crash_handler(int signum)
{
    printf("Caught signal %d\n", signum);
    backtrace_print();
    exit(EXIT_FAILURE);
}

void backtrace_print(void)
{
    int nptrs;
    void *buffer[BT_BUF_SIZE];
    char **strings;

    nptrs = backtrace(buffer, BT_BUF_SIZE);
    printf("backtrace() returned %d addresses\n", nptrs);

    /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
        would produce similar output to the following: */

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }

    for (size_t j = 0; j < nptrs; j++) {
        Dl_info info;
        if (dladdr(buffer[j], &info) && info.dli_sname) {
            char *demangled = NULL;
            int status;
            demangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
            printf("%s\n", status == 0 ? demangled : info.dli_sname);
            free(demangled);
        } else {
            printf("%s\n", strings[j]);
        }
    }

    free(strings);
    g_queue.Quit();
    exit(EXIT_FAILURE);
}