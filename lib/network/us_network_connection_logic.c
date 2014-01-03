#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <zlib.h>
#include <us_network_statics.h>

/*! \struct us_net_mesg_s us_net.h
 * \brief A message sent on network
 * All members are network byte order
 */
struct us_net_mesg_s {
	uint32_t magic; /*!< Magic Code (0xBEEFBA5C) */
	uint32_t length; /*!< Length of the data */
	uint32_t lenpack; /*!< Packed length of data */
	uint32_t ID; /*!< Packet ID */
	uint32_t flags; /*!< Flags: NET_MESG_FLAG */
};

/*! The message is compressed */
#define NET_SEND_MESG_COMPRESSED 0x10
/*! The message is encrypted */
#define NET_SEND_MESG_ENCRYPTED 0x100

/*! Create a type name for ::us_net_mesg */
typedef struct us_net_mesg_s us_net_mesg;

static void *net_recv(int fd, size_t *length, unsigned char breakchar)
{
	if (length == NULL) return NULL;

	if(!fd)
	{
		fprintf(stderr, "Bad input to net_recv");
		return 0;
	}
	void *data = NULL;

	if(*length > 0)
	{
		data = malloc(*length);
		if(!data) return NULL;
		size_t recvd = 0;
		while(recvd < *length)
		{
			ssize_t res = recv(fd, (void *)((unsigned long)data + recvd), (*length - recvd), 0);
			if(res == 0) break;
			else if(res < 0) break;
			recvd += res;
		}
		if(recvd < *length){
			free(data);
			return NULL;
		}
	}
	else
	{
		// get the data until there is a breakchar ...
		char *cdata = NULL;
		size_t recvd = 0;
		while(1)
		{
			cdata = realloc(cdata, recvd + 2);
			ssize_t res = recv(fd, (void *)((unsigned long)cdata + recvd), 1, 0);
			if(res <= 0)
			{
				if(recvd == 0) 
				{
					free(cdata);
					return NULL;
				}
				break;
			}
			else
			{
				recvd++;
				if(cdata[recvd-1] == breakchar)
				{
					*length = recvd;
					cdata[recvd] = '\0';
					return cdata;
				}
			}
		}
	}

	return data;
}

static bool net_send(int fd, void *data, size_t length)
{
	/* recv() doesn't like zero-length messages. */
	if (length == 0)
	{
		fprintf(stderr, "Do not send zero-length packets\n");
		return false;
	}
	if(!fd || !data)
	{
		return false;
	}

	size_t sent = 0;
	int zcount = 0;
	while(sent < length)
	{
		ssize_t res = send(fd, (void *)((unsigned long)data + sent), (length - sent), 0);
		if(res == 0)
		{
			zcount++;
			if(zcount > 5) break;
		}
		else if(res < 0) break;
		sent += res;
	}
	if(sent < length){
		printf("sending %u bytes failed after only %u bytes: %s\n", (unsigned int)length, (unsigned int)sent, strerror(errno));
		return false;
	}

	return true;
}

static bool connect_invalid(us_event e)
{
	// TODO: print or something here ...
	return true; // thats fine, we dont mind
}

static bool connect_occured(us_event e)
{
	us_connection c = e->priv;
	// invalid connection
	if(c == NULL) return false;

	// try recieve a packet ...
	us_message m = us_connection_recv(c);

	if(m == NULL)
	{
		// notify the application
		if(c->dropped)
			c->dropped(c);
		// destroy the connection
		if(!us_connection_unref(c)) {}
		return false;
	}

	// Dont give-up our rights to it
	if(!us_connection_ref(c)) {}
 
	// process the message
	if(c->message)
		c->message(m);

	if(!us_message_destroy(m)) {}

	// done, success
	return true;
}


/*! \brief Get the address as a string
 * \param sock_addr sockaddr_in{,6} to get address of
 * \param conn_type what protocols to use/support ?
 * \return A string representing the address
 */
static char *us_net_get_addr(void *sock_addr, us_connection_type conn_type)
{
	char *str = malloc(30);
	if(conn_type->IPv6)
		inet_ntop(AF_INET6, &((struct sockaddr_in6 *)sock_addr)->sin6_addr , str , 30 );
	else if(conn_type->IPv4)  
		inet_ntop(AF_INET, &((struct sockaddr_in *)sock_addr)->sin_addr, str, 30);
	else *str = '\0';
	return str;
}	


