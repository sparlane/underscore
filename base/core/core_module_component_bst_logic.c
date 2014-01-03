#include <string.h>

int module_component_bst_compare_s(module_component first, module_component second)
{
	int res;
	res = strcmp(first->name, second->name);
	return res;
}
