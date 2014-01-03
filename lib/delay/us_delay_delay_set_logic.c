#include <unistd.h>
#include <stdio.h>

bool us_delay_set_thread(us_thread t)
{
	us_delay_set ds = t->priv;
	while(1)
	{
		us_cond_delay_lock(ds->cond);
		while(ds->first) {
			us_delay d = ds->first;
			unsigned long long delay = ds->first->delay;
			ds->first->delay = (delay >> 1); // this should make things work 'on average'
			us_cond_delay_unlock(ds->cond);
			if(usleep(delay)!=0) { perror("usleep"); } // sleep for however long required
			us_cond_delay_lock(ds->cond);
			while(ds->first != d) {
				// run each that has already expired
				us_delay dt = ds->first;
				ds->first = ds->first->next;
				us_job_queue_enqueue(ds->jq, dt->handler, dt->priv);
				free(dt);
			}
			ds->first = ds->first->next;
			us_job_queue_enqueue(ds->jq, d->handler, d->priv);
			free(d); // done, so free it
		}
		us_cond_delay_wait(ds->cond);  // wait for new things to do ...
		us_cond_delay_unlock(ds->cond);
	}
	return true;
}

bool us_delay_set_start_s(us_delay_set o, us_job_queue jq)
{
	bool res = 1;
	o->edelay = us_thread_create(0, o, us_delay_set_thread);
	us_thread_start(o->edelay);
	return res;
}

bool us_delay_set_add_s(us_delay_set o, us_delay d)
{
	us_cond_delay_lock(o->cond);
	
	// find where to place it ...
	if(o->first == NULL)
	{ // simple case
		o->first = d;
		us_cond_delay_signal(o->cond, false);
	}
	else if(o->first->delay > d->delay)
	{
		d->next = o->first;
		o->first->delay -= d->delay;
		o->first = d;
	} else {
		unsigned long long td = 0;
		us_delay s = o->first, t = o->first;
		while(t != NULL)
		{
			if((td + t->delay) > d->delay) break;
			td += t->delay;
			s = t;
			t = t->next;
		}
		s->next = d;
		d->next = t;
		d->delay -= td;
		if(t != NULL) t->delay -= d->delay;
	}

	us_cond_delay_unlock(o->cond);
	return true;
}

