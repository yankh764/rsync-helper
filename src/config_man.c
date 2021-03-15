/*
*This header contains all required functions to manage configuration files.
*/ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "errors.h" //Header for error handling
#include "config_man.h"

/*
*The function will check if the given file in the 
*file path exists. It retruns 1 for yes and 0 for no.
*/
int check_file_existence(char *file_path) {
	FILE *fp;

	fp = fopen(file_path, "r");
	if(fp == NULL) //if file doesnt exists 
		return 0;
	fclose(fp);
	return 1;
}

/*
*A function to create a configuration file with the given path.
*Then it'll write a comentted discription and instruction for this file.
*/ 
void create_config_file(char *file_path, const char *description) {
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
		"# If the list is group of commands you can add '&&' to the end #\n"
		"# of each line. If the command requires root privileges you    #\n"
		"# can just add sudo to the command. For example:               #\n"
		"#                                                              #\n"
		"# CommandsUnit = {                                             #\n"
		"#    command 1 &&                                              #\n"
		"#    sudo root_command 2 &&                                    #\n"
		"#    command 3 &&                                              #\n"
		"#    sudo root_command 3 &&                                    #\n"
		"#    command 4                                                 #\n"
		"# }                                                            #\n"
		"#                                                              #\n"
		"# Be aware that every new line in the list syntax is converted #\n"
		"# into a space, and '}' terminates the list.                   #\n"
		"# Note: Please if you want to use indention for the list, only #\n"
		"# use tabs, becuase they are ignored while reading a list.     #\n"
		"# Lastly as you can see hashes ('#') are ignored.              #\n"
		"# By the way, sorry for the hard syntax I really tried to make #\n"
		"# as easy as I can and that's the result. I hope you like it :)#\n"
		"################################################################\n"
	};

	fp = fopen(file_path, "w");
	if(fp == NULL) //Check if error occurred
		handle_files_error("An error occurred in create_config_file() while creating ", file_path);
	fprintf(fp, "%s\n", description);
	fprintf(fp, "%s\n", config_instruct);
	fclose(fp);
}

/*
*This function takes a unit name and a description then it'll 
*write the description to it so youll know how to configure it.
*/
void write_config_unit(char *file_path, const char *unit_name, const char *unit_desc) {
	FILE *fp;

	fp = fopen(file_path, "a");
	if(fp == NULL) //Check if error occurred
		handle_files_error("An error occurred in write_config_unit() while opening ", file_path);
	fprintf(fp, "%s", unit_name);
	fprintf(fp, "%s", " = ");
	fprintf(fp, "%s;\n", unit_desc);
	fclose(fp);	
}

/*
*This function will read all the config file, it'll
*ignore every commented line (with '#'), it will search 
*for the desired unit's confgis. If the unit exists it will 
*return a pointer to it, otherwise it will return NULL.
*/
char *read_config_unit(char *file_path, const char *unit_name) {
	FILE *fp;
	char unit_buffer[400]; //Buffer for the unit name and its configs 
	char *configs_beginning; //Pointer to the targeted unit's configs beginning
	static char read_configs[500]; //Array for the read configs
	unsigned int config_status = 0; //1 = list, 0 = one line configurations

	fp = fopen(file_path, "r");
	if(fp == NULL) //Check if error occurred
		handle_files_error("An error occurred in read_config_file() while reading ", file_path);
	memset(read_configs, '\0', sizeof(read_configs)); //Make sure the static array is empty 
	//Loop in the file's content 
	while(fgets(unit_buffer, 400, fp) != NULL) { 
		//If the beginning of a line is commented or empty
		if(*unit_buffer=='#' || *unit_buffer=='\n' || *unit_buffer=='\0') 
			continue; //Ignore and read next line
		if(config_status) { //If list was found
			if(read_list_syntax(unit_buffer, read_configs, sizeof(read_configs))) //If reading list
				continue; //read the next line of the list
			fclose(fp);
			return read_configs;
		}
		else {
			if(check_unit_existence(unit_name, unit_buffer))  //If unit wasnt found in line
				continue; //Read the next one 
			/*The address of the beginning of the unit's configurations =
			beginning of the line + len of the unit name + 3 bytes (2 spaces and '=' sign)*/
			configs_beginning = unit_buffer + strlen(unit_name) + 3;
			if(*(configs_beginning) == '{') { //If unit configs is beginning of a list
				config_status = 1;
				continue; 
			}
			else {
				read_reg_syntax(configs_beginning, read_configs, sizeof(read_configs));
				fclose(fp);
				return read_configs;
			}
		}
	}

	fclose(fp);
	return NULL;
}

/*
*This function reads the regular configurations syntax,
*Then passes it to the config_buffer.
*/
void read_reg_syntax(char *config_beginning, char *configs_buffer, int buffer_size) {
	int i = 0;

	while(1) 
		switch(*(config_beginning+i)) {
			case '\n': //If line is terminated
				*(configs_buffer+i) = '\0';
				return; //exit
			default:
				//Check for overflow
				if(check_input_size(buffer_size, i+1)) {
					fprintf(stderr, "The size of the inserted string in config_man.c -> read_reg_syntax is invalid.\n");
					exit(EXIT_FAILURE);
				}
				//insert char into the configs_buffer
				*(configs_buffer+i) = *(config_beginning+i);
				i++;
				break;
			}
}

/*
*This function will read the syntax of a list, if finished reading itll 
*return 0, if still reading it will return 1 to read the next line.
*/
int read_list_syntax(char *line_beginning, char *configs_buffer, int buffer_size) {
	int char_cnt, i;

	char_cnt = strlen(configs_buffer); //number of characters in configs_buffer
	i = 0;  
	while(1) { //Loop and read line until reaching '\n' or ';'
		switch(*(line_beginning+i)) {
			case '}': //If line is terminated
				*(configs_buffer+char_cnt) = '\0';
				return 0; //Finished reading list
			case '\t': //Ignore indention 
				break;
			case '\n': //Convert to one space 
				//Make sure there is no overflow
				if(check_input_size(buffer_size, char_cnt+1)) {
					fprintf(stderr, "The size of the inserted string in config_man.c -> read_list_syntax is invalid.\n");
					exit(EXIT_FAILURE);
				}
				*(configs_buffer+char_cnt) = ' ';
				return 1; //Read next line
			default:
				//Make sure there is no overflow
				if(check_input_size(buffer_size, char_cnt+1)) {
					fprintf(stderr, "The size of the inserted string in config_man.c -> read_list_syntax is invalid.\n");
					exit(EXIT_FAILURE);
				}
				*(configs_buffer+char_cnt) = *(line_beginning+i);
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
*0 = equal, 1 = not equal
*/
int check_unit_existence(const char *unit_name, char *line_begin) {
	unsigned int i;
	char unit_to_check[50]; //Array for the unit name that needs to be checked

	//Get unit name
	for(i=0; line_begin[i]!='\n' && line_begin[i]!='\t' && line_begin[i]!=' ' && line_begin[i]!='\0'; i++) {
		//Make sure there is no overflow
		if(check_input_size(sizeof(unit_to_check), i+1)) {
					fprintf(stderr, "The size of the inserted string in config_man.c -> check_unit_existence is invalid.\n");
					exit(EXIT_FAILURE);
				} 
		unit_to_check[i] = *(line_begin+i);
	}
	if(strcmp(unit_name, unit_to_check) == 0) //If equal 
		return 0;
	else //Not equal
		return 1;
}
