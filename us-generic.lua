function explode(d,p)
  local t, ll
  t={}
  ll=0
  if(#p == 1) then return {p} end
    while true do
      l=string.find(p,d,ll,true) -- find the next d in the string
      if l~=nil then -- if "not not" found then..
        table.insert(t, string.sub(p,ll,l-1)) -- Save it in our array.
        ll=l+1 -- save just after where we found it for searching next time.
      else
        table.insert(t, string.sub(p,ll)) -- Save what's left in our array.
        break -- Break at end, as it should be, according to the lua manual.
      end
    end
  return t
end

cflags = { '-Wall','-Werror', '-std=c99', '-D_GNU_SOURCE', '-g', '-ggdb2' }

if(feature('cflags') ~= nil) then
	for i,f in pairs(explode(' ', feature('cflags'))) do
		print('cflags+='..f)
		if f ~= '' then
			table.insert(cflags,f)
		end
	end
end

mkdir(feature('destdir')..'/bin')
mkdir(feature('destdir')..'/sbin')
mkdir(feature('destdir')..'/lib')

function gcc(dir,custom_cflags,files,output,cxx)
	this_cflags = {'-o',output}

	for i,f in pairs(cflags) do
		table.insert(this_cflags,f)
	end
	for i,f in pairs(custom_cflags) do
		table.insert(this_cflags,f)
	end
	for i,f in pairs(files) do
		table.insert(this_cflags,f)
	end

	compiler = 'gcc'
	if cxx then
		compiler = 'g++'
	end
	output_files = cmd(dir,compiler,this_cflags,{output})
	return output_files[output]
end

function generate_library(libname,desc)
	l_cflags = { '-o' , libname, '-fPIC', '-shared' }
	l_files = {}
	l_o_cflags = {'-c','-fPIC'}
	use_cxx = false
	if(desc.cxx ~= nil) then
		if(desc.cxx == true) then
			use_cxx = true
		end
	end
	if(desc.extra_cflags ~= nil) then
		for i,f in pairs(desc.extra_cflags) do
			table.insert(l_o_cflags, f)
		end
	end
	for i,f in pairs(desc.sources) do
		f_o_name = f.short_name:gsub('.c$','.o')
		f_o_name = f_o_name:gsub('.cpp$','.o')
		table.insert(l_files, gcc('',l_o_cflags, {f},f_o_name,use_cxx))
	end
	if(desc.extra_libs ~= nil) then
		for i,l in pairs(desc.extra_libs) do
			table.insert(l_cflags, l)
		end
	end
	l_lib = gcc('', l_cflags, l_files, libname, use_cxx)
	finalcmd('','install',{'-D',l_lib, feature('destdir')..'/lib/'..libname})
	finalcmd('','ln',{'-s',libname,feature('destdir')..'/lib/'..desc.short_name})	
end

function generate_program(appname,desc)
	p_cflags = { '-o' , appname }
	p_files = {}
	p_o_cflags = {'-c'}
	use_cxx = false
	if(desc.cxx ~= nil) then
		print "Using cxx"
		use_cxx = true
	end
	if(desc.extra_cflags ~= nil) then
		for i,f in pairs(desc.extra_cflags) do
			table.insert(p_o_cflags, f)
			print(f)
		end
	end
	for i,f in pairs(desc.sources) do
		f_o_name = f.short_name:gsub('.c$','.o')
		f_o_name = f_o_name:gsub('.cpp$','.o')
		table.insert(p_files, gcc('',p_o_cflags, {f},f_o_name,use_cxx))
	end
	if(desc.extra_libs ~= nil) then
		for i,l in pairs(desc.extra_libs) do
			table.insert(p_cflags, l)
		end
	end
	p_app = gcc('', p_cflags, p_files, appname, use_cxx)
	finalcmd('','install',{'-D',p_app, feature('destdir')..'/'..desc.installpath..'/'..appname})
end

