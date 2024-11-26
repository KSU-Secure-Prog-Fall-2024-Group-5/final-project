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

Arguments parse_args(size_t args_len, char *args[]);

typedef struct {
	size_t fields_num, lines_num;
	size_t length_with_terminator;
} ArgumentsStringInfo;

#define is_field_separator(c) (c == ' ' || c == '\t')
#define is_line_separator(c)  (c == '\n' || c == '\r')
#define is_separator(c)       (is_field_separator(c) || is_line_separator(c))

ArgumentsStringInfo parse_args_info(char *arg_string) {
	ArgumentsStringInfo result;
	result.fields_num = 0;
	result.lines_num = 0;
	result.length_with_terminator = 0;

	if (arg_string == NULL || arg_string[0] == '\0') return result;

	// note that because we're really kinda "splitting" a string in half when we
	// write a terminator in the middle of it, we *start* with one field and one
	// line.
	bool on_new_line = true;

	// first scan: find length of string and count the fields and lines.
	char curr, *iter;
	for (iter = arg_string, curr = *iter; curr != '\0'; curr = *(++iter)) {
		if (is_separator(curr)) {
			on_new_line |= is_line_separator(curr);
		} else {
			result.fields_num++;
			if (on_new_line) {
				result.lines_num++;
				on_new_line = false;
			}

			// eat rest of field
			do curr = *(++iter);
			while (!is_separator(curr));

			// without this, we'd step past the next character
			// without checking it, because of `for` loop shenanigans.
			iter--;
		}
	}

	result.length_with_terminator = iter - arg_string;
	// as if given by strlen, but plus 1

	return result;
}

ArgumentsList parse_args_batch(char *arg_string, ArgumentsStringInfo info) {
	ArgumentsList result;
	result.length = 0;
	result.args_items = NULL;

	if (arg_string == NULL || arg_string[0] == '\0') return result;

	// copy the arg string to do in-place antics with it without fear of someone
	// else allocating/deallocating with it and invalidating all the pointers
	char *arg_buffer = calloc(info.length_with_terminator, sizeof(char));
	strncpy(arg_buffer, arg_string, info.length_with_terminator);

	// pointers to where each field/line starts in the arg_buffer
	char **fields = calloc(info.fields_num, sizeof(char *));
	size_t *first_field_index_of_lines =
		calloc(info.lines_num + 1, sizeof(size_t));

	// also keep a respective iterator for the two pointer arrays
	size_t field_index = 0;
	size_t line_index = 0;
	bool on_new_line = false;

	// make the funny iter variables...
	char curr, *iter = arg_buffer;

	// stash the start point of each field to assign, when the time comes
	// and we find an end point and split it off by writing a '\0'
	char *field_start = iter;
	size_t first_field_index_of_line = 0;

	for (iter = arg_buffer, curr = *iter; curr != '\0'; curr = *(++iter)) {
		if (is_separator(curr)) {
			// split off the string as we've reached the end of a field.
			// since the iterator has already passed the character, we index
			// backwards to overwrite the separator character, but this is okay
			*iter = '\0';

			// eat rest of line separators
			on_new_line |= is_line_separator(curr);
			do {
				curr = *(++iter);
				on_new_line |= is_line_separator(curr);
			} while (is_separator(curr));

			// a complete field item is inside the range
			// arg_buffer[field_start..iter]
			fields[field_index++] = field_start;
			field_start = iter;
			printf("\t%ld: [%s]\n", field_index - 1, fields[field_index - 1]);

			// without this, we'd step past the next character
			// without checking it, because of `for` loop shenanigans.
			iter--;

			// if it's a line, save the next field index. it's important
			if (on_new_line) {
				first_field_index_of_lines[line_index++] =
					first_field_index_of_line;
				first_field_index_of_line = field_index;
				on_new_line = false;

				printf("line %ld (%ld): %ld\n", line_index, field_index,
					first_field_index_of_lines[line_index - 1]);
			}
		}
	}

	first_field_index_of_lines[info.lines_num] = field_index;

	// and then we allocate the Arguments buffer
	Arguments *args_items = calloc(info.lines_num, sizeof(Arguments));
	result.args_items = args_items;
	result.length = info.lines_num;

	for (size_t i = 0; i < result.length; i++) {
		size_t args_len =
			first_field_index_of_lines[i + 1] - first_field_index_of_lines[i];
		printf("%ld: %ld to %ld = %ld\n", i, first_field_index_of_lines[i],
			first_field_index_of_lines[i + 1], args_len);
		args_items[i] =
			parse_args(args_len, &fields[first_field_index_of_lines[i]]);
	} // and then finally use parse_args on the resulting arrays.


	free(first_field_index_of_lines);

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

	if (args_len < 1)
		die("got an empty args list. never would've passed validation anyway",
			1);

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
		printf(CONSOLE_VIS_PANIC "ERROR: %s" CONSOLE_VIS_RESET "\n", msg);
		exit(1);
	}

	return result;
}

char *read_into_string(FILE *file, size_t *out_length) {
#define BUFFER_GROW_BY 256
	size_t local_length;
	if (out_length == NULL) out_length = &local_length;
	*out_length = 0;

	char *result = NULL;

	while (!feof(file)) {
		size_t new_length = BUFFER_GROW_BY + *out_length;
		result = realloc(result, new_length);
		size_t entries =
			fread(&result[*out_length], sizeof(char), BUFFER_GROW_BY, file);
		*out_length += entries;
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

	bool use_batch_file = argv == 3 && strncmp(argc[1], "-B", 3) == 0;
	printf("use batch file? %s\n", use_batch_file ? "yeah" : "no");

	ArgumentsList args_list;
	if (use_batch_file) {
		FILE *file = fopen(argc[2], "r");
		char *file_str = read_into_string(file, NULL);
		ArgumentsStringInfo info = parse_args_info(file_str);
		args_list = parse_args_batch(file_str, info);
		free(file_str);
	} else {
		Arguments args = parse_args(argv - 1, &argc[1]);
		args_list.length = 1;
		args_list.args_items = &args;
	}


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
