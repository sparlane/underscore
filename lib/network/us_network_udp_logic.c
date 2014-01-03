#include <string.h>
#include <stdio.h>
#include <us_network_statics.h>

static bool listen_udp_invalid(us_event e)
{
	// TODO: print or something here ...
	return true; // thats fine, we dont mind
}

static bool listen_udp_occured(us_event e)
{
	us_udp c = e->priv;
	// invalid connection
	if(c == NULL) return false;

	// try recieve a packet ...
	us_udp_message m = us_udp_recvfrom(c);

	if(m == NULL)
	{
		// TODO: destroy the connection
		return false;
	}
	
	// process the message
	if(c->message)
		c->message(m);
	// free the information
	if(!us_udp_message_destroy(m)) {}
	// done, success
	return true;
}

us_udp_message us_udp_recvfrom_s(us_udp o)
{
	void *buf = malloc(1500);
	unsigned int sa_len = us_net_inline_get_addrlen(o->conn_type);
	struct sockaddr *sa = malloc(sa_len);

	ssize_t len = recvfrom(o->fd, buf, 1500, 0, sa, &sa_len);
	if(len < 0)
	{
		free(buf);
		free(sa);
		return NULL;
	}

	us_udp_message m = us_udp_message_create(o, sa, us_string_length_create(buf, len));

	return m;
}

bool us_udp_begin_s(us_udp o, uint16_t port, us_connection_type conn_type, us_network_udp_message message, us_event_set es)
{
	bool res = 1;

	// create the sock_addr struct
	int sock_addr_len = 0;
	struct sockaddr *sock_addr;
	if(conn_type->IPv6)
	{
		// IPv6
		sock_addr_len = sizeof(struct sockaddr_in6);
		struct sockaddr_in6 *saddr6 = malloc(sock_addr_len);
		sock_addr = (struct sockaddr *)saddr6;
		memset(saddr6, 0, sock_addr_len);
		saddr6->sin6_family = AF_INET6;
		inet_pton(AF_INET6, "::", &saddr6->sin6_addr);
		saddr6->sin6_port = htons(port);
	}
	else
	{
		// IPv4
		sock_addr_len = sizeof(struct sockaddr_in);
		struct sockaddr_in *saddr = malloc(sock_addr_len);
		sock_addr = (struct sockaddr *)saddr;
		memset(saddr, 0, sock_addr_len);
		saddr->sin_family = AF_INET;
		saddr->sin_addr.s_addr = INADDR_ANY;
		saddr->sin_port = htons(port);
	}
	
	
	int fd;
	// create a new socket ...
	fd = socket(us_net_inline_get_AF(conn_type), SOCK_DGRAM, 0);
	if(fd < 0)
	{
		perror("socket");
		free(sock_addr);
		return NULL;
	}

	if(port != 0)
	{
		// bind to the port (only if its not 0)
		if((bind(fd, sock_addr, sock_addr_len)) < 0){
			perror("bind");
			close(fd);
			return NULL;
		}
	}
	
	int broadcast = 1;
	// enable broadcasting
	if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast) < 0)
	{
		perror("setsockopt (SO_BROADCAST)");
		exit(1);
	}

	
	o->fd = fd;
	o->message = message;
	o->conn_type = conn_type;
	
	// retrieve the port info
	o->port = us_net_inline_get_port(conn_type, sock_addr);
	
	// TODO: should check return values here
	us_event e = us_event_create(o->fd, listen_udp_occured, listen_udp_invalid);
	e->priv = o;
	us_event_set_add(es, e);

	free(sock_addr);

	return res;
}

bool us_udp_sendto_sa_s(us_udp o, us_string_length sl, struct sockaddr *sa, unsigned int sa_len)
{
	ssize_t len = sendto(o->fd, sl->string, sl->length, 0, sa, sa_len);
	if(len <= 0)
	{
		perror("sendto");
		return false;
	}
	
	return true;
}

bool us_udp_sendto_s(us_udp o, us_string_length sl, char* addr, int port)
{
	// create the sock_addr struct
	unsigned int sock_addr_len = 0;
	struct sockaddr *sock_addr;
	if(o->conn_type->IPv6)
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
	
	bool success = us_udp_sendto_sa_s(o, sl, sock_addr, sock_addr_len);
	
	free(sock_addr);
	return success;
}

bool us_udp_broadcast_s(us_udp o, us_string_length sl, int port)
{
	return us_udp_sendto(o, sl, "255.255.255.255", port);
}

