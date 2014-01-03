module_type local_state_module_type_find_s(local_state o, char* name)
{
	module_type res = NULL;
	module_type tmp = module_type_create(name);
	module_type_bst_find(o->module_types, tmp);
	if (!module_type_unref(tmp)) {}
	return res;
}

