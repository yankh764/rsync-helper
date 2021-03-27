/*
*This header contains all required functions to manage configuration files.
*/ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include "config_man.h"

#define CONFIGS 1001 //Size of read_configs
#define UNITS 700 //Size of unit_buffer

/*
*A function to create a configuration file with the given path.
*Then it'll write a comentted discription and instruction for this file.
*/ 
int create_config_file(const char *file_path, const char *description) {
	FILE *fp;
	const char *config_instruct = {
		"################################################################\n"
		"#--------------------------------------------------------------#\n"
		"#                        [Instructions]                        #\n"
		"#--------------------------------------------------------------#\n"
		"################################################################\n"
		"# Please it's very important to make sure there are 3 charact- #\n"
		"# ers between the unit name and its configurations.            #\n"
		"# For example:                                                 #\n"
		"#                                                              #\n"
		"# UnitName = configurations                                    #\n"
		"#                                                              #\n"
		"# As you can see there are 3 characters between the unit name  #\n"
		"# and the configurations (2 spaces and an equal sign). Be awa- #\n"
		"# re that each new line terminates the unit's configurations.  #\n"
		"# If the unit's configurations are too long you can put it in- #\n"
		"# side a pair of braces and make it a list. For example:       #\n"
		"#                                                              #\n"
		"# VeryLongUnit = {                                             #\n"
		"#    config_1                                                  #\n"
		"#    config_2                                                  #\n"
		"#    config_3                                                  #\n"
		"#    config_4                                                  #\n"
		"# }  <- Don't forget to close the list                         #\n"
		"#                                                              #\n"
		"# Be aware that '}' terminates the list.                       #\n"
		"# Note: Please if you want to use indention for the list, only #\n"
		"# use tabs, becuase they are ignored while reading a list.     #\n"
		"# It's also good to note that in case the unit is a commands   #\n"
		"# unit and this command requires root privileges, you can just #\n"
		"# prepend 'sudo' to it like this:                              #\n"
		"#                                                              #\n"
		"# CommandsUnit = sudo root_command                             #\n"
		"#                                                              #\n"
		"# Lastly as you can see hashes ('#') are ignored.              #\n"
		"# By the way, sorry for the hard syntax I really tried to make #\n"
		"# as easy as I can and that's the result. I hope you like it :)#\n"
		"################################################################\n"
	};

	fp = fopen(file_path, "w");
	if(fp == NULL) { //Check if error occurred
		fprintf(stderr, "An error occurred while creating: %s\n", file_path);
		return -1;
	}

	fprintf(fp, "%s\n%s\n", description, config_instruct);
	if(ferror(fp)) {
		fprintf(stderr, "An error occurred while creating: %s\n", file_path);
		return -1;
	}

	if(fclose(fp)==EOF) {
		fprintf(stderr, "An error occurred while closing: %s\n", file_path);
		return -1;
	}
	return 0;
}

/*
*This function takes a unit name and a description then it'll 
*write the description to it so youll know how to configure it.
*/
int write_config_unit(const char *file_path, const char *unit_name, const char *unit_desc) {
	FILE *fp;

	fp = fopen(file_path, "a");
	if(fp == NULL) {//Check if error occurred
		fprintf(stderr, "An error occurred while writing to: %s\n", file_path);
		return -1;
	}

	fprintf(fp, "%s = %s\n\n", unit_name, unit_desc);
	if(ferror(fp)) {
		fprintf(stderr, "An error occurred while writing to: %s\n", file_path);
		return -1;
	}

	if(fclose(fp)==EOF) {
		fprintf(stderr, "An error occurred while closing: %s\n", file_path);
		return -1;
	}
	return 0;	
}

