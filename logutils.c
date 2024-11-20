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

	if (result.entry.timestamp & 0xC0000000)
		die("timestamp (-T <timestamp>) is required", 1);
	if ((result.entry.room_id & 0xC0000000) &&
		(result.entry.room_id != UINT32_MAX))
		die("room id (-R <room-id>) is in invalid range", 1);
	if ((result.entry.person.name == NULL) ||
		(result.entry.person.role == '\0'))
		die("employee name (-E <employee-name>) or guest name "
			"(-G <guest-name>) but not both is required",
			1);
	if (result.entry.event == '\0')
		die("arrival event (-A) or departure event (-L) required", 1);
	if (result.given_token == NULL) die("token (-K <token>) required", 1);
	if (result.log_file == NULL)
		die("log file name required (no flag, just put it as a string)", 1);

	return result;
}

LogFile *logfile_read(char *filename, char *given_token) {
	FILE *file = fopen(filename, "r");

	LogFile parsed;
	for (size_t i = 0; i < sizeofarr(parsed.token); i++) parsed.token[i] = 0;
	parsed.entries.entry = NULL;
	parsed.entries.length = 0;

	fclose(file);
	file = NULL;

	exit(1);
	// return parsed;
}
