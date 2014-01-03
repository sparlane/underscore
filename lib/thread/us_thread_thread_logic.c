#include <stdio.h>

static void *jumper(void *data)
{
	fprintf(stderr, "Starting thread with data = %p\n", data);
	us_thread t = (us_thread)data;
	int a;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &a);
	t->func(t);
	return NULL;
}

bool us_thread_start_s(us_thread o)
{
	// now create the thread ...
	int res = pthread_create(o->t, NULL, jumper, o);

	if(res != 0)
	{
		perror("us_thread_start: failed to run pthread_create ");
	}
	
	return 1;
}
