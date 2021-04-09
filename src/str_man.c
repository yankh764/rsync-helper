/*
*This source code purposes are to provide functions for string manipulations.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "str_man.h"

/*
*This function gets a pointer to a line and then it 
*gets the first word in the line. Reutn -1 if error occured,
*1 if line beginning is nulled and 0 for success.
*/
int get_name(const char *line, char *name_buff, unsigned int buff_size) {
	size_t i; 

	if(isspace(*line)) { //If name starts with white char
		fprintf(stderr, "\nWarning!\n");
		fprintf(stderr, "The line starts with a white character: %s\n", line);
		return -1;
	}
	else if(*line=='\0') //If end of a word
		return 1;
	for(i=0; i<buff_size; i++) {
		if(isspace(line[i]) || line[i]=='\0')
			break;
		name_buff[i] = line[i];
	}
	name_buff[i] = '\0';
	return 0;
}

/*
*This function is going to check if the line ends with any space, 
*and modify it to avoid space sesitivity errors
*/
void clean_line(char *line) {
	size_t i = strlen(line);
	
	for(i=i-1; (isspace(line[i]) && i>0); i--)
		line[i] = '\0';
}

/*
*This function is going to split a paragraph into 
*separated lines, then return a pointer to it. NULL for 
*failure.
*/
char *split_paragraph(const char *paragraph, unsigned int max_size) {
	char *line;
	unsigned int i;
	static unsigned int i_in_par = 0; //Char index in the paragraph 

	if(paragraph[i_in_par]=='\0') { //If the beginning of line is terminated
		i_in_par = 0; //Reinitialize static variable to 0 so itll be usable next call
		return NULL;
	}
	line = (char *) malloc(max_size);
	
	if(line==NULL) {
		fprintf(stderr, "An error occurred while allocating memory.\n");
		return NULL;
	}
	for(i=0; paragraph[i_in_par]!='\n'; i++, i_in_par++) {
		if(paragraph[i_in_par] == '\0') {
			line[i] = '\0';
			return line;
		}	
		line[i] = paragraph[i_in_par];
	}
	line[i] = '\0';
	i_in_par++;
	return line;
}
