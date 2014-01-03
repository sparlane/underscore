static bool bst_foreach_r(struct us_event_set_bst_node_s *node, void *passthru, us_event_bst_foreach_cb func)
{
	// input checking
	if(node == NULL || passthru == NULL || func == NULL) return false;
	
	// do this IN-ORDER
	// go deep left ...
	if(node->left) if(!bst_foreach_r(node->left, passthru, func)) return false;
	// now my turn ...
	if(!func(node->data, passthru)) return false;
	// go deep right ...
	if(node->right) if(!bst_foreach_r(node->right, passthru, func)) return false;
	
	// done
	return true;
}

bool us_event_set_bst_foreach_s(us_event_set_bst o, us_event_bst_foreach_cb cb, void* priv)
{
	return bst_foreach_r(o->root, priv, cb);
}

int us_event_set_bst_compare_s(us_event first, us_event second)
{
	if(first == NULL && second == NULL)
	{
		return 0;
	}
	if(first == NULL)
	{
		return 1;
	}
	if(second == NULL)
	{
		return -1;
	}
	if(first->fd == second->fd)
	{
		return 0;
	}
	return first->fd - second->fd;
}
