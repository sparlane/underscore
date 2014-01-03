#include <poll.h>
#include <stdio.h>
#include <unistd.h>

/*! \brief A struct pollfd and its size
 */
struct pollfd_set
{
	struct pollfd *fds; /*!< the fd's to poll */
	nfds_t count; /*!< Count of fd's */
	short int events; /*!< The value to set pollfd.event to */
};

/*! \brief Process an event
 * \param p The event to process
 * \returns false if something really bad happens
 */
bool us_event_processor(void *data)
{
	us_event e = (us_event)data;
	if(e)
	{
		printf("Event on %i (%x)\n", e->fd, e->event);
		// call the event handler ...
		if((e->event & POLLNVAL) && e->invalid(e))
		{
			us_event_set_add(e->es, e);
		}
		else if((!(e->event & POLLNVAL)) && e->occured(e))
		{
			us_event_set_add(e->es, e);
		}
		else
		{
			// close the fd
			close(e->fd);
			// destroy it
			if(!us_event_unref(e)) {}
		}
	}
	return true;
}

static bool pollfd_add_event(us_event e, void *pass)
{
	struct pollfd_set *s = (struct pollfd_set *)pass;
	void *old = s->fds;
	s->fds = realloc(s->fds, sizeof(struct pollfd) * (s->count + 1));
	if(s->fds == NULL)
	{
		s->fds = old;
		return false;
	}
	s->fds[s->count].fd = e->fd;
	s->fds[s->count].events = s->events;
	s->fds[s->count].revents = 0;
	s->count++;
	return true;
}

/*! \brief Collect events for the es pending queue
 * \param es The event set to collect events for
 * \returns false if something really bad happens
 */
bool us_event_collector(us_event_set es)
{
	struct pollfd_set *set = malloc(sizeof(struct pollfd_set));
	set->fds = NULL;
	set->count = 0;
	set->events = POLLIN | POLLPRI;

	while(1)
	{
		if(us_event_set_updated_get(es))
		{
			set->count = 0;
			// TODO: check return value, and do something about it
			us_event_set_bst_foreach(es->all, pollfd_add_event, set);
			us_event_set_updated_set(es, false);
		}
		// TODO: do some checks about this 100 value, there might be something better ?
		int res = poll(set->fds, set->count, 100);
		if(res < 0)
		{
			// an error occured
			perror("us_event_collector: poll");
			// TODO check for EBADF, we can deal with this ...
		}
		else if(res > 0)
		{
			// some events occurred ...
			for(int i = 0; i < set->count; i++)
			{
				if(set->fds[i].revents)
				{
					us_event temp = us_event_create(set->fds[i].fd, NULL, NULL);
					us_event e = us_event_set_bst_remove(es->all, temp);
					if(!us_event_unref(temp)) {}
					// we have updated the set ...
					es->updated = true;
					if(e)
					{
						if(!us_job_queue_enqueue(es->jq, us_event_processor, e))
						{
							printf("Event on %i: Scheduling handler (FAILED)\n", set->fds[i].fd);
						}
						// they need to make sure it gets put back in ...
					}
				}
			}
		}
	}
	
	free(set->fds);
	free(set);
	return true;
}

static bool event_collector_thread(us_thread t)
{
	printf("Event collector THREAD\n");
	while(us_event_collector(t->priv)) ;
	return false;
}

bool us_event_set_start_s(us_event_set o)
{
	o->ewatcher = us_thread_create(0, o, event_collector_thread);
	us_thread_start(o->ewatcher);
	return true;
}

bool us_event_set_add_s(us_event_set o, us_event e)
{
	// add the event
	bool res = us_event_set_bst_insert(o->all, e);
	
	
	if(res)
	{
		e->es = o;
		// set updated
		o->updated = true;
	}

	return res;
}

