#pragma once

#include <stdbool.h>

#include <gcrypt.h>

#define NEED_LIBGCRYPT_VERSION "1.11.0"

static bool init_libgcrypt() {
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

typedef enum {
	LOG_ROLE_EMPLOYEE,
	LOG_ROLE_GUEST,
} LogPersonRole;

typedef enum {
	LOG_EVENT_ARRIVAL,
	LOG_EVENT_DEPARTURE,
} LogEventType;

typedef struct {
	char *name;
	LogPersonRole role;
} LogPerson;

typedef struct {
	// timestamp
	// employee name -or- guest name
	// arrival -or- departure
	// room id
	unsigned long timestamp;
	unsigned long room_id;
	LogPerson person;
	LogEventType event;
} LogEntry;
