module_component module_type_component_find_s(module_type o, char* name)
{
	module_component res = NULL;
	module_component tmp = module_component_create(NULL, name, false);
	module_component_bst_find(o->components, tmp);
	if (!module_component_unref(tmp)) {}
	return res;
}

