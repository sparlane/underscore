#include <stdint.h>
#include <string.h>

#include <us_encryption.h>
#include <us_test.h>

int main(int argc, char *argv[]){
	us_rc4 k1 = us_rc4_create();
	us_rc4 k2 = us_rc4_create();
	
	us_assert(k1 != NULL, "Key(1) CREATE failed");
	us_assert(k2 != NULL, "Key(2) CREATE failed");

	us_assert(us_rc4_setup(k1, (unsigned char *)"test", 4), "Key(1) Setup failed");
	us_assert(us_rc4_setup(k2, (unsigned char *)"test", 4), "Key(2) Setup failed");
	

	for(int i = 0; i < 26; i++)
	{
		us_assert(us_rc4_crypt(k2, us_rc4_crypt(k1, 'a' + i)) == ('a' + i), "Failed enCRYPT and deCRYPT %c", 'a' + i);
	}
	
	us_assert(us_rc4_unref(k1), "Failed to DESTROY Key(1)");
	us_assert(us_rc4_unref(k2), "Failed to DESTROY Key(2)");
	
	k1 = us_rc4_create();
	us_assert(us_rc4_setup(k1, (unsigned char *)"Key", 3), "Key(1) Setup failed");

	for(int i = 0; i < 256; i++)
	{
		printf("%02x ", (unsigned char)k1->S[i]);
	}

	char *text = us_rc4_crypt_string (k1, "Plaintext", 9);
	us_assert(text != NULL, "Encrypting string");
	
	for(int i = 0; i < 9; i++)
	{
		printf("%02x ", (unsigned char)text[i]);
	}
	
	return 0;
}
