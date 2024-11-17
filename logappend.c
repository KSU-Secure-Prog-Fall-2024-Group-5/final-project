#include <stdlib.h> // -> EXIT_*
#include <stdio.h>  // -> printf

#define GCRYPT_NO_MPI_MACROS
#define GCRYPT_NO_DEPRECATED
#include <gcrypt.h>

#include "common.h"

#define NEED_LIBGCRYPT_VERSION "1.11.0"

int main() {
	puts(gcry_check_version(NULL));
	return EXIT_SUCCESS;
}