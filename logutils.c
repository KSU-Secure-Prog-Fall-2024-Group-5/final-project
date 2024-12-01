#include <stddef.h> // -> size_t, ptrdiff_t
#include <stdio.h>
#include <ctype.h> // -> isalnum
#include <string.h>

#include "common.h"
#include "logutils.h"

const char *validate_token(char *token) {
	if (token == NULL || token[0] == '\0') return "token is required";
	char curr;
	while ((curr = *(token++)) != '\0') {
		// (A-Z)(a-z)(0-9) no spaces case sensitive
		if (!isalnum(curr))
			return "tokens must only have alphanumeric characters";
	}
	return NULL;
}

const char *validate_name(char *name) {
	if (name == NULL || name[0] == '\0') return "name is required";
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
	LogFile *parsed = calloc(1, sizeof(LogFile));
	parsed->token_to_save = NULL;
	parsed->entries.entry = NULL;
	parsed->entries.length = 0;

	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		printf(CONSOLE_VIS_ERROR
			"ERROR: Unable to open file '%s'" CONSOLE_VIS_RESET "\n",
			filename);
		return parsed;
	}

	char header[8];
	int read = fread(header, 1, 8, file);
	if (read != 8 || strncmp("STARTLOG", header, 8) != 0) {
		printf(CONSOLE_VIS_ERROR
			"ERROR: '%s' is not a valid log\n" CONSOLE_VIS_RESET,
			filename);
		return NULL;
	}

	char *f_buf = malloc(2048); // File buffer
	char *e_buf = malloc(2048); // Entry buffer
	char *p_buf = malloc(2048); // Previous buffer

	int tokenSize = strlen(given_token);
	fgets(f_buf, 2048, file);
	e_buf = strtok(f_buf, "*");
	int bufSize = strlen(e_buf);
	if (strncmp(given_token, e_buf, tokenSize) != 0 ||
		strncmp(e_buf, given_token, bufSize) != 0) {
		printf("Error: tokens do not match.\n");
		return NULL;
	}

	e_buf = strtok(NULL, "#");
	while (e_buf[0] != '\0' && strncmp(e_buf, "ENDLOG", 6) != 0 &&
		strncmp(p_buf, "ENDLOG", 6) != 0) {
		// Read timestamp
		uint32_t timestamp = strtoul(e_buf, NULL, 2);

		// Read employee-name | guest-name | room-id
		p_buf = e_buf;
		e_buf = strtok(NULL, "#");

		LogPerson person;
		person.role = e_buf[0];

		int name_len = strlen(e_buf) - 1;
		person.name = malloc(name_len * sizeof(char));
		for (int i = 0; i < name_len + 1; i++) {
			person.name[i] = e_buf[i + 1];
		}

		// Read arrival-time | departure-time
		p_buf = e_buf;
		e_buf = strtok(NULL, "#");
		LogEventType event = e_buf[0];

		if (e_buf[1] != '\0') {
			printf(CONSOLE_VIS_ERROR
				"ERROR: Log '%s' is broken\n" CONSOLE_VIS_RESET,
				filename);
			return NULL;
		}

		// Optional: room-id
		p_buf = e_buf;
		e_buf = strtok(NULL, "#");
		uint32_t room_id;
		if (e_buf[0] != '\n') {
			room_id = strtoul(e_buf, NULL, 10);
		} else {
			room_id = UINT32_MAX;
		}

		LogEntry entry;
		entry.timestamp = timestamp;
		entry.person = person;
		entry.event = event;
		entry.room_id = room_id;

		logentry_push(&parsed->entries, entry);

		// Load next entry or end of log
		p_buf = e_buf;
		fgets(f_buf, 1024, file);
		e_buf = strtok(f_buf, "#");
	}
	p_buf = e_buf;

	if (strncmp(p_buf, "ENDLOG", 6) != 0) {
		printf(CONSOLE_VIS_ERROR
			"ERROR: '%s' is not a valid log! Read %s\n" CONSOLE_VIS_RESET,
			filename, p_buf);
		return NULL;
	}

	printf("Log '%s' seems good!\n", filename);

	free(f_buf);

	fclose(file);
	file = NULL;

	printf("Read in log with %i entries\n", (int)parsed->entries.length);

	return parsed;
}

void logfile_write(char *filename, LogFile *data) {
	FILE *file = fopen(filename, "w");
	if (file == NULL) die("couldn't create logfile!", 1);

	fprintf(file,
		"STARTLOG"
		"%s*",
		data->token_to_save);

	for (size_t i = 0; i < data->entries.length; i++) {
		LogEntry *entry = &data->entries.entry[i];

		printf("entry timestamp %u\n", entry->timestamp);
		fprintf(file,
			"%b#"   // timestamp
			"%c%s#" // person (role and name)
			"%c#"   // event type
			,
			entry->timestamp, entry->person.role, entry->person.name,
			entry->event);
		if (entry->room_id != UINT32_MAX) {
			fprintf(file, "%u#", entry->room_id);
		}
		fputc('\n', file);
	}

	fprintf(file, "ENDLOG");
	fclose(file);
}

void logentry_push(LogEntryList *list, LogEntry entry) {
	size_t new_length = list->length + 1;
	size_t new_size = new_length * sizeof(LogEntry);
	if (new_size / sizeof(LogEntry) != new_length)
		die("overflow in logentry push realloc", 1);
	list->entry = realloc(list->entry, new_size);
	if (list->entry == NULL)
		die("failed to resize list in logentry push realloc", 1);
	list->length = new_length;
	list->entry[new_length - 1] = entry;
}

LogEntry logentry_pop(LogEntryList *list) {
	LogEntry result = list->entry[list->length--];
	size_t new_size = list->length * sizeof(LogEntry);
	if (new_size / sizeof(LogEntry) != list->length)
		die("overflow in logentry pop realloc", 1);
	list->entry = realloc(list->entry, new_size);
	return result;
}
// you're in charge of alloc'ing names