us_message us_connection_recv_s(us_connection o)
{
	size_t length = 0;
	
	o->receiving = true;
	
	// create the message to return
	us_message m = us_message_create();
	if(m == NULL)
	{
		o->receiving = false;
		return NULL;
	}
	m->connection = o;
	
	if(o->us_connection)
	{
		// get the header
		size_t len = sizeof(us_net_mesg);
		us_net_mesg *h = net_recv(o->fd, &len, 0);
		if(h == NULL)
		{
			if(!us_message_destroy(m)) {}
			o->receiving = false;
			return NULL;
		}
		
		length = ntohl(h->lenpack);
	
		m->compressed = (ntohl(h->flags) & NET_SEND_MESG_COMPRESSED) == NET_SEND_MESG_COMPRESSED;
		m->encrypted = (ntohl(h->flags) & NET_SEND_MESG_ENCRYPTED) == NET_SEND_MESG_ENCRYPTED;
		m->ID = ntohl(h->ID);

		free(h);
	}	
	
	void *data = net_recv(o->fd, &length, o->breakchar);
	
	// check if any data was received
	if(data == NULL)
	{
		fprintf(stderr, "Got no data\n");
		if(!us_message_destroy(m)) {}
		o->receiving = false;
		return NULL;
	}

	if(o->us_connection)
	{
		// deal with encryption
		if(m->encrypted && o->recv_key)
		{
			// decrypt the data ...
			void *old = data;
			data = us_rc4_crypt_string(o->recv_key, old, length);
			if(data) free(old);
			else
			{
				data = old;
				m->encrypted = false;
			}
		}
		else
		{
			m->encrypted = false;
		}
	
		// deal with compression
		if(m->compressed)
		{
			Bytef *compressedData = (Bytef *)data;
			uLongf data_length = length;
			data = malloc(data_length);
			int res = uncompress((Bytef *)data, &data_length, (Bytef *)compressedData, length);
			if(res != Z_OK){
				fprintf(stderr, "Error with decompression (%i) of %u (clen is %u) bytes\n", res, (unsigned int)data_length, (unsigned int)length);
				free(compressedData);
				free(data);
				if(!us_message_destroy(m)) {}
				return NULL;
			}
			length = data_length;
			free(compressedData);
		}
	}
	
	m->data = us_string_length_create(data, length);

	o->receiving = false;
	return m;
}

bool us_connection_drop_s(us_connection o)
{
	bool res = true;
	if(close(o->fd) != 0)
	{
		fprintf(stderr, "close(%i): %s", o->fd, strerror(errno));
		res = false;
	}
	return res;
}

bool us_connection_port_ip_s(us_connection o, const char* addr, uint16_t port, us_connection_type conn_type, us_network_callback_dropped dropped, us_network_message message, bool us_proto, us_event_set es)
{
	// input checking ...
	if(port == 0 || addr == NULL) return 0;

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
		inet_pton(AF_INET6, addr, &saddr6->sin6_addr);
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
		saddr->sin_addr.s_addr = inet_addr(addr);
		saddr->sin_port = htons(port);
	}


	int fd;
	// create a new socket ...
	fd = socket(us_net_inline_get_AF(conn_type), SOCK_STREAM, 0);

	if(connect(fd, sock_addr, sock_addr_len) < 0)
	{
		perror("connect");
		char *str = us_net_get_addr(sock_addr, conn_type);

		printf("Connecting to %s (%s) %i FAILED\n", str, addr, port);

		free(str);
		free(sock_addr);
		return 0;
	}

	o->fd = fd;
	o->dropped = dropped;
	o->message = message;
	o->us_connection = us_proto;
	o->conn_type = conn_type;
	o->sock_addr = sock_addr;
	o->breakchar = '\n';

	unsigned int sa_len = us_net_inline_get_addrlen(conn_type);
	struct sockaddr *sa = malloc(sa_len);
	getsockname(o->fd, sa, &sa_len);
	o->port = us_net_inline_get_port(conn_type, sa);
	o->sa_local = sa;

	// TODO: should check return values here
	us_event e = us_event_create(o->fd, connect_occured, connect_invalid);
	e->priv = o;
	us_event_set_add(es, e);

	return 1;
}

