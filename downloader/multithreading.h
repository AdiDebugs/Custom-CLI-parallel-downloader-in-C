#ifndef __MULTITHREADING_H__
#define __MULTITHREADING_H__

int spawnthreads(int count, void *(*routine)(void *), threadinfo *workerinfo);
void *workerfuncntls(void *arg);
void *workerfunctls(void *arg);

#endif


