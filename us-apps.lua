require ('us-generic')


require ('lb/lib/common')
require ('lb/lib/delay')
require ('lb/lib/encryption')
require ('lb/lib/event')
require ('lb/lib/job_queue')
require ('lb/lib/network')
require ('lb/lib/thread')
require ('lb/base/core')

base =
{
	['core'] =
	{
		['sources'] = base_core_sources(),
		['extra_cflags'] = base_core_cflags(),
		['installpath'] = 'bin',
		['extra_libs'] = {}
	},
}

table.insert(base.core.sources, file('base/core', 'core.c'))

for p,desc in pairs(base) do
	generate_program(p,desc)
end
