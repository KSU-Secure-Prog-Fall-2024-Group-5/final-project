#include <stdlib.h> // -> EXIT_*
#include <stdio.h>  // -> printf

#define GCRYPT_NO_MPI_MACROS
#define GCRYPT_NO_DEPRECATED
#include <gcrypt.h>

#include "common.h"

int main(int argv, char *argc[]) {
	if (argv <= 1) {
		printf(
			"%s usage:\n"
			"logappend -T <timestamp> -K <token>\n"
			"	( -E <employee-name> | -G <guest-name> )\n"
			"	( -A | -L )\n"
			"	[-R <room-id>]\n"
			"	<log>\n"
			"logappend -B <file>\n",
			argv ? argc[0] : "logappend");
	}

	if (!init_libgcrypt()) return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
