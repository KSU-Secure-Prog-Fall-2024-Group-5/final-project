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
	char *buffer;
} ArgumentsList;

typedef struct {
	size_t fields_num, lines_num;
	size_t length;
} ArgumentsStringInfo;

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

	if (args_len < 1) die("got an empty args list", 1);

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

#define is_field_separator(c) (c == ' ' || c == '\t')
#define is_line_separator(c)  (c == '\n' || c == '\r')
#define is_separator(c)       (is_field_separator(c) || is_line_separator(c))

ArgumentsStringInfo parse_args_info(char *arg_string) {
	ArgumentsStringInfo result;
	result.fields_num = 0;
	result.lines_num = 0;
	result.length = 0;

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
			while (curr != '\0' && !is_separator(curr));
			// `iter` is left pointing at first non-separator character.

			// without this, we'd step past the next character
			// without checking it, because of `for` loop shenanigans.
			iter--;
		}
	}

	result.length = iter - arg_string;

	return result;
}

ArgumentsList parse_args_batch(char *arg_string) {
	ArgumentsStringInfo info = parse_args_info(arg_string);

	ArgumentsList result;
	result.length = 0;
	result.args_items = NULL;
	result.buffer = NULL;

	if (arg_string == NULL || arg_string[0] == '\0') return result;

	// copy the arg string to do in-place antics with it without fear of someone
	// else allocating/deallocating with it and invalidating all the pointers
	result.buffer = calloc(info.length + 1, sizeof(char));
	strncpy(result.buffer, arg_string, info.length);

	// pointers to where each field/line starts in the arg_buffer
	char **fields = calloc(info.fields_num, sizeof(char *));
	size_t *first_field_of_lines = calloc(info.lines_num + 1, sizeof(size_t));

	// also keep a respective iterator for the two pointer arrays
	size_t field_index = 0;
	size_t line_index = 0;
	bool on_new_line = true;

	for (size_t iter = 0; iter < info.length; ++iter) {
		char curr = result.buffer[iter];
		if (curr == '\0' || is_separator(curr)) {
			on_new_line |= is_line_separator(curr);
		} else {
			char *field_start = &result.buffer[iter];

			do curr = result.buffer[++iter];
			while (iter < info.length && !is_separator(curr));

			// Do bookkeeping before we write the null terminator.

			// if it's a line, save the next field index. it's important
			if (on_new_line) {
				first_field_of_lines[line_index++] = field_index;
				on_new_line = false; // formality, lol..
			}

			// a complete field item is inside the range
			// result.buffer[field_start..iter]
			fields[field_index++] = field_start;

			// before overwriting the separator, is it a newline?
			// that might be important... ["didn't cause a mystery bug" voice]
			on_new_line = is_line_separator(curr);

			// split off the string as we've reached the end of a field.
			// all good if this is the null terminator, because it's null anyway
			result.buffer[iter--] = '\0';
			// without stepping back, we'd step past the next character
			// without checking it, because of `for` loop shenanigans.
		}
	}

	if (field_index != info.fields_num || line_index != info.lines_num)
		die("incorrect ending position!", 1);

	// for cleaner code, we also stash the final line
	first_field_of_lines[info.lines_num] = info.fields_num;

	// and then we allocate the Arguments buffer
	Arguments *args_items = calloc(info.lines_num, sizeof(Arguments));
	result.args_items = args_items;
	result.length = info.lines_num;

	for (size_t i = 0; i < result.length; i++) {
		size_t args_len = first_field_of_lines[i + 1] - first_field_of_lines[i];
		args_items[i] = parse_args(args_len, &fields[first_field_of_lines[i]]);
	} // and then finally use parse_args on the resulting arrays.

	free(fields);
	free(first_field_of_lines);

	return result;
}

#undef is_separator
#undef is_line_separator
#undef is_field_separator

// PLEASE only use this with ArgumentsLists that you have rec'vd
// from the corresponding parse_args_batch function
void free_args_batch(ArgumentsList list) {
	free(list.args_items);
	free(list.buffer);
}

