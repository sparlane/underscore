static inline void swap_uchar(unsigned char *b1, unsigned char *b2)
{
	unsigned char t = *b1;
	*b1 = *b2;
	*b2 = t;
}

bool us_rc4_setup_s(us_rc4 o, unsigned char* key, size_t length)
{
	bool res = 1;
	o->idx1 = 0;
	o->idx2 = 0;
	o->S = malloc(256);
	res = (o->S != NULL);
	
	if(res)
	{
		// first set all the S to the identity
		for(int i = 0; i < 256; i++)
		{
			o->S[i] = (unsigned char)i;
		}
		// now randomise this data based on the key
		unsigned char j = 0;
		for(int i = 0; i < 256; i++)
		{
			j = (unsigned char)(j + o->S[i] + key[i % length]);
			swap_uchar(&o->S[i], &o->S[j]);
		}
	}
	
	return res;
}

unsigned char us_rc4_crypt_s(us_rc4 o, unsigned char c)
{
	unsigned char res = 0;
	o->idx1++;
	o->idx2 += o->S[o->idx1];
	
	swap_uchar(&o->S[o->idx1], &o->S[o->idx2]);
	
	res = (c ^ ((unsigned char)(o->S[o->idx1] + o->S[o->idx2])));

	return res;
}

unsigned char *us_rc4_crypt_string_s(us_rc4 o, unsigned char* s, size_t length)
{
	unsigned char *res = malloc(length);
	if(o->S == NULL || res == NULL)
	{
		res = NULL;
	}
	else
	{
		for(int i = 0; i < length; i++)
		{
			o->idx1++;
			o->idx2 += o->S[o->idx1];
	
			swap_uchar(&o->S[o->idx1], &o->S[o->idx2]);
	
			res[i] = (s[i] ^ ((unsigned char)(o->S[o->idx1] + o->S[o->idx2])));
		}
	}
	return res;
}

