#include <config.h>
#include "print_error.h"
#include "api.h"
#include "configuration.h"
#include "neighbours.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

typedef struct holder_t_struct {
	size_t size_offset;
	char* str_data;
} * holder_t;

size_t curl_cb_process_buffer(void* data_buffer, size_t unit_size, size_t unit_count, void* passed_holder_t_data)
{
	size_t buffer_count = 0;
	holder_t holder_t_data;

	if (passed_holder_t_data == NULL) {
		print_error("Unable to determine where to store the data from the server");
		exit(EX_SOFTWARE);
	}
	holder_t_data = (holder_t)passed_holder_t_data;

	if (holder_t_data->size_offset == 0) {
		holder_t_data->str_data = (char*)malloc((unit_size * unit_count) + 1);
	} else {
		holder_t_data->str_data = (char*)realloc(
				holder_t_data->str_data,
				holder_t_data->size_offset + (unit_size * unit_count) + 1);
	}
	if (holder_t_data->str_data == NULL) {
		print_syserror("Unable to allocate memory to store additional data from the server");
		exit(EX_OSERR);
	}

	for (buffer_count = 0; buffer_count < unit_count; buffer_count++) {
		memcpy(
				holder_t_data->str_data + holder_t_data->size_offset,
				(char*)data_buffer + (buffer_count * unit_size),
				unit_size);
		holder_t_data->size_offset += unit_size;
	}
	holder_t_data->str_data[holder_t_data->size_offset] = '\0';

	return buffer_count * unit_size;
}

char* wiomw_login(const char* const str_url, const config_t config)
{
	char* str_session_id;
	char str_error_buffer[CURL_ERROR_SIZE];
	char str_post_json[30 + MAX_USERNAME_LENGTH + MAX_PASSHASH_LENGTH];
	holder_t holder_t_data;
	CURL* curl_handle;

	holder_t_data = (holder_t)malloc(sizeof(struct holder_t_struct));
	if (holder_t_data == NULL) {
		print_syserror("Unable to allocate memory to store the data from the server");
		exit(EX_OSERR);
	}
	holder_t_data->size_offset = 0;
	holder_t_data->str_data = NULL;

	snprintf(
			str_post_json,
			30 + MAX_USERNAME_LENGTH + MAX_PASSHASH_LENGTH,
			"{\"username\":\"%s\",\"password\":\"%s\"}",
			config.str_username,
			config.str_passhash);

	curl_handle = curl_easy_init();
	curl_easy_setopt(curl_handle, CURLOPT_URL, str_url);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, &curl_cb_process_buffer);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, holder_t_data);
	curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, str_error_buffer);
	curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, str_post_json);

	if (curl_easy_perform(curl_handle) == 0) {
		size_t size_data_length;
		if (holder_t_data->size_offset == 0) {
			print_error("Did not receive data from the server");
			exit(EX_PROTOCOL);
		}
		
		size_data_length = strlen(holder_t_data->str_data);
		if (size_data_length > MAX_SESSION_ID_LENGTH) {
			print_error("Session ID received was larger than %d bytes", MAX_SESSION_ID_LENGTH);
			exit(EX_PROTOCOL);
		} else {
			str_session_id = holder_t_data->str_data;
		}
	} else {
		print_error("Login failed: %s", str_error_buffer);
		exit(EX_UNAVAILABLE);
	}

	curl_easy_cleanup(curl_handle);
	free(holder_t_data);
	return str_session_id;
}

/* TODO: make a better prototype for this function. */
void wiomw_get_updates(const char* const str_url, const config_t config, const char* const str_session_id)
{
	/* TODO: functionality */
	if (str_url == NULL) {
		wiomw_send_updates(str_url, config, str_session_id);
	}
}

/* TODO: make a better prototype for this function. */
void wiomw_send_updates(const char* const str_url, const config_t config, const char* const str_session_id)
{
	char str_temp[MAX_SEND_UPDATES_SIZE];
	FILE* fd = fmemopen(str_temp, MAX_SEND_UPDATES_SIZE, "w");
	print_neighbours(fd);
	fclose(fd);
	/* TODO: functionality */
	if (str_url == NULL) {
		wiomw_get_updates(str_url, config, str_session_id);
	}
}

