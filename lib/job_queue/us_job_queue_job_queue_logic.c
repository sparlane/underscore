#include <stdio.h>

/*! \brief Process events in the job queue
 * \param t The thread which we are
 * \returns false if something really bad happens
 */
static bool us_jq_processor_thread(us_thread t)
{
	us_job_queue jq = t->priv;
	while(1)
	{
		us_job j = NULL;
		if(!us_cond_jq_lock(jq->jobs_cond)) return NULL;
		// while there is something to do ...
		while((j = us_job_heap_remove(jq->pending)) != NULL)
		{
			if(!us_cond_jq_unlock(jq->jobs_cond)) return NULL;

			// call the job handler ...
			j->cb(j->data);
			// disconnect the priv data
			j->data = NULL;
			if(!us_job_destroy(j)) return NULL;

			if(!us_cond_jq_lock(jq->jobs_cond)) return NULL;
		}
		// wait til something interesting happens
		if(!us_cond_jq_wait(jq->jobs_cond)) return NULL;

		// unlock the cond
		if(!us_cond_jq_unlock(jq->jobs_cond)) return NULL;
	}
	return NULL;
}

bool us_job_queue_add_workers_s(us_job_queue o, int w)
{
	bool res = 1;
	// realloc
	void *old = o->handlers;
	o->handlers_size += w;
	o->handlers = realloc(o->handlers, sizeof(us_thread) * o->handlers_size);
	if(o->handlers == NULL)
	{
		o->handlers = old;
		o->handlers_size -= w;
		res = 0;
	}
	
	for(int i = (o->handlers_size-w); i < o->handlers_size; i++)
	{
		// TODO: check this worked
		o->handlers[i] = us_thread_create(0, o, us_jq_processor_thread);
		us_thread_start(o->handlers[i]);
	}
	return res;
}

bool us_job_queue_enqueue_s(us_job_queue o, us_job_queue_callback handler, void* data)
{
	bool res = true;
	us_job j = us_job_create(handler, data, time(NULL));
	res = (j != NULL);
	
	if(res)
	{
		if(!us_cond_jq_lock(o->jobs_cond))
		{
			printf("Locking conditional FAILED\n");
			res = false;
		}
	
		if(res)
		{
			// we need to palm this off to one of the receiving threads ...
			res = us_job_heap_insert(o->pending, time(NULL), j);
		}
		// signal them ...
		us_cond_jq_signal(o->jobs_cond, true);
		us_cond_jq_unlock(o->jobs_cond);
	}
	return res;
}
