#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <us_network.h>
#include <us_test.h>

static bool callback_new_conn(us_listen l, us_connection c)
{
	printf("New connection on %i connected as %i\n", l->fd, c->fd);
	
	us_string_length sl = us_string_length_create(strdup("SCOTT"), 6);
	
	us_connection_send(c, sl, 0);
	
	if(!us_string_length_destroy(sl)) {}
	
	return true;
}

static bool callback_dropped(us_connection c)
{
	printf("Connection on %i dropped\n", c->fd);
	
	return true;
}

static bool callback_message(us_message m)
{
	printf("Got new message on %i\n", m->connection->fd);
	
	printf("Message reads:%s\n\n", m->data->string);
	
	return true;
}

int main(int argc, char *argv[])
{
	us_job_queue jq = us_job_queue_create();

	us_assert(us_job_queue_add_workers(jq, 2), "Failed to add workers");
	
	us_event_set es = us_event_set_create(jq);
	
	us_assert(es != NULL, "Creating us_event_set failed");
	
	us_event_set_start(es);
	
	us_connection_type ct_ipv4 = us_connection_type_create(true, false);
	us_connection_type ct_any = us_connection_type_create(true, true);
	
	us_listen l = us_listen_create();
	us_assert(l != NULL, "Creating us_net_listen failed");
	
	us_assert(us_listen_open_port(l, 0, NULL, callback_new_conn, ct_ipv4 , true, es, callback_dropped, callback_message), "Failed to start listening");
	
	
	us_listen l2 = us_listen_create();
	us_assert(l2 != NULL, "Creating us_net_listen (IPv6) failed");
	
	us_assert(us_listen_open_port(l2, 0, NULL, callback_new_conn, ct_any , true, es, callback_dropped, callback_message), "Failed to open IPv6 listening socket");

	printf("l->port = %i\n", l->port);
	printf("l2->port = %i\n", l2->port);
	
	// try connecting to them ...
	us_connection conn = us_connection_create();
	us_connection conn2 = us_connection_create();
	
	
	us_assert(us_connection_port_ip(conn, "127.0.0.1", l->port, ct_ipv4 , callback_dropped, callback_message, true, es), "Failed to make connection to self (IPv4)");
	us_assert(us_connection_port_ip(conn2, "::1/128", l2->port, ct_any , callback_dropped, callback_message, true, es), "Failed to make connection to self (IPv6)");
	
	us_connection conng = us_connection_create();
	
	us_assert(us_connection_port_addr(conng, "ipv6.google.com", 80, ct_any , callback_dropped, callback_message, true, es), "Failed to make connection to ipv6.google.com (pref IPv6)");
	
	sleep(5);
	
	if(!us_event_set_unref(es)) {}
	return 0;
}
