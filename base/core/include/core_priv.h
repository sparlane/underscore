#define CORE_PRINT_ERROR 1
#define CORE_PRINT_DEBUG 2

#include <us_event.h>

void core_print(int mode, const char *format, ...);


// local.c
extern local_state global;

bool local_init(char *primaryIP, char *configFile, bool IPv6);

us_event_set *local_event_set(void);
bool local_is_primary(void);
bool local_core_connected(void);
bool local_set_other_core(us_net_connection *c);
bool local_send_other_core(us_string_length sl, uint32_t ident);
char *local_get_other_core_addr(void);
bool local_lost_other_core(us_net_connection *c);
module_type *module_type_find(char *name);
module_component *module_component_find(module_type *mt, char *name);
server_type *server_type_find(char *name);
// stuff that lua uses (as well)

bool module_type_source(module_type *mt, char *file);
bool server_type_source(server_type *st, char *file);
bool server_depends_component(server_type *st, module_component *mc);

