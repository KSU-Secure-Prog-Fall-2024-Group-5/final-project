#include <stddef.h>
#include <stdio.h>

#include "logutils.h"

LogFile read_logfile(const char *filename) {
	FILE *file = fopen(filename, "r");

	LogFile parsed;

	fclose(file);
	file = NULL;

	return parsed;
}
