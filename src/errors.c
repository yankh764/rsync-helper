/*
*This header is for handling errors 
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "errors.h"

/*
*This function will handle errors that occurres 
*while managing files and prints the right message.
*/
void handle_files_error(const char *message, char *file_path) {
	char full_message[300];

	if(check_input_size(sizeof(full_message), strlen(message)+strlen(file_path))) { //Check the final full_message size
		fprintf(stderr, "The size of the inserted string in errors.c -> handle_files_error is invalid.\n");	
		exit(EXIT_FAILURE);
	}
	strcpy(full_message, message);
	strcat(full_message, file_path);
	perror(full_message);
	exit(EXIT_FAILURE);
}

/*
*This function will check if the size of the input is 
*valid and can fit in the array without overflowing it.
*0 for valid, 1 otherwise.
*/
int check_input_size(int valid_size, int input_size) {
	if(input_size < valid_size)
		return 0;
	else 
		return 1;
}

/*
*This function will handle general errors and display 
*the relevant message.
*/
void handle_general_error(const char *message) {
	fprintf(stderr, "%s\n", message);
	exit(EXIT_FAILURE);
}

/*
*This function will handle errors for the strstr() function 
*and it will display the write message if a string wasnt found.
*/
void handle_strstr_error(const char *serched_str) {
	fprintf(stderr, "Error occurred while serching for unit : %s.\n", serched_str);
}