#include <stdlib.h> // -> EXIT_*
#include <stdio.h>  // -> printf
#include <string.h>

#define GCRYPT_NO_MPI_MACROS
#define GCRYPT_NO_DEPRECATED
#include <gcrypt.h>

#include "common.h"
#include "logutils.h"

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
		exit(EXIT_FAILURE);
	}

	bool use_batch_file = argv == 3 && strncmp(argc[1], "-B", 3);
	if (use_batch_file) {}

	LogArgs args = parse_args(argv - 1, &argc[1]);

	LogArgsList args_list;
	args_list.length = 1;
	args_list.args_items = &args;

	puts("programmer has a nap! etc. from sonic");
	return EXIT_FAILURE;

	for (size_t i = 0; i < args_list.length; i++) {
		LogArgs *args_item = &args_list.args_items[i];
		if (!args_item) die("item in args list was NULL?", EXIT_FAILURE);

		LogFile *file =
			logfile_read(args_item->log_file, args_item->given_token);
		logentry_push(&file->entries, args_item->entry);
		logfile_write(args_item->log_file, file);
	}

	if (!init_libgcrypt()) return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
