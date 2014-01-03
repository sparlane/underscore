#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static inline int us_net_inline_get_AF(us_connection_type conn_type)
{
	if(conn_type->IPv6) return AF_INET6;
	if(conn_type->IPv4) return AF_INET;
	return 0;
}

static inline uint16_t us_net_inline_get_port(us_connection_type conn_type , void *sockaddr)
{
	if(conn_type->IPv6) return htons(((struct sockaddr_in6 *)sockaddr)->sin6_port);
	if(conn_type->IPv4) return htons(((struct sockaddr_in *)sockaddr)->sin_port);
	return 0;
}

static inline size_t us_net_inline_get_addrlen(us_connection_type conn_type)
{
	if(conn_type->IPv6) return sizeof(struct sockaddr_in6);
	if(conn_type->IPv4) return sizeof(struct sockaddr_in);
	return 0;
}

static inline bool us_net_inline_supported_AF(us_connection_type conn_type, int address_family)
{
	switch(address_family)
	{
		case AF_INET6: return (conn_type->IPv6);
		case AF_INET: return (conn_type->IPv4);
		default:
			return false;
	}
}

static inline us_connection_type get_AF_AF(int AF)
{
	us_connection_type ct;
	if(AF == AF_INET6) ct = us_connection_type_create(false, true);
	if(AF == AF_INET) ct = us_connection_type_create(true, false);
	return ct;
}
