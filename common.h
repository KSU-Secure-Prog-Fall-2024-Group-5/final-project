#pragma once

#include <stdbool.h>
#include <string.h>

#include <gcrypt.h>

#define NEED_LIBGCRYPT_VERSION "1.11.0"

#define sizeofarr(arr) (sizeof(arr) / sizeof(*arr))

#define stringify_eval(x) #x
#define stringify(x)      stringify_eval(x)

#define CONSOLE_VIS_ERROR "\033[31m"
#define CONSOLE_VIS_PANIC "\033[30;41m"
#define CONSOLE_VIS_RESET "\033[m"

#define die(msg, exit_no) \
	do { \
		puts(CONSOLE_VIS_PANIC __FILE_NAME__ \
			":" stringify(__LINE__) ": " msg CONSOLE_VIS_RESET); \
		exit(exit_no); \
	} while (0)

static char *duplicate_string(char *s) {
	size_t s_size = strlen(s);
	char *d = calloc(s_size + 1, sizeof(char));
	strncpy(d, s, s_size);
	return d;
}

static bool init_libgcrypt() {
	return true;
	// Directly copied from:
	// https://gnupg.org/documentation/manuals/gcrypt/Initializing-the-library.html

	/* Version check should be the very first call because it
	    makes sure that important subsystems are initialized.
	    #define NEED_LIBGCRYPT_VERSION to the minimum required version. */
	if (!gcry_check_version(NEED_LIBGCRYPT_VERSION)) {
		fprintf(stderr, "libgcrypt is too old (need %s, have %s)\n",
			NEED_LIBGCRYPT_VERSION, gcry_check_version(NULL));
		exit(2);
	}

	/* We don't want to see any warnings, e.g. because we have not yet
	   parsed program options which might be used to suppress such
	   warnings. */
	gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);

	/* ... If required, other initialization goes here.  Note that the
	   process might still be running with increased privileges and that
	   the secure memory has not been initialized.  */

	/* Allocate a pool of 16k secure memory.  This makes the secure memory
	   available and also drops privileges where needed.  Note that by
	   using functions like gcry_xmalloc_secure and gcry_mpi_snew Libgcrypt
	   may expand the secure memory pool with memory which lacks the
	   property of not being swapped out to disk.   */
	gcry_control(GCRYCTL_INIT_SECMEM, 16384, 0);

	/* It is now okay to let Libgcrypt complain when there was/is
	   a problem with the secure memory. */
	gcry_control(GCRYCTL_RESUME_SECMEM_WARN);

	/* ... If required, other initialization goes here.  */

	/* Tell Libgcrypt that initialization has completed. */
	gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

	return true;
}
