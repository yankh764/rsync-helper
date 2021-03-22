/*
*This header contains all required functions to manage configuration files.
*/ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config_man.h"

/*
*The function will check if the given file in the 
*file path exists. It retruns 1 for yes and 0 for no.
*/
int check_file_existence(char *file_path) {
	FILE *fp;

	fp = fopen(file_path, "r");
	if(fp == NULL) { //if file doesnt exists 
		fprintf(stderr, "Error occured due to missing configurations file.\n");
		return 0;
	}
	fclose(fp);
	return 1;
}

/*
*A function to create a configuration file with the given path.
*Then it'll write a comentted discription and instruction for this file.
*/ 
int create_config_file(char *file_path, const char *description) {
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
		fprintf(stderr, "An error occurred while creating: %s.\n", file_path);
		return -1;
	}
	fprintf(fp, "%s\n", description);
	fprintf(fp, "%s\n", config_instruct);
	fclose(fp);
	return 0;
}

/*
*This function takes a unit name and a description then it'll 
*write the description to it so youll know how to configure it.
*/
int write_config_unit(char *file_path, const char *unit_name, const char *unit_desc) {
	FILE *fp;

	fp = fopen(file_path, "a");
	if(fp == NULL) {//Check if error occurred
		fprintf(stderr, "An error occurred while writing to: %s\n", file_path);
		return -1;
	}
	fprintf(fp, "%s = %s\n\n", unit_name, unit_desc);
	fclose(fp);
	return 0;	
}

/*
*This function will read all the config file, it'll
*ignore every commented line (with '#'), it will search 
*for the desired unit's confgis. If the unit exists it will 
*return a pointer to it, otherwise it will return NULL.
*/
char *read_config_unit(char *file_path, const char *unit_name) {
	FILE *fp;
	char unit_buffer[600]; //Buffer for the unit name and its configs 
	char *configs_beginning; //Pointer to the targeted unit's configs beginning
	char *read_configs; //Array for the read configs
	unsigned int config_status = 0; //1 = list, 0 = one line configurations
	int status;

	fp = fopen(file_path, "r");
	if(fp == NULL) { //Check if error occurred
		fprintf(stderr, "An error occurred reading: %s\n", file_path);
		return NULL;
	}
	read_configs = (char *) calloc(801, sizeof(char));
	if(read_configs==NULL) {
		fprintf(stderr, "An error occurred while allocating memory.\n");
		return NULL;
	}
	//Loop in the file's content 
	while(fgets(unit_buffer, sizeof(unit_buffer), fp) != NULL) { 
		//If the beginning of a line is commented or empty
		if(*unit_buffer=='#' || *unit_buffer=='\n' || *unit_buffer=='\0') 
			continue; //Ignore and read next line
		if(config_status) { //If list was found
			if((status=read_list_syntax(unit_buffer, read_configs, 801)) == -1) {
				fprintf(stderr, "An overflow was detected while reading unit: %s.\n", unit_name);
				fclose(fp);
				free(read_configs);
				return NULL;
			}
			else if(status) //If reading list
				continue; //read the next line of the list
			fclose(fp);
			return read_configs;
		}
		else {
			status=check_unit_existence(unit_name, unit_buffer);
			
			if(status)  //If unit wasnt found in line
				continue; //Read the next one 
			/*The address of the beginning of the unit's configurations =
			beginning of the line + len of the unit name + 3 bytes (2 spaces and '=' sign)*/
			configs_beginning = unit_buffer + strlen(unit_name) + 3;
			if(*(configs_beginning) == '{') { //If unit configs is beginning of a list
				config_status = 1;
				continue; 
			}
			else {
				if(read_reg_syntax(configs_beginning, read_configs, 801) == -1) { 
					fprintf(stderr, "An overflow was detected while reading unit: %s.\n", unit_name);
					fclose(fp);
					free(read_configs);
					return NULL;
				}
				fclose(fp);
				return read_configs;
			}
		}
	}

	fclose(fp);
	free(read_configs);
	fprintf(stderr, "Error occured while searching for unit: %s.\n", unit_name);
	return NULL;
}

/*
*This function reads the regular configurations syntax,
*Then passes it to the config_buffer. If there was a buffer 
*overflow the function will return -1, otherwise 0.
*/
int read_reg_syntax(char *config_beginning, char *configs_buffer, int buffer_size) {
	int i = 0;

	while(1) 
		switch(config_beginning[i]) {
			case '\n':
				if(i+1 > buffer_size) 
					return -1;
				configs_buffer[i] = '\0'; //teminate line
				return 0; //exit
			case '\0': //Line is terminated
				return 0; //exit
			default:
				//Check for overflow
				if(i+1 > buffer_size) 
					return -1;
				//insert char into the configs_buffer
				configs_buffer[i] = config_beginning[i];
				i++;
				break;
			}
}

/*
*This function will read the syntax of a list, if finished reading itll 
*return 0, if still reading it will return 1 to read the next line, if 
*overflow occured -1 is returned.
*/
int read_list_syntax(char *line_beginning, char *configs_buffer, int buffer_size) {
	int char_cnt, i;

	char_cnt = strlen(configs_buffer); //number of characters in configs_buffer
	i = 0;  
	while(1) { //Loop and read line until reaching '\n' or ';'
		switch(line_beginning[i]) {
			case '}': //If end of list
				configs_buffer[char_cnt] = '\0'; //terminate line
				return 0; //Finished reading list
			case '\t': //Ignore indention 
				break;
			case '\n':
				//Make sure there is no overflow
				if(char_cnt+1 > buffer_size)
					return -1;
				configs_buffer[char_cnt] = line_beginning[i];
				return 1; //Read next line
			default:
				//Make sure there is no overflow
				if(char_cnt+1 > buffer_size)
					return -1;
				configs_buffer[char_cnt] = line_beginning[i];
				char_cnt++;
				break;
		}
		i++;
	}
}

/*
*This function takes a unit name and pointer to the beginning
*of the unit's line and it then it finds the unit's name which is
*the first word then it checks if the founded unit and the passed 
*one are equal and the same.
*0 = equal, 1 = not equal, -1 error.
*/
int check_unit_existence(const char *unit_name, char *line_begin) {
	unsigned int i;
	int unit_name_len = strlen(unit_name);
	char unit_to_check[unit_name_len]; //Array for the unit name that needs to be checked

	for(i=0; i<unit_name_len+1; i++) {
		if(line_begin[i]==' ' || line_begin[i]=='\t' || line_begin[i]=='\n') {
			unit_to_check[i] = '\0';
			break;
		}
		unit_to_check[i] = line_begin[i];
	}
	if(strcmp(unit_name, unit_to_check) == 0)
		return 0;
	else //Not equal
		return 1;
}