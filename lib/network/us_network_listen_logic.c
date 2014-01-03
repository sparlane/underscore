#include <string.h>
#include <stdio.h>
#include <us_network_statics.h>

static bool listener_invalid(us_event e)
{
	// TODO: print or something here ...
	return true; // thats fine, we dont mind
}

static bool listener_occured(us_event e)
{
	us_listen l = e->priv;
	printf("%s:%i\n", __FILE__, __LINE__);
	if(!l) return false;
	
	return us_listen_event_occurred(l);
}


bool us_listen_event_occurred_s(us_listen o)
{
	bool res = 1;

	// hopefully just accept and move along
	
	us_connection c = us_connection_create();
	res = (c != NULL);
	
	if(res)
	{
		res = us_connection_accept(c, o->fd, o->es, o->conn_type, o->dropped, o->message);
	}
	
	// if newconn exists ...
	if(res && o->newconn == NULL)
	{
		close(c->fd);
		if(!us_connection_unref(c)) {}
		res = 0; // no newconn defined ...
	}
	
	if(res)
	{
		// inherit this one from the listen socket
		c->us_connection = o->us_connection;
		c->breakchar = o->breakchar;
	
		// notify the user about this ...
		// TODO: check return value
		o->newconn(o, c);
	}

	return res;
}

bool us_listen_open_port_s(us_listen o, int port, char* addr, us_network_new_connection conn, us_connection_type conn_type, bool us_proto, us_event_set es, us_network_callback_dropped dropped, us_network_message message)
{
		bool res = 1;
	res = (es != NULL);
	
	if(res)
	{
		o->newconn = conn;
		o->conn_type = conn_type;
		o->us_connection = us_proto;
		o->es = es;
		o->dropped = dropped;
		o->message = message;
		o->breakchar = '\n';
	}
	
	// create the sock_addr struct
	unsigned int sock_addr_len = 0;
	struct sockaddr *sock_addr;
	if(res)
	{
		if(conn_type->IPv6)
		{	// IPv6
			sock_addr_len = sizeof(struct sockaddr_in6);
			struct sockaddr_in6 *saddr6 = malloc(sock_addr_len);
			sock_addr = (struct sockaddr *)saddr6;
			memset(saddr6, 0, sock_addr_len);
			saddr6->sin6_family = AF_INET6;
			inet_pton(AF_INET6, ((addr) ? addr : "::"), &saddr6->sin6_addr);
			saddr6->sin6_port = htons(port);
		}
		else
		{	// IPv4
			sock_addr_len = sizeof(struct sockaddr_in);
			struct sockaddr_in *saddr = malloc(sock_addr_len);
			sock_addr = (struct sockaddr *)saddr;
			memset(saddr, 0, sock_addr_len);
			saddr->sin_family = AF_INET;
			saddr->sin_addr.s_addr = inet_addr(((addr) ? addr : "0.0.0.0"));
			saddr->sin_port = htons(port);
		}
	}
	
	if(res)
	{
		// create the socket
		o->fd = socket(us_net_inline_get_AF(conn_type), SOCK_STREAM, 0);
		res = (o->fd != 0);
	}
	
	if(res)
	{
		// set it so re-binding is possible
		int v = 1;
		if((setsockopt(o->fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v))) < 0)
		{
			perror("setsockopt");
			res = 0;
		}
	}
	
	if(res)
	{
		// bind to the port
		if((bind(o->fd, sock_addr, sock_addr_len)) < 0)
		{
			perror("bind");
			res = 0;
		}
	}

	if(res)
	{
		// listen for incoming connections
		if((listen(o->fd, 10)) < 0)
		{
			perror("listen");
			res = 0;
		}
	}

	if(res)
	{
		// find out details of this socket
		if(getsockname(o->fd, sock_addr, &sock_addr_len) < 0)
		{
			perror("getsockname");
			close(o->fd);
			res = 0;
		}
	}
	
	if(res)
	{
		o->port = us_net_inline_get_port(conn_type, sock_addr);
	}
	
	free(sock_addr);

	// all going to plan, that all just worked ...
	// so now add this event to the event set and be happy ...
	us_event e = us_event_create(o->fd, listener_occured, listener_invalid);
	e->priv = o;
	us_event_set_add(es, e);
	printf("Listening on port %i (fd = %i)\n", o->port, e->fd);
	
	return res;
}