char *read_into_string(FILE *file, size_t *out_length) {
#define BUFFER_GROW_BY 256
	size_t local_length;
	if (out_length == NULL) out_length = &local_length;
	*out_length = 0;

	size_t capacity = BUFFER_GROW_BY * 2;
	char *result = calloc(capacity, sizeof(char));

	while (!feof(file)) {
		size_t margin = capacity - *out_length;
		if (margin <= BUFFER_GROW_BY) {
			printf("margin is %ld; resizing from %ld to %ld\n", margin,
				capacity, capacity + BUFFER_GROW_BY);
			capacity += BUFFER_GROW_BY;
			result = realloc(result, capacity);
		}
		size_t entries =
			fread(&result[*out_length], sizeof(char), BUFFER_GROW_BY, file);
		*out_length += entries;
	}

	if (capacity == *out_length)
		die("not enough space to add null terminator?", 1);

	result[*out_length] = '\0';

	return result;
#undef BUFFER_GROW_BY
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
			"# insert an entry\n"
			"\n"
			"logappend -B <file>\n"
			"# execute list of commands read line-by-line from <file>\n"
			"# the commands shouldn't start with the executable name,\n"
			"# and they should resemble the first command's form.\n",
			argv ? argc[0] : "logappend");
		exit(EXIT_FAILURE);
	}

	bool use_batch_file = argv == 3 && strncmp(argc[1], "-B", 3) == 0;
	printf("use batch file? %s\n", use_batch_file ? "yeah" : "no");

	ArgumentsList args_list;
	if (use_batch_file) {
		FILE *file = fopen(argc[2], "r");
		if (file == NULL) die("couldn't open file!", 1);
		char *file_str = read_into_string(file, NULL);
		fclose(file);
		args_list = parse_args_batch(file_str);
		free(file_str);
	} else {
		Arguments args = parse_args(argv - 1, &argc[1]);
		args_list.length = 1;
		args_list.args_items = &args;
	}

	for (size_t i = 0; i < args_list.length; i++) {
		Arguments *args_item = &args_list.args_items[i];
		printf(
			"args %ld:"
			"\tgiven_token: [%s]\n"
			"\tlog_file: [%s]\n"
			"\tentry:"
			"\ttimestamp: %u\n"
			"\t\troom_id: %u (%s)\n"
			"\t\tperson:"
			"\tname: %s\n"
			"\t\t\trole: %c (%s)\n"
			"\t\tevent: %c (%s)\n",
			i, args_item->given_token, args_item->log_file,
			args_item->entry.timestamp, args_item->entry.room_id,
			args_item->entry.room_id == UINT32_MAX ? "none" : "some",
			args_item->entry.person.name, args_item->entry.person.role,
			args_item->entry.person.role == LOG_ROLE_EMPLOYEE    ? "employee"
				: args_item->entry.person.role == LOG_ROLE_GUEST ? "guest"
																 : "invalid",
			args_item->entry.event,
			args_item->entry.event == LOG_EVENT_ARRIVAL         ? "arrival"
				: args_item->entry.event == LOG_EVENT_DEPARTURE ? "departure"
																: "invalid");
	}

	if (!init_libgcrypt()) return EXIT_FAILURE;

	for (size_t i = 0; i < args_list.length; i++) {
		Arguments *args_item = &args_list.args_items[i];
		if (!args_item) die("item in args list was NULL?", EXIT_FAILURE);

		bool new_file = false;
		{
			FILE *test = fopen(args_item->log_file, "r");
			new_file = test == NULL;
			if (!new_file) fclose(test);
		}

		LogFile *file;
		if (new_file) {
			file = calloc(1, sizeof(LogFile));
			file->entries.entry = NULL;
			file->entries.length = 0;
		} else {
			file = logfile_read(args_item->log_file, args_item->given_token);
			if (file == NULL) exit(EXIT_FAILURE);
		}
		file->token_to_save = args_item->given_token;

		logentry_push(&file->entries, args_item->entry);
		logfile_write(args_item->log_file, file);

		logfile_free(file);
		free(file);
	}

	if (use_batch_file) free_args_batch(args_list);

	return EXIT_SUCCESS;
}
