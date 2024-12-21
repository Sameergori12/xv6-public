#include "types.h"
#include "stat.h"
#include "user.h"

#define MAX_LINES 2048       // Increased line capacity
#define MAX_LINE_LEN 1024    // Increased line length
#define MAX_FILES 16         // Support for more simultaneous files

char line_buffer[MAX_LINES][MAX_LINE_LEN];

void tail_with_header(int fd, char *filename, int num_lines, int show_filename) {
  int total_lines = 0, current_char = 0;
  int n, last_line_terminated = 1;
  char ch;

  // Enhanced line reading with circular buffer
  while ((n = read(fd, &ch, 1)) > 0) {
    if (ch == '\n' || current_char >= MAX_LINE_LEN - 1) {
      line_buffer[total_lines % MAX_LINES][current_char] = '\0';
      total_lines++;
      current_char = 0;
      last_line_terminated = (ch == '\n');
    } else {
      line_buffer[total_lines % MAX_LINES][current_char++] = ch;
      last_line_terminated = 0;
    }
  }

  // Handle incomplete final line
  if (current_char > 0) {
    line_buffer[total_lines % MAX_LINES][current_char] = '\0';
    total_lines++;
  }

  // Error and edge case handling
  if (n < 0) {
    printf(2, "tail: error reading %s\n", filename);
    exit();
  }

  if (total_lines == 0) {
    printf(1, "tail: %s is empty\n", filename);
    return;
  }

  // Multi-file header support
  if (show_filename && filename[0] != '\0') {
    printf(1, "==> %s <==\n", filename);
  }

  // Flexible last line display
  int start_line = total_lines > num_lines ? total_lines - num_lines : 0;
  for (int i = start_line; i < total_lines; i++) {
    printf(1, "%s", line_buffer[i % MAX_LINES]);
    if (i < total_lines - 1 || last_line_terminated) {
      printf(1, "\n");
    }
  }

  // Optional: Add a separator for multi-file display
  if (show_filename) {
    printf(1, "\n");
  }
}

int main(int argc, char *argv[]) {
  int fd, num_lines = 10;
  int multi_file_mode = 0;

  // Advanced argument parsing
  if (argc > 1) {
    if (argv[1][0] == '-') {
      num_lines = atoi(argv[1] + 1);
      if (num_lines <= 0) {
        printf(2, "tail: invalid line count %s\n", argv[1]);
        exit();
      }
      argv++;
      argc--;
    }

    // Detect multi-file mode
    multi_file_mode = (argc > 2);
  }

  // Standard input handling
  if (argc <= 1) {
    tail_with_header(0, "", num_lines, 0);
    exit();
  }

  // Process files with optional headers
  for (int i = 1; i < argc; i++) {
    if ((fd = open(argv[i], 0)) < 0) {
      printf(2, "tail: cannot open %s\n", argv[i]);
      continue;  // Skip to next file instead of exiting
    }
    tail_with_header(fd, argv[i], num_lines, multi_file_mode);
    close(fd);
  }

  exit();
}