#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <us_network.h>
#include <us_test.h>

static bool callback_dropped(us_connection c)
{
	printf("Connection on %i dropped\n", c->fd);
	
	return true;
}

static bool callback_message(us_message m)
{
	printf("%s\n", m->data->string);
	
	return true;
}

int main(int argc, char *argv[])
{
	
	{
		us_job_queue jq = us_job_queue_create();

		us_assert(us_job_queue_add_workers(jq, 2), "Failed to add workers");
	
		us_event_set es = us_event_set_create(jq);
	
		us_assert(es != NULL, "Creating us_event_set failed");

		us_event_set_start(es);

		// try connecting to obsidian for this test (using ipv4)	
		us_connection c = us_connection_create();
		us_assert(c != NULL, "Failed to create new connection");
	
		us_connection_type ct = us_connection_type_create(true, false);
	
		us_assert(us_connection_port_addr(c, "scottnz.com", 80, ct, callback_dropped, callback_message, false, es), "Failed to make connection to scottnz.com (pref IPv4)");
	
		us_assert(us_connection_type_destroy(ct), "failed to destroy connection type");
	
		// create a message
		us_string_length sl = us_string_length_create(strdup("GET /\n\n"), 8);
	
		us_assert(us_connection_send(c, sl, 0), "Failed to send GET / to scottnz.com");
	
		us_assert(us_string_length_destroy(sl), "Failed to destroy GET sl");
	
		sleep(10);
	
		if(!us_event_set_unref(es)) {}
	}

	return 0;
}
