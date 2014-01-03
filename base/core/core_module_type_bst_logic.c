#include <string.h>

int module_type_bst_compare_s(module_type first, module_type second)
{
	int res;
	res = strcmp(first->name, second->name);
	return res;
}
