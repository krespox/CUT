#ifndef MAIN_H
#define MAIN_H

static void term(int signo);
void* reader(void* unused);
void* analyzer(void* unused);
void* printer(void* unused);
static void* watchdog(void* unused);

#endif