/*
*This function will read all the config file, it'll
*ignore every commented line (with '#'), it will search 
*for the desired unit's confgis. If the unit exists it will 
*return a pointer to it, otherwise it will return NULL.
*/
char *read_config_unit(const char *file_path, const char *unit_name) {
	FILE *fp;
	char unit_buffer[UNITS]; //Buffer for the unit name and its configs 
	char *configs_beginning; //Pointer to the targeted unit's configs beginning
	char *read_configs; //Already read configs
	char unit_to_check[200]; //Unit name to check 
	unsigned int config_status = 0; //1 = list, 0 = one line configurations

	fp = fopen(file_path, "r");
	if(fp == NULL) { //Check if error occurred
		fprintf(stderr, "An error occured while opening file: %s\n", file_path);
		return NULL;
	}

	read_configs = (char *) calloc(CONFIGS, sizeof(char));
	if(read_configs==NULL) {
		fprintf(stderr, "An error occurred while allocating memory.\n");
		if(fclose(fp)==EOF)
			fprintf(stderr, "An error occurred while closing: %s\n", file_path);
		return NULL;
	}

	//Loop in the file's content 
	while(fgets(unit_buffer, sizeof(unit_buffer), fp) != NULL) { 
		if(ferror(fp)) {
			fprintf(stderr, "An error occurred while reading: %s\n", file_path);
			free(read_configs);
			if(fclose(fp)==EOF)
				fprintf(stderr, "An error occurred while closing: %s\n", file_path);
			return NULL;
		}

		//If the beginning of a line is commented or empty
		if(*unit_buffer=='#' || *unit_buffer=='\n' || *unit_buffer=='\0') 
			continue; //Ignore and read next line

		if(config_status) { //If list was found
			if(read_list_syntax(unit_buffer, read_configs))
				continue; //read the next line of the list
			if(fclose(fp)==EOF) {
				free(read_configs);
				fprintf(stderr, "An error occurred while closing: %s\n", file_path);
				return NULL;
			}
			return read_configs;
		}

		else {
			if(isspace(*unit_buffer)) //To ignore list's indentions
				continue;
			//Get unit name from the buffer to compare it
			if(get_name(unit_buffer, unit_to_check, sizeof(unit_to_check))==-1) { 
				free(read_configs);
				if(fclose(fp)==EOF)
					fprintf(stderr, "An error occurred while closing: %s\n", file_path);
				return NULL;
			}
			if(strcmp(unit_name, unit_to_check)) //If unit wasnt found in line
				continue; //Read the next one 
			/*The address of the beginning of the unit's configurations =
			beginning of the line + len of the unit name + 3 bytes (2 spaces and '=' sign)*/
			configs_beginning = unit_buffer + strlen(unit_name) + 3;
			if(*(configs_beginning) == '{') { //If unit configs is beginning of a list
				config_status = 1;
				continue; 
			}
			else {
				read_reg_syntax(configs_beginning, read_configs);
				
				if(fclose(fp)==EOF) {
					free(read_configs);
					fprintf(stderr, "An error occurred while closing: %s\n", file_path);
					return NULL;
				}
				return read_configs;
			}
		}
	}
	free(read_configs);
	if(fclose(fp)==EOF)
		fprintf(stderr, "An error occurred while closing: %s\n", file_path);
	fprintf(stderr, "Error occured while searching for unit: %s.\n", unit_name);
	return NULL;
}

/*
*This function reads the regular configurations syntax,
*Then passes it to the config_buffer. If there was a buffer 
*overflow the function will return -1, otherwise 0.
*/
int read_reg_syntax(const char *config_beginning, char *configs_buffer) {
	unsigned int i;

	for(i=0; config_beginning[i]!='\0'; i++)
		switch(config_beginning[i]) {
			case '\n':
				configs_buffer[i] = '\0'; //teminate line
				return 0; //exit
			default:
				//insert char into the configs_buffer
				configs_buffer[i] = config_beginning[i];
				break;
			}
	return 0;
}

/*
*This function will read the syntax of a list, if finished reading itll 
*return 0, if still reading it will return 1 to read the next line.
*/
int read_list_syntax(const char *line_beginning, char *configs_buffer) {
	unsigned int i;
	size_t char_cnt = strlen(configs_buffer); //number of characters in configs_buffer

	for(i=0; line_beginning[i]!='\0'; i++) //Loop and read line until reaching '\n' or ';'
		switch(line_beginning[i]) {
			case '}': //If end of list
				configs_buffer[char_cnt] = '\0'; //terminate line
				return 0; //Finished reading list
			case '\t': //Ignore indention 
				break;
			case '\n':
				configs_buffer[char_cnt] = line_beginning[i];
				return 1; //Read next line
			default:
				configs_buffer[char_cnt] = line_beginning[i];
				char_cnt++;
				break;
		}
	return 0; //Finished reading list
}

/*
*This function is going to split a read list of configurations
*into a separated lines, then return a pointer to it. NULL for 
*failure.
*/
char *split_configs(const char *configs) {
	char *line;
	unsigned int i;
	static int i_in_configs = 0;

	if(configs[i_in_configs]=='\0') { //If the beginning of configs is terminated
		i_in_configs = 0; //Reinitialize static variable to 0 so itll be usable next call
		return NULL;
	}

	line = calloc(UNITS, sizeof(char));
	if(line==NULL) {
		fprintf(stderr, "An error occurred while allocating memory.\n");
		return NULL;
	}

	for(i=0; configs[i_in_configs]!='\0'; i++) { 
		if(configs[i_in_configs]=='\n') {
			i_in_configs++; //Skip it and break
			break;
		}
		line[i] = configs[i_in_configs];
		i_in_configs++;
	}
	return line;
}

/*
*This function gets a pointer to a line and then it 
*gets the first word in the line .
*/
int get_name(const char *line, char *name_buff, int buff_size) {
	size_t i; 

	if(isspace(*line)) { //If name starts with invalid char 
		fprintf(stderr, "\nWarning!\n");
		fprintf(stderr, "The line starts with a white space character: %s\n", line);
		return -1;
	}

	else if(*line=='\0')
		return -1;

	for(i=0; i<buff_size; i++) {
		if(isspace(line[i])) {
			name_buff[i] = '\0';
			break;
		}
		name_buff[i] = line[i];
	}
	return 0;
} 

/*
*This function is going to check if the line ends with any space, 
*and modify it to avoid space sesitivity errors
*/
void clean_line(char *line) {
	unsigned int i;
	size_t len = strlen(line);

	i = 1;
	while(isspace(line[len-i])) { //Clean line from spaces at the end
		line[len-i] = '\0';
		i++;
	}
}