bool us_connection_port_addr_s(us_connection o, const char* addr, uint16_t port, us_connection_type conn_type, us_network_callback_dropped dropped, us_network_message message, bool us_proto, us_event_set es)
{
	bool res = 1;
	// get the host's ip by its name ...
	struct addrinfo hints;
	struct addrinfo *addrinfo, *ai;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_CANONNAME;

	int ret = getaddrinfo(addr, NULL, &hints, &addrinfo);
	
	if(ret != 0)
	{
		perror("getaddrinfo");
		return 0;
	}
	
	ai = addrinfo;
	// find a suitable address
	while(ai)
	{
		if(!us_net_inline_supported_AF(conn_type, ai->ai_family)) continue; // skip this address
		us_connection_type ct = get_AF_AF(ai->ai_family);
		char *str = us_net_get_addr(ai->ai_addr, ct);
		
		// try to connect to it
		res = us_connection_port_ip_s(o, str, port, ct, dropped, message, us_proto, es);

		free(str);
	
		if(res) break;

		ai = ai->ai_next;
	}
	
	freeaddrinfo(addrinfo);
	
	return res;
}

bool us_connection_send_s(us_connection o, us_string_length sl, uint32_t type)
{
	o->sending = true;

	us_string_length output = sl;

	if(o->us_connection)
	{
		bool compressed = false;
		bool encrypted = false;
		
		// check for compression
		if(o->compressed)
		{
			// attempt to compress the data
			uLongf length = (output->length);
			uLongf output_length = (length * 1001/1000) + 14;
			void *odata = malloc(output_length);
			if(odata != NULL)
			{
				int result = compress2((Bytef *)odata, &output_length, (Bytef *)output->string, length, 9);
				if(!(result != Z_OK || output_length > length))
				{
					output = us_string_length_create(odata, output_length);
					compressed = true;
				}
				else
				{
					free(odata);
				}
			}
		}
	
		// check for encryption
		if(o->encrypted)
		{
			// attempt to encrypt the data
			if(o->send_key)
			{
				char *old = output->string;
				unsigned int output_length = output->length;
				unsigned char *temp = us_rc4_crypt_string(o->send_key, (unsigned char *)old, output_length);
				if(temp != NULL)
				{
					if(output != sl)
					{
						if(!us_string_length_destroy(output)) {}
					}
					output = us_string_length_create((char *)temp, output_length);
					encrypted = true;
				}
				else
				{
					return 0;
				}
			}
		}
	
		// make a header for the data
		us_net_mesg *h = malloc(sizeof(us_net_mesg));
		if(h == NULL)
		{
			return 0;
		}
	
		h->magic = htonl(0xDEADBA5C);
		h->length = htonl(sl->length);
		h->lenpack = htonl(output->length);
		h->ID = htonl(type);
		h->flags = htonl(((compressed) ? NET_SEND_MESG_COMPRESSED : 0) | ((encrypted) ? NET_SEND_MESG_ENCRYPTED : 0)); // set flags if encryption and compression was used or not ...

		// send the header ...
		if(!net_send(o->fd, h, sizeof(us_net_mesg)))
		{
			free(h);
			if(output != sl)
			{
				if(!us_string_length_destroy(output)) {}
			}
			return 0;
		}
		free(h);
	}
	// now actually send it ...
	if(!net_send(o->fd, output->string, output->length))
	{
		return false;
	}
	
	o->sending = false;

	if(output != sl)
	{
		if(!us_string_length_destroy(output)) {}
	}

	return true;
}

bool us_connection_accept_s(us_connection o, int fd, us_event_set es, us_connection_type conn_type, us_network_callback_dropped dropped, us_network_message message)
{
	bool res = 1;
	
	o->conn_type = conn_type;
	o->dropped = dropped;
	o->message = message;

	// TODO: support non-IP protocols ?
	unsigned int addrlen = us_net_inline_get_addrlen(o->conn_type);
	o->sock_addr = malloc(addrlen);
	if(o->sock_addr == NULL)
	{
		return false;
	}

	o->fd = accept(fd, o->sock_addr, &addrlen);
	
	// TODO: should check return values here
	us_event e = us_event_create(o->fd, connect_occured, connect_invalid);
	e->priv = o;
	us_event_set_add(es, e);

	return res;
}

