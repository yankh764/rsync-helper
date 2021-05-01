/*
*This source file contains all required functions to manage configuration files.
*/ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include "str_man.h"
#include "config_man.h"

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
		"# If a unit is optional you can leave it empty and the program #\n"
		"# will skip it.                                                #\n"
		"# Lastly as you can see hashes ('#') are ignored.              #\n"
		"# By the way, sorry for the hard syntax I really tried to make #\n"
		"# as easy as I can and that's the result. I hope you like it :)#\n"
		"################################################################\n"
	};
	
	fp = fopen(file_path, "w");
	
	if(fp == NULL) { //Check if error occurred
		if(errno==EACCES)
			fprintf(stderr, "Permission denied error occured while creating: %s\n", file_path);
		else
			fprintf(stderr, "Error occurred while creating: %s\n", file_path);
		return -1;
	}
	
	fprintf(fp, "%s%s\n", description, config_instruct);
	
	if(ferror(fp)) {
		fprintf(stderr, "Error occurred while writing to: %s\n", file_path);
		return -1;
	}
	if(fclose(fp)) {
		fprintf(stderr, "Error occurred while closing: %s\n", file_path);
		return -1;
	}
	return 0;
}

/*
*This function takes a unit name and a description then it'll 
*write the description to it so youll know how to configure it.
*/
int write_config_unit(const char *file_path, const char *unit_name, 
		const char *unit_desc, const char *special_note) {
	FILE *fp;

	fp = fopen(file_path, "a");

	if(fp == NULL) {//Check if error occurred
		fprintf(stderr, "Error occurred while opening: %s\n", file_path);
		return -1;
	}
	if(special_note != NULL) {
		fprintf(fp, "#%s\n", special_note);
		if(ferror(fp)) {
			fprintf(stderr, "Error occured while writing to: %s\n", file_path);
			return -1;
		}
	}
	fprintf(fp, "%s = %s\n\n", unit_name, unit_desc);
	if(ferror(fp)) {
		fprintf(stderr, "Error occurred while writing to: %s\n", file_path);
		return -1;
	}
	if(fclose(fp)) {
		fprintf(stderr, "Error occurred while closing: %s\n", file_path);
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
	char buffer[UNITS_SIZE]; //Buffer for the each lines 
	char *configs_beginning; //Pointer to the targeted unit's configs beginning
	char *configs; //Already read configs
	char unit_to_check[200]; //Unit name to check 
	unsigned int list = 0; //1 = list, 0 = one line configurations
	unsigned int status;

	fp = fopen(file_path, "r");
	
	if(fp == NULL) { //Check if error occurred
		fprintf(stderr, "Error occured while opening file: %s\n", file_path);
		return NULL;
	}
	configs = (char *) calloc(CONFIGS_SIZE, sizeof(char));
	
	if(configs==NULL) {
		fprintf(stderr, "Error occurred while allocating memory.\n");
		if(fclose(fp))
			fprintf(stderr, "Error occurred while closing: %s\n", file_path);
		return NULL;
	}

	//Loop in the file's content 
	while(fgets(buffer, sizeof(buffer), fp) != NULL) { 
		if(ferror(fp)) {
			fprintf(stderr, "Error occurred while reading: %s\n", file_path);
			free(configs);
			if(fclose(fp))
				fprintf(stderr, "Error occurred while closing: %s\n", file_path);
			return NULL;
		}
		//If the beginning of a line is commented or empty
		if(*buffer=='#' || *buffer=='\n' || *buffer=='\0') 
			continue;
		if(list) { //If list was found
			status = read_list_syntax(buffer, configs);

			if(status == 1)
				continue; //read the next line of the list
			else if(status == -1) { //If overflow is detected 
				free(configs);
				fprintf(stderr, "Please make sure that the unit '%s' is  configured properly.\n", unit_name);
				if(fclose(fp))
					fprintf(stderr, "Error occured while closing: %s\n", file_path);
				return NULL;
			}
			if(fclose(fp)) {
				free(configs);
				fprintf(stderr, "Error occurred while closing: %s\n", file_path);
				return NULL;
			}
			return configs;
		}
		else {
			if(isspace(*buffer)) //To ignore list's indentions
				continue;
			//Get unit name from the buffer to compare it
			if(get_name(buffer, unit_to_check, sizeof(unit_to_check))) { 
				free(configs);
				if(fclose(fp))
					fprintf(stderr, "Error occurred while closing: %s\n", file_path);
				return NULL;
			}
			if(strcmp(unit_name, unit_to_check)) //If unit wasnt found in line read the next one
				continue;
			/*The address of the beginning of the unit's configurations =
			beginning of the line + len of the unit name + 3 bytes (2 spaces and '=' sign)*/
			configs_beginning = buffer + strlen(unit_name) + 3;
			
			if(*configs_beginning == '{') { //If unit configs is beginning of a list
				list = 1;
				continue;
			}
			else if(isspace(*configs_beginning)) { //If configurations are empty 
				if(fclose(fp)) {
					free(configs);
					fprintf(stderr, "Error occured while closing: %s\n", file_path);
					return NULL;
				}
				return configs;
			}	
			else {
				if(read_reg_syntax(configs_beginning, configs)) {
					free(configs);
					fprintf(stderr, "Please make sure that the unit '%s' is  configured properly.\n", unit_name);
					if(fclose(fp))
						fprintf(stderr, "Error occured while closing: %s\n", file_path);
					return NULL;
				}
			
				if(fclose(fp)) {
					free(configs);
					fprintf(stderr, "Error occurred while closing: %s\n", file_path);
					return NULL;
				}
				return configs;
			}
		}
	}
	free(configs);
	if(fclose(fp))
		fprintf(stderr, "Error occurred while closing: %s\n", file_path);
	fprintf(stderr, "Error occured while searching for unit: %s.\n", unit_name);
	return NULL;
}

/*
*This function reads the regular configurations syntax,
*Then passes it to the config_buffer.
*/
int read_reg_syntax(const char *config_beginning, char *configs_buffer) {
	unsigned int i;

	for(i=0; i<CONFIGS_SIZE; i++)
		switch(config_beginning[i]) {
			case '\n':
				configs_buffer[i] = '\0';
				return 0;
			case '\0': 
				return 0;
			default:
				//insert char into the configs_buffer
				configs_buffer[i] = config_beginning[i];
				break;
			}
	fprintf(stderr, "Buffer overflow was detected!\n");
	return -1;
}

/*
*This function will read the syntax of a list, if finished reading itll 
*return 0, if still reading it will return 1 to read the next line.
*/
int read_list_syntax(const char *line_beginning, char *configs_buffer) {
	unsigned int i;
	size_t char_cnt = strlen(configs_buffer); //number of characters in configs_buffer

	for(i=0; char_cnt<CONFIGS_SIZE; i++) //Read until reaching '\n' or ';'
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
	fprintf(stderr, "Buffer overflow was detected!\n");
	return -1;
}

