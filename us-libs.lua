require ('us-generic')

require ('lb/lib/common')
require ('lb/lib/datacoding')
require ('lb/lib/delay')
require ('lb/lib/encryption')
require ('lb/lib/event')
require ('lb/lib/job_queue')
require ('lb/lib/lua')
require ('lb/lib/network')
require ('lb/lib/thread')

libs =
{
	['libus_common.so.0.0.1'] =
	{
		['sources'] = lib_common_sources(),
		['headers'] = lib_common_headers(),
		['extra_cflags'] = lib_common_cflags(),
		['short_name'] = 'libus_common.so'
	},
	['libus_datacoding.so.0.0.1'] =
	{
		['sources'] = lib_datacoding_sources(),
		['headers'] = lib_datacoding_headers(),
		['extra_cflags'] = lib_datacoding_cflags(),
		['short_name'] = 'libus_datacoding.so'
	},
	['libus_delay.so.0.0.1'] =
	{
		['sources'] = lib_delay_sources(),
		['headers'] = lib_delay_headers(),
		['extra_cflags'] = lib_delay_cflags(),
		['short_name'] = 'libus_delay.so'
	},
	['libus_encryption.so.0.0.1'] =
	{
		['sources'] = lib_encryption_sources(),
		['headers'] = lib_encryption_headers(),
		['extra_cflags'] = lib_encryption_cflags(),
		['short_name'] = 'libus_encryption.so'
	},
	['libus_event.so.0.0.1'] =
	{
		['sources'] = lib_event_sources(),
		['headers'] = lib_event_headers(),
		['extra_cflags'] = lib_event_cflags(),
		['short_name'] = 'libus_event.so'
	},
	['libus_job_queue.so.0.0.1'] =
	{
		['sources'] = lib_job_queue_sources(),
		['headers'] = lib_job_queue_headers(),
		['extra_cflags'] = lib_job_queue_cflags(),
		['short_name'] = 'libus_job_queue.so'
	},
	['libus_lua.so.0.0.1'] =
	{
		['sources'] = lib_lua_sources(),
		['headers'] = lib_lua_headers(),
		['extra_cflags'] = lib_lua_cflags(),
		['short_name'] = 'libus_lua.so'
	},
	['libus_network.so.0.0.1'] =
	{
		['sources'] = lib_network_sources(),
		['headers'] = lib_network_headers(),
		['extra_cflags'] = lib_network_cflags(),
		['short_name'] = 'libus_network.so'
	},
	['libus_thread.so.0.0.1'] =
	{
		['sources'] = lib_thread_sources(),
		['headers'] = lib_thread_headers(),
		['extra_cflags'] = lib_thread_cflags(),
		['short_name'] = 'libus_thread.so'
	},


}

for l,desc in pairs(libs) do
	generate_library(l,desc)
	for t,d in pairs(desc.headers) do
		print (t,d)
		finalcmd('','install',{'-m','0644','-D',d,feature('destdir')..'/usr/include/'..d.short_name})
	end
end
