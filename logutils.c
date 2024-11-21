#include <stddef.h> // -> size_t, ptrdiff_t
#include <stdio.h>
#include <ctype.h> // -> isalnum
#include <string.h>

#include "common.h"
#include "logutils.h"

const char *validate_token(char *given_token) {
	if (given_token == NULL) return "token is required";
	char curr;
	while ((curr = *(given_token++)) != '\0') {
		// (A-Z)(a-z)(0-9) no spaces case sensitive
		if (!isalnum(curr))
			return "tokens must only have alphanumeric characters";
	}
	return NULL;
}

const char *validate_name(char *name) {
	if (name == NULL) return "name is required";
	char curr;
	while ((curr = *(name++)) != '\0') {
		// (A-Z)(a-z) no spaces case sensitive
		if (curr == ' ') return "spaces are not allowed in name";
		if (isdigit(curr)) return "numbers are not allowed in name";
		if (!isalpha(curr))
			return "names must only have alphabetical characters and no spaces";
	}
	return NULL;
}

const char *logentry_validate(LogEntry *entry) {
	const char *msg;

	if (entry->timestamp & 0xC0000000)
		return "timestamp (-T <timestamp>) is required";
	if ((entry->room_id & 0xC0000000) && (entry->room_id != UINT32_MAX))
		return "room id (-R <room-id>) is in invalid range";
	if ((entry->person.name == NULL) || (entry->person.role == '\0'))
		return "employee name (-E <employee-name>) or guest name "
			   "(-G <guest-name>) but not both is required";
	if (entry->event == '\0')
		return "arrival event (-A) or departure event (-L) required";

	if ((msg = validate_name(entry->person.name)) != NULL) return msg;

	return NULL;
}

LogFile *logfile_read(char *filename, char *given_token) {
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		printf(CONSOLE_VIS_ERROR
			"ERROR: Unable to open file '%s'" CONSOLE_VIS_RESET "\n",
			filename);
		return NULL;
	}

    char header[8];
    int read = fread(header, 1, 8, file);
    if (read != 8 || strncmp("STARTLOG", header, 8) != 0) {
        printf(CONSOLE_VIS_ERROR "ERROR: '%s' is not a valid log\n" CONSOLE_VIS_RESET, filename);
        return NULL;
    }

    int tokenSize = strlen(given_token); 
    char* buf = malloc(tokenSize);
    read = fread(buf, 1, tokenSize, file);
    if (read != tokenSize || strncmp(given_token, buf, tokenSize) != 0) {
        printf("Error: tokens do not match");
        return NULL;
    }
    free(buf);

    // MOVE THIS --- TEMPORARY
    buf = malloc(32);
    read = fread(buf, 1, 32, file);
    if (read != 32) {
        printf("Error while reading timestamp\n");
        return NULL;
    }
    uint32_t timestamp = strtoul(buf, NULL, 2);
    printf("Timestamp: %i\n", timestamp);
    // END MOVE

	LogFile parsed;
	for (size_t i = 0; i < sizeofarr(parsed.token); i++) parsed.token[i] = 0;
	parsed.entries.entry = NULL;
	parsed.entries.length = 0;

	fclose(file);
	file = NULL;

	exit(1);
	// return parsed;
}
