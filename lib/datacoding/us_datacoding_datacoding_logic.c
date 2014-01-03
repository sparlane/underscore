#include <string.h>
#include <stdio.h>
#include <ctype.h>

bool us_datacoding_add_s(us_datacoding o, us_datacoding_data data)
{
	bool res = (data != NULL);
	us_datacoding_data *temp = NULL;
	if(res)
	{
		temp = realloc(o->packets, (o->packets_size+1) * sizeof(temp));
		res = (temp != NULL);
	}
	if(res)
	{
		o->packets = temp;
		o->packets[o->packets_size] = data;
		o->packets_size++;
	}
	return res;
}

bool us_datacoding_import_string_s(us_datacoding o, us_string_length string)
{
	bool res = 1;

	uint32_t i = 0;
	int s = 0;
	int c = 0;
	
	unsigned char ident = 0;
	unsigned int len = 0;
	
	while(i < string->length){
		if(!res) break;
		switch(s){
			case 0:{ // waiting for a single char (type ident)
				ident = string->string[i];
				s++;
			}break;
			case 1:{ // waiting for an integer (length)
				if(string->string[i] != ':'){
					if(isdigit(string->string[i])){
						len = (len * 10) + (string->string[i] - '0');
					}else{
						string->string[i] = '\0';
						fprintf(stderr, "Um, wtf ? unexcepted non-number @ %u (last 10 chars are '%.10s' of %i chars)\n", i, &string->string[(i > 10) ? (i-10) : 0], string->length);
						res = 0;
						continue;
					}
				}else{
					char *data = malloc(len+1);
					if(data == NULL)
					{
						res = 0;
						continue;
					}
					if((i + len) < string->length){
						memcpy(data, &string->string[i+1], len);
						((unsigned char *)data)[len] = '\0';
						us_datacoding_add_s(o, us_datacoding_data_create(ident, len, data));
						s = 0;
						i += len;
						len = 0;
					}else{ // err ...
						string->string[i] = '\0';
						fprintf(stderr, "us_datacoding_string_input : error decoding string is too long, %u bytes is bigger than the remaining %u bytes @ byte = %u, last 10 bytes are '%s'\n", len, (string->length - i), i, &string->string[i-10]);
						res = 0;
						continue;
					}
				}
			}break;
		}
		i++;
		c++;
	}
	return res;
}

bool us_datacoding_add_sl_s(us_datacoding o, uint8_t ident, us_string_length string)
{
	bool res = (string != NULL);
	void *data = NULL;
	us_datacoding_data d = NULL;
	if(res)
	{
		data = malloc(string->length);
		res = (data != NULL);
	}
	if(res)
	{
		memcpy(data, string->string, string->length);
		d = us_datacoding_data_create(ident, string->length, data);
		res = (d != NULL);
	}
	if(res)
	{
		res = us_datacoding_add_s(o, d);
	}
	if(!res)
	{
		if(data != NULL) free(data);
		if(d != NULL)
			if(!us_datacoding_data_destroy(d)) {}
	}
	return res;
}

bool us_datacoding_add_sint_s(us_datacoding o, uint8_t ident, signed long long sint)
{
	bool res = 1;
	char *d = NULL;
	int l = asprintf(&d, "%lli", sint);
	res = (l != 0);
	if(res)
	{
		res = us_datacoding_add_s(o, us_datacoding_data_create(ident, l, d));
	}
	free(d);
	return res;
}

bool us_datacoding_add_uint_s(us_datacoding o, uint8_t ident, unsigned long long uint)
{
	bool res = 1;
	char *d = NULL;
	int l = asprintf(&d, "%llu", uint);
	res = (l != 0);
	if(res)
	{
		res = us_datacoding_add_s(o, us_datacoding_data_create(ident, l, d));
	}
	free(d);
	return res;
}

bool us_datacoding_add_blank_s(us_datacoding o, uint8_t ident)
{
	bool res = us_datacoding_add_s(o, us_datacoding_data_create(ident, 0, NULL));
	return res;
}

bool us_datacoding_add_string_s(us_datacoding o, uint8_t ident, uint32_t length, char* s)
{
	bool res = (s != NULL);
	char *str = NULL;
	us_datacoding_data d = NULL;
	if(res)
	{
		str = malloc(length);
		res = (str != NULL);
	}
	if(res)
	{
		memcpy(str, s, length);
	}
	if(res)
	{
		d = us_datacoding_data_create(ident, length, str);
		res = (d != NULL);
	}
	if(res)
	{
		res = us_datacoding_add_s(o, d);
	}
	if(!res)
	{
		if(str != NULL) free(str);
		if(d != NULL)
			if(!us_datacoding_data_destroy(d)) {}
	}
	return res;
}

us_datacoding_data us_datacoding_get_s(us_datacoding o)
{
	us_datacoding_data res = NULL;
	if(o->next < o->packets_size)
	{
		res = o->packets[o->next];
		o->next++;
	}
	return res;
}

bool us_datacoding_reset_get_s(us_datacoding o)
{
	bool res = 1;
	o->next = 0;
	return res;
}

us_string_length us_datacoding_string_s(us_datacoding o)
{
	us_string_length res = NULL;
	char *str = malloc(100);
	size_t length = 0;
	size_t alloc = 100;
	
	for (size_t i = 0; i < o->packets_size; i++)
	{
		// create a string for the header of this packet
		char *t = NULL;
		int l = asprintf(&t,"%c%u:", o->packets[i]->ident, o->packets[i]->length);
		// extend the length of this set
		length += l;
		while ((length + o->packets[i]->length + 1) >= alloc)
		{
			alloc += alloc;
			str = realloc(str, alloc);
		}
		// append the new header to the stream
		memcpy(&(((unsigned char *)str)[length-l]), t, l);
		// cleanup
		free(t);
		// append the new data to stream
		memcpy(&(((unsigned char *)str)[length]), o->packets[i]->data, o->packets[i]->length);
		// fix the length
		length += o->packets[i]->length;
		// fix the last character
		((unsigned char *)str)[length] = '\0';
		// ok, done
	}	
	res = us_string_length_create(str, length);
	return res;
}

