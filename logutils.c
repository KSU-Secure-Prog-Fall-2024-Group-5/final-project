#include <stddef.h> // -> size_t, ptrdiff_t
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "logutils.h"

LogArgsList parse_args_batch(char *arg_string) {
	// split by \n
	// then split by whitespace as if argc/argv
	// fuck up ooops
	LogArgsList result;
	result.length = 0;
	result.args_items = NULL;
	return result;
}

LogArgs parse_args(size_t args_len, char *args[]) {
	LogArgs result;
	result.entry.timestamp = UINT32_MAX;
	result.entry.room_id = UINT32_MAX; // optional
	result.entry.person.name = NULL;
	result.entry.person.role = '\0';
	result.entry.event = '\0';
	result.given_token = NULL;
	result.log_file = NULL;

	for (size_t i = 0; i < args_len; i++) {
#define match_flag(f) (strncmp(args[i], f, 3) == 0)
#define need_arg      ((i + 1) < args_len)
		if (false) { // space-filler so everything lines up :)
		} else if (match_flag("-T") && need_arg) {
			// -T <timestamp>
			char *tail = args[i + 1];
			char *expected_tail = &tail[strlen(tail)];
			result.entry.timestamp = strtoul(args[i + 1], &tail, 10);
			if (tail != expected_tail) die("malformed number in timestamp", 1);
			i++;
		} else if (match_flag("-K") && need_arg) {
			// -K <token>
			result.given_token = args[i + 1];
			i++;
		} else if ((match_flag("-E") || match_flag("-G")) && need_arg) {
			// -E <employee-name> | -G <guest-name>
			if (result.entry.person.name != NULL)
				die("only one person per entry", 1);
			result.entry.person.name = args[i + 1];
			result.entry.person.role =
				args[i][1] == 'E' ? LOG_ROLE_EMPLOYEE : LOG_ROLE_GUEST;
			i++;
		} else if (match_flag("-A") || match_flag("-L")) {
			// -A | -L
			if (result.entry.event != '\0')
				die("only one event type per entry", 1);
			result.entry.event =
				args[i][1] == 'A' ? LOG_EVENT_ARRIVAL : LOG_EVENT_DEPARTURE;
		} else if (match_flag("-R") && need_arg) {
			// -R <room-id>
			char *tail = args[i + 1];
			char *expected_tail = &tail[strlen(tail)];
			result.entry.room_id = strtoul(args[i + 1], &tail, 10);
			if (tail != expected_tail) die("malformed number in room id", 1);
			i++;
		} else {
			// <log>
			if (result.log_file != NULL)
				die("only one log file per command", 1);
			result.log_file = args[i];
		}
#undef need_arg
#undef match_flag
	}

	const char *msg = logargs_validate(&result);
	if (msg != NULL) puts(msg), exit(1);

	return result;
}

const char *validate_token(char *given_token) {
	char curr;
	while ((curr = *(given_token++)) != '\0') {
		// (A-Z)(a-z)(0-9) no spaces case sensitive
		// return nice error messages
	}
	return NULL;
}

const char *validate_name(char *name) {
	char curr;
	while ((curr = *(name++)) != '\0') {
		// (A-Z)(a-z) no spaces case sensitive
		// return nice error messages so maybe have separate ones for spaces and
		// numbers and any other.
		// and check logargs_validate for example of bringing other errors
		// forward
	}
	return NULL;
}

const char *logentry_validate(LogEntry *entry) {
	if (entry->timestamp & 0xC0000000)
		return "timestamp (-T <timestamp>) is required";
	if ((entry->room_id & 0xC0000000) && (entry->room_id != UINT32_MAX))
		return "room id (-R <room-id>) is in invalid range";
	if ((entry->person.name == NULL) || (entry->person.role == '\0'))
		return "employee name (-E <employee-name>) or guest name "
			   "(-G <guest-name>) but not both is required";
	if (entry->event == '\0')
		return "arrival event (-A) or departure event (-L) required";

	return NULL;
}

const char *logargs_validate(LogArgs *args) {
	{ // LogArgs itself has a LogEntry inside it. validate that.
		const char *msg = logentry_validate(&args->entry);
		if (msg != NULL) return msg;
	}

	if (args->given_token == NULL) return "token (-K <token>) required";
	if (args->log_file == NULL)
		return "log file name required (no flag, just put it as a string)";

	return NULL;
}

LogFile *logfile_read(char *filename, char *given_token) {
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		printf("\033[0;31mERROR: Unable to open file '%s' \033[0;37m \n", filename);
		return NULL;
	}

	LogFile parsed;
	for (size_t i = 0; i < sizeofarr(parsed.token); i++) parsed.token[i] = 0;
	parsed.entries.entry = NULL;
	parsed.entries.length = 0;

	fclose(file);
	file = NULL;

	exit(1);
	// return parsed;
}
