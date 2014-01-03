/*! \file core.c
 * \brief The main code for the core application
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <core.h>
#include <core_priv.h>

#include <us_core.h>
#include <us_datacoding.h>

// dealing with the other core ...
// dropped connection to other core
bool core_callback_dropped(us_net_connection *conn)
{
	core_print(CORE_PRINT_DEBUG, "Lost connection to other core");
	// thats ok, well not if they were the primary, we need to broadcast in this case ...
	if(!local_lost_other_core(conn))
		fprintf(stderr, "Error calling 'lost_local_core'\n");
	return true;
}

bool core_callback_message(us_net_message mesg)
{
	// what kind of transfers do we have ??
	us_datacoding ds = us_datacoding_create();
	switch(mesg->ID)
	{
		case US_CORE_MESG_ID_IDENT: {
			us_datacoding_import_string(ds, mesg->data);
			us_datacoding_data d;
			while((d = us_datacoding_get(ds)) != NULL)
			{
				core_print(CORE_PRINT_DEBUG, "Contains: '%c':\"%s\"", d->ident, (char *)d->data);
			}
			if(!us_datacoding_unref(ds))
				fprintf(stderr, "us_datacoding_set_destroy() failed\n");
		} break;
		case US_CORE_MESG_ID_CONFIG: {
			// config transfer ...
			us_datacoding_import_string(ds, mesg->data);
			us_datacoding_data d = us_datacoding_get(ds);
			// figure out what type it is ...
			switch(d->ident)
			{
				case 'M': { // module type ...
					// create the module type ...
					module_type mt = module_type_create((char *)d->data);
					while((d = us_datacoding_get(ds)) != NULL)
					{
						switch(d->ident)
						{
							case 's': {
								if(!module_type_source_set(mt, (char *)d->data))
									fprintf(stderr, "module_type_source(%s, ...) failed\n", mt->name);
							} break;
							default: {
								core_print(CORE_PRINT_DEBUG, "Unexpected %c parsing a Module", d->ident);
							} break;
						}
					}
				} break;
				case 'C': {
					// module component ...
					us_datacoding_data data = us_datacoding_get(ds);
					if(data->ident != (uint8_t)'M')
					{
						core_print(CORE_PRINT_DEBUG, "Expected 'M' (not '%c') as second data packet of module component", d->ident);
					}
					module_type mt = local_state_module_type_find(global, (char *)data->data);
					module_component mc = module_component_create(mt, (char *)d->data, false);
					while((data = us_datacoding_get(ds)) != NULL)
					{
						switch(data->ident)
						{
							case 'm': {
								mc->mirror = true;
							} break;
							case 'C': {
								module_component_config_set(mc, data->data);
							} break;
							case 'I': {
								mc->pID = atoll(data->data);
							} break;
							case 'i': {
								mc->sID = atoll(data->data);
							} break;
							default: {
								core_print(CORE_PRINT_DEBUG, "Unexpected %c parsing a Module Component", data->ident);
							} break;
						}
					}
				} break;
				case 'S': {
					// server type ...
					server_type st = server_type_create(d->data);
					while((d = us_datacoding_get(ds)) != NULL)
					{
						switch(d->ident)
						{
							case 's': {
								server_type_source_set(st, d->data);
							} break;
							case 'C': { // uncompressed config ..
								server_type_config_set(st, d->data);
							} break;
							case 'D': { // depends ...
								us_datacoding dset = us_datacoding_create();
								us_datacoding_import_string(dset, d->data);
								us_datacoding_data data = us_datacoding_get(dset);
								char *mname = data->data;
								data = us_datacoding_get(dset);
								char *cname = data->data;
								server_type_requires_add(st, module_type_component_find(local_state_module_type_find(global, mname), cname));
								us_datacoding_unref(dset);
							} break;
							default: {
								core_print(CORE_PRINT_DEBUG, "Unexpected %c parsing a Server Type", d->ident);
							} break;
						}
					}
				} break;
				default: {
					// confused ...
					core_print(CORE_PRINT_DEBUG, "Unknown ident (%c) for config transfer (%i packets)", d->ident, ds->packets_size);
				} break;
			}
		} break;
		case US_CORE_MESG_ID_NEXTID: {
			if(!local_is_primary())
			{
				// decode the ID they used ...
				us_datacoding_import_string(ds, mesg->data);
				us_datacoding_data d = us_datacoding_get(ds);
				local_state_nextID_set(global, atoll(d->data));
			}
		} break;
		case US_CORE_MESG_ID_ASSIGN_ROLE: {
			if(!local_is_primary())
			{
				us_datacoding_import_string(ds, mesg->data);
				us_datacoding_data d = NULL;
				char *modt = NULL;
				char *modc = NULL;
				unsigned long long ID = 0;
				bool primary = false;
				while((d = us_datacoding_get(ds)) != NULL)
				{
					switch(d->ident)
					{
						case 'M': {
							modt = strdup(d->data);
						} break;
						case 'C': {
							modc = strdup(d->data);
						} break;
						case 'I': {
							ID = atoll(d->data);
						} break;
						case 'R': {
							primary = ((char *)d->data)[0] == 'T';
						} break;
					}
				}
				module_type mt = local_state_module_type_find(global, modt);
				module_component mc = module_type_component_find(mt, modc);
				if(modt != NULL) free(modt);
				if(modc != NULL) free(modc);
				module_component_update_handler(mc, ID, primary);
			}
		} break;
		default: {
			core_print(CORE_PRINT_DEBUG, "Got unknown message (code = 0x%x) from another core (length = %i)", mesg->ID, (int)mesg->data->length);
		} break;
	}
	us_datacoding_unref(ds);
	return true;
}


void bad_command_line(char *app)
{
	fprintf(stderr, "Usage: %s [-c primaryIP] [-s configScript] [-6]\n", app);
	fprintf(stderr, "One of: [-c primaryIP], [-s configScript] must be supplied\n");
	exit(-1);
}

void ignored_handler(int signum){
}

int main(int argc, char *argv[])
{
	printf("Welcome to %s\n", argv[0]);
	
	// parse the command line parameters
	char *primaryIP = NULL; // ip of the primary (if we are a failover)
	char *configFileName = NULL; // name of configuration file
	bool IPv6 = false; // enable support for IPv6
	for(int i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-c"))
		{
			// we need to be failover
			if((i + 1) >= argc)
			{
				bad_command_line(argv[0]);
			}
			else
			{
				primaryIP = strdup(argv[i+1]);
				i++;
			}
		}
		else if(!strcmp(argv[i], "-s"))
		{
			if((i + 1) >= argc)
			{
				bad_command_line(argv[0]);
			}
			else
			{
				configFileName = strdup(argv[i+1]);
				i++;
			}
		}
		else if(!strcmp(argv[i], "-6"))
		{
			IPv6 = true;
		}
		else
		{
			bad_command_line(argv[0]);	
		}
	}
	if(!configFileName && !primaryIP){
		bad_command_line(argv[0]);
	}
	// yay we have suitable command line arguments ...
	// now lets do something interesting ...
	// ignore PIPE errors
	signal(SIGPIPE, ignored_handler);
	// now create the local state
	if(!local_init(primaryIP, configFileName, IPv6))
	{
		core_print(CORE_PRINT_ERROR, "local_init failed");
		exit(-1);
	}
	// we should do something here ... (like say, be a job queue handler ...)
	while(1)
	{
		us_thread thread_me = us_thread_create(0, local_state_jq_get(global), NULL);
		us_job_queue_processor_thread(thread_me);
		us_thread_unref(thread_me);
	}
}
