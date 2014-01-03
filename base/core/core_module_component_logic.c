bool module_component_update_handler_s(module_component o, uint64_t ID, bool primary)
{
	bool res = 0;
		if(primary)
	{
		if(o->pID != ID)
		{
			if(o->pID)
			{
				// remove them ...
				//
			}
			// check if this one is handling the secondary already ?
			if(o->sID == ID)
			{
				// if so, just swap it over
				o->sID = 0;
				o->primary = o->secondary;
				o->secondary = NULL;
			}
		}
	}
	else // secondary
	{
		if(o->sID != ID)
		{
			if(o->pID == ID)
			{
				// swap them over
				o->pID = 0;
				o->secondary = o->primary;
				o->primary = NULL;
			}
		}
	}
	return res;
}

