#include <stdlib.h> // -> EXIT_*
#include <stdio.h>  // -> printf

#define GCRYPT_NO_MPI_MACROS
#define GCRYPT_NO_DEPRECATED
#include <gcrypt.h>

#include "common.h"

int main() {
	init_libgcrypt();
	return EXIT_SUCCESS;
}
