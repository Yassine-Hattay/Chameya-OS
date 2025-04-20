#include "my_spiffs.h"

void mount_spiffs() {
	esp_vfs_spiffs_conf_t conf = { .base_path = "/spiffs", .partition_label =
			"spiffs", .max_files = 20, .format_if_mount_failed = true };

	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		printf("Failed to mount or format fileSystem ");
	} else {
		printf("SPIFFS mounted successfully ");
	}
}

void unmount_spiffs() {
	esp_vfs_spiffs_unregister("spiffs");
	printf("SPIFFS unmounted successfully\n");
}

void append_to_file(const char *full_path, const char *data) {

	mount_spiffs();

	// Open the file for appending
	FILE *f = fopen(full_path, "a");
	if (f == NULL) {
		printf("Failed to open file '%s' for appending\n", full_path);
		return;
	}

	// Write data to the file
	fprintf(f, "%s", data);
	fclose(f);

	unmount_spiffs();

}

void print_file_contents(const char *full_path) {

	mount_spiffs();

	// Open the file in read mode
	FILE *f = fopen(full_path, "r");
	if (f == NULL) {
		printf("Failed to open file '%s' for reading \n", full_path);
		return;
	}

	int ch;
	while ((ch = fgetc(f)) != EOF) {
		putchar(ch);
	}

	fclose(f);

	unmount_spiffs();

}

char* read_file_contents(const char *full_path) {
	mount_spiffs();

	FILE *f = fopen(full_path, "r");
	if (f == NULL) {
		printf("Failed to open file '%s' for reading \n", full_path);
		return NULL;
	}

	// Seek to end to determine file size
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	rewind(f);

	// Allocate buffer (+1 for null-terminator)
	char *buffer = malloc(size + 1);
	if (buffer == NULL) {
		printf("Memory allocation failed.\n");
		fclose(f);
		unmount_spiffs();
		return NULL;
	}

	// Read file into buffer
	fread(buffer, 1, size, f);
	buffer[size] = '\0';  // Null-terminate

	fclose(f);
	unmount_spiffs();

	return buffer;  // Caller is responsible for freeing the buffer
}

void clear_file_contents(const char *full_path) {
	mount_spiffs();

	// Open the file in write mode (truncates/clears the file)
	FILE *f = fopen(full_path, "w");
	if (f == NULL) {
		printf("Failed to open file '%s' for clearing\n", full_path);
		return;
	}

	// Do nothing, just close to commit the empty file
	fclose(f);

	printf("File '%s' cleared successfully.\n", full_path);
	unmount_spiffs();
}

void format_spiffs() {
	// You must first unmount the filesystem before formatting
	esp_vfs_spiffs_unregister("spiffs");

	esp_err_t ret = esp_spiffs_format("spiffs");
	if (ret == ESP_OK) {
		printf("SPIFFS formatted successfully.\n");
	} else {
		printf("Failed to format SPIFFS. Error: %s\n", esp_err_to_name(ret));
	}
}

void delete_file(const char *full_path) {
	mount_spiffs();

	// 1. Delete the actual file
	if (remove(full_path) == 0) {
		printf("File '%s' deleted successfully.\n", full_path);
	} else {
		printf("Failed to delete file '%s'.\n", full_path);
	}

	// 2. Extract just the file name from the full path
	const char *filename = strrchr(full_path, '/');
	if (!filename || *(filename + 1) == '\0') {
		printf("Invalid file path: '%s'\n", full_path);
		unmount_spiffs();
		return;
	}
	filename++;  // Move past the last '/'

	// Copy and clean filename to remove trailing newline or carriage return
	char filename_clean[128];
	strncpy(filename_clean, filename, sizeof(filename_clean) - 1);
	filename_clean[sizeof(filename_clean) - 1] = '\0';

	size_t len = strlen(filename_clean);
	while (len > 0
			&& (filename_clean[len - 1] == '\n'
					|| filename_clean[len - 1] == '\r')) {
		filename_clean[--len] = '\0';
	}

	printf("Cleaned filename to search for: '%s'\n", filename_clean);

	// 3. Open the original list and a temp file
	const char *list_path = "/spiffs/notebook/filenames.txt";
	FILE *f_in = fopen(list_path, "r");
	if (!f_in) {
		printf("Could not open '%s' for reading.\n", list_path);
		unmount_spiffs();
		return;
	}

	FILE *f_out = fopen("/spiffs/notebook/filenames_temp.txt", "w");
	if (!f_out) {
		printf("Could not open temp file for writing.\n");
		fclose(f_in);
		unmount_spiffs();
		return;
	}

	char line[128];
	int match_found = 0;

	// 4. Line-by-line filter
	while (fgets(line, sizeof(line), f_in)) {
		// Strip \n and \r from the line read
		size_t line_len = strlen(line);
		while (line_len > 0
				&& (line[line_len - 1] == '\n' || line[line_len - 1] == '\r')) {
			line[--line_len] = '\0';
		}

		printf("Comparing '%s' with '%s'\n", line, filename_clean);

		if (strcmp(line, filename_clean) == 0) {
			match_found = 1;
			continue;  // Skip matched line
		}

		fprintf(f_out, "%s\n", line);  // Write back unmatched lines
	}

	fclose(f_in);
	fclose(f_out);

	// Replace original file if a match was found
	if (match_found) {
		remove(list_path);
		rename("/spiffs/notebook/filenames_temp.txt", list_path);
		printf("Filename '%s' removed from list.\n", filename_clean);
	} else {
		remove("/spiffs/notebook/filenames_temp.txt");
		printf("Filename '%s' not found in list.\n", filename_clean);
	}

	unmount_spiffs();
}
