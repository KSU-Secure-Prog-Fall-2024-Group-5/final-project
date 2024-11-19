#include <stdlib.h> // -> EXIT_*
#include <stdio.h>  // -> printf

#include "common.h"

int main(int argv, char *argc[]) {
	if (argv <= 1) {
		printf(
			"%s usage:\n"
			"logread -K <token> -S <log>\n"
			"logread -K <token> -R (-E <name> | -G <name>) <log>\n",
			argv ? argc[0] : "logread");
	}

	if (!init_libgcrypt()) return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
