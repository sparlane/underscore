int us_connection_set_bst_compare_s(us_connection first, us_connection second)
{
	int res;
	//Return 0 if these are the same
	if (first == second || first->fd == second->fd)
		res = 0;
	//Return a negative value if first comes before second
	else if (first->fd < second->fd)
		res = -1;
	//Return a postive value if first comes after second
	else if (first->fd > second->fd)
		res = 1;
	return res;
}
