/*
 * my_spiffs.h
 *
 *  Created on: 17 Apr 2025
 *      Author: hatta
 */

#ifndef COMPONENTS_MY_SPIFFS_MY_SPIFFS_H_
#define COMPONENTS_MY_SPIFFS_MY_SPIFFS_H_

#include "esp_spiffs.h"


#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void mount_spiffs();
void append_to_file(const char *filepath, const char *data);
void print_file_contents(const char *filepath);
void unmount_spiffs();
void format_spiffs();
char* read_file_contents(const char *full_path);
void delete_file(const char *full_path);
void overwrite_file(const char *full_path, const char *data);

#endif /* COMPONENTS_MY_SPIFFS_MY_SPIFFS_H_ */
