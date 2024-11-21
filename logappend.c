#include <stdlib.h> // -> EXIT_*
#include <stdio.h>  // -> printf
#include <string.h>

#define GCRYPT_NO_MPI_MACROS
#define GCRYPT_NO_DEPRECATED
#include <gcrypt.h>

#include "common.h"
#include "logutils.h"

typedef struct {
	char *given_token;
	LogEntry entry;
	char *log_file;
} Arguments;

typedef struct {
	size_t length;
	Arguments *args_items;
} ArgumentsList;

ArgumentsList parse_args_batch(char *arg_string) {
	ArgumentsList result;
	result.length = 0;
	result.args_items = NULL;

	if (arg_string == NULL || arg_string[0] == '\0') return result;

#define is_field_separator(c) (c == ' ' || c == '\t')
#define is_line_separator(c)  (c == '\n' || c == '\r' || is_field_separator(c))

	// note that because we're really kinda "splitting" a string in half when we
	// write a terminator in the middle of it, we *start* with one field and one
	// line.
	size_t fields_num = 1;
	size_t lines_num = 1;

	// first scan: find length of string and count the fields and lines.
	char curr, *iter = arg_string;
	while ((curr = *(iter++)) != '\0') {
		if (is_field_separator(curr)) {
			fields_num++;

			// eat rest of field separators
			do curr = *(++iter);
			while is_field_separator(curr);
		} else if (is_line_separator(curr)) {
			// making a new line is splitting off a new field AND a new line
			fields_num++;
			lines_num++;

			// eat rest of line separators
			do curr = *(++iter);
			while is_line_separator(curr);
		}
	}

	size_t length_with_terminator = iter - arg_string;
	size_t length = length_with_terminator - 1; // as if given by strlen

	// copy the arg string to do in-place antics with it without fear of someone
	// else allocating/deallocating with it and invalidating all the pointers
	char *arg_string_copy = calloc(length_with_terminator, sizeof(char));
	strncpy(arg_string_copy, arg_string, length_with_terminator);

	char **fields = calloc(fields_num, sizeof(char *));
	char **lines = calloc(lines_num, sizeof(char *));

	char **fields_iter = fields;
	char **lines_iter = lines;
	iter = arg_string_copy;
	char *field_start = iter;
	char *line_start = iter;
	while ((curr = *(iter++)) != '\0') {
		if (is_field_separator(curr)) {
			*iter = '\0';
			*(fields_iter++) = field_start;

			// eat rest of field separators
			do curr = *(++iter);
			while is_field_separator(curr);

			// a new field starts here
			field_start = iter;
		} else if (is_line_separator(curr)) {
			*iter = '\0';
			*(fields_iter++) = field_start;
			*(lines_iter++) = line_start;

			// eat rest of line separators
			do curr = *(++iter);
			while is_line_separator(curr);

			// a new field AND a new line both start here
			field_start = iter;
			line_start = iter;
		}
	}

	// and then finally use parse_args on the resulting arrays.

	return result;
}

// PLEASE only use this with ArgumentsLists that you have rec'vd
// from the corresponding parse function
void free_args_batch(ArgumentsList list) { exit(1); }

const char *validate_args(Arguments *args) {
	const char *msg;

	// LogArgs itself has a LogEntry inside it. validate that.
	if ((msg = logentry_validate(&args->entry)) != NULL) return msg;

	if (args->log_file == NULL)
		return "log file name required (no flag, just put it as a string)";

	// (and also validate the token too)
	if (args->given_token == NULL) return "token (-K <token>) required";
	if ((msg = validate_token(args->given_token)) != NULL) return msg;

	return NULL;
}

Arguments parse_args(size_t args_len, char *args[]) {
	Arguments result;
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

	const char *msg = validate_args(&result);
	if (msg != NULL) {
		printf(CONSOLE_VIS_ERROR "ERROR: %s" CONSOLE_VIS_RESET "\n", msg);
		exit(1);
	}

	return result;
}

int main(int argv, char *argc[]) {
	if (argv <= 1) {
		printf(
			"%s usage:\n"
			"logappend -T <timestamp> -K <token>\n"
			"    ( -E <employee-name> | -G <guest-name> )\n"
			"    ( -A | -L )\n"
			"    [-R <room-id>]\n"
			"    <log>\n"
			"# insert \n"
			"\n"
			"logappend -B <file>\n"
			"# execute list of commands read line-by-line from <file>\n"
			"# the commands shouldn't start with the executable name,\n"
			"# and they should resemble the first command's form.",
			argv ? argc[0] : "logappend");
		exit(EXIT_FAILURE);
	}

	bool use_batch_file = argv == 3 && strncmp(argc[1], "-B", 3);
	if (use_batch_file) die("i'ven't bothered yet.", EXIT_FAILURE);

	Arguments args = parse_args(argv - 1, &argc[1]);

	ArgumentsList args_list;
	args_list.length = 1;
	args_list.args_items = &args;

	for (size_t i = 0; i < args_list.length; i++) {
		Arguments *args_item = &args_list.args_items[i];
		if (!args_item) die("item in args list was NULL?", EXIT_FAILURE);

		LogFile *file =
			logfile_read(args_item->log_file, args_item->given_token);
		if (file == NULL) die("aaah, well...", EXIT_FAILURE);
		logentry_push(&file->entries, args_item->entry);
		logfile_write(args_item->log_file, file);
	}

	if (!init_libgcrypt()) return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
