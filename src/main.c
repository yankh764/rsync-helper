/*
*Rsync helper and a time saver. This program will automate
*the backup process for you and some of the housekeeping.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "config_man.h" //For config files managment 
#include "str_man.h" //for string and line manipulation 
#include "sys_backup.h"

int main(int argc, char *argv[]) {
	const unsigned int max_args = 8; //Maximum number of arguments commands_list can take
	const char *config_desc = {
		"################################################################\n"
		"# This configuration file purposes are to provide the backup   #\n"
		"# program the right cleaning process and storage device path,  #\n"
		"# customized to your own needs and preferences.                #\n"
		"################################################################\n"
	};
	const char *units_list[] = {
		"DirsToClean", "Commands", "DevicePath", 
		"RsyncOpt", "DirsToBackup", NULL
	};
	const char *units_desc[] = {
		"{\n"
		"\tDirs path you regularly clean, like some cache dirs\n"
		"\tPlease be aware in case the path is something like this:\n"
		"\t/path/to/dir\n"
		"\tIt will delete the dir and its content, so if you want\n"
		"\tto keep the dir just append a slash to the path.\n"
		"}",
		"{\n"
		"\tCommands that you usually execute before system backup.\n"
		"\tFor example cleaning commands or generating packages list.\n"
		"}", 
		"Your storage device's path",
		"Rsync backup option, for example: -aAXHv",
		"Directories to backup", NULL
	};
	const char *config_path = ".config/sys_backup";
	const char *home = getenv("HOME");
	char config_full_path[strlen(home)+strlen(config_path)+1];
	char *configurations, *backup_path, *ptr, *note;
	char *command_list[max_args], dir[200];
	int opt, status, c;
	unsigned int i, len, dir_removal_status;
	date backup_dir_name;

	if(getuid()==0) { //If user is a root or using root priviliges
		fprintf(stderr, "Error: Running the program with root privileges is forbidden for security concerns.\n");
		exit(EXIT_FAILURE);
	}
	if(argc > 2) {
		fprintf(stderr, "Too many arguments were given.\n");
		exit(EXIT_FAILURE);
	}

	/*Prepare configurations file's full path*/
	snprintf(config_full_path, sizeof(config_full_path)+1, "%s/%s", home, config_path);

	while((opt = getopt(argc, argv, ":ch")) != EOF) {
		switch(opt) {
			case 'c':
				if(create_config_file(config_full_path, config_desc)) 
					exit(EXIT_FAILURE);
				for(i=0; units_list[i]!=NULL; i++) {
					switch(i) {
						case 0: case 1:
							note = "Optional section";
							break;
						default:
							note = NULL;
							break;
					}
				if(write_config_unit(config_full_path, units_list[i], units_desc[i], note)) 
					exit(EXIT_FAILURE);
				}
				printf("%s was succesfully generated.\n", config_full_path);
				return 0;
			case 'h':
				help(argv[0]);
				return 0;
			default:
				fprintf(stderr, "The argument %s is invalid.\n", argv[1]);
				exit(EXIT_FAILURE);
		}		
	}
	
	//DirsToClean (optional section)
	if((configurations=read_config_unit(config_full_path, units_list[0])) == NULL) {
		if(errno==ENOENT) { //If file doesnt exists
			fprintf(stderr, "The configurations file doesn't exists.\n");
			fprintf(stderr, "\nReminder: you can always generate new one using the -c option.\n");	
		}
		exit(EXIT_FAILURE);
	}
	//Separate each dir path on its own to delete it
	if(*configurations == '\0') //If configurations are empty
		printf("Omitting '%s' section!\n", units_list[0]);
	else {
		while((ptr=split_paragraph(configurations, UNITS))!=NULL) {
			clean_line(ptr);
	
			if(*ptr=='/' && ptr[1]=='\0'){ //If path is stand alone root tree
				fprintf(stderr, "Error: you can't remove root tree.\n");
				fprintf(stderr, "Please make sure that there is no sign of stand alone '/' in your configs.\n");
				free(configurations);
				free(ptr);
				exit(EXIT_FAILURE);
			}
			//If path ends with a slash remove its content only, otherwise remove it also.
        	dir_removal_status = (ptr[strlen(ptr)-1] == '/') ? 0 : 1;
        
			if((status=rm_dir(ptr, dir_removal_status)) == -1) {
				free(configurations);
				free(ptr);
				exit(EXIT_FAILURE);
			}
			else if(status) //If dir doesnt exist 
				;
			free(ptr);
		}
		if(errno==ENOMEM) { //If error is memory error (malloc error)
			free(configurations);
			exit(EXIT_FAILURE);
		}
		free(ptr);
	}
	free(configurations);

	//Commands (optional section)
	if((configurations=read_config_unit(config_full_path, units_list[1])) == NULL) {
		if(errno==ENOENT) { //If file doesnt exists
			fprintf(stderr, "The configurations file doesn't exists.\n");
			fprintf(stderr, "Reminder: you can always generate new one using the -c option.\n");
		}
		exit(EXIT_FAILURE);
	}
	if(*configurations=='\0')
		printf("Omitting '%s' section!\n", units_list[1]);
	else {
		/*Separate each cleaning command on its own to excute it*/
		memset(command_list, '\0', sizeof(command_list)); //Make sure array is empty
    
		while((ptr=split_paragraph(configurations, UNITS))!=NULL) {
			clean_line(ptr);
	
			if(prepare_command(ptr, command_list, max_args)) {
				free(ptr);
				free(configurations);
				exit(EXIT_FAILURE);
			}
			if(strcmp(command_list[0], "sudo")==0) {
				printf("\nDo you want to execute the command '%s' with root privileges (y/n): ", ptr);
				
				while((c=getchar()) != EOF) {
					getchar(); //Eat newline 
					
					if(c=='y')
						goto execute;
					else if(c=='n') {
						printf("Ommiting the command '%s'.\n\n", ptr);
						free(ptr);
						for(i=0; command_list[i]!=NULL; i++)
							free(command_list[i]);
						break;
					}
					else {
						printf("Please answer with y or n!\n");
						printf("\nDo you want to execute the command '%s' with root privileges (y/n): ", ptr);
						continue;
					}
				}
				continue;
			}
			execute: ;

			status = exec_command(command_list[0], command_list);

			free(ptr);
			for(i=0; command_list[i]!=NULL; i++)
				free(command_list[i]);

			if(status) {
				free(configurations);
				exit(EXIT_FAILURE);
			}	
		}	
		if(errno==ENOMEM) { //If error is memory error (malloc error)
			free(configurations);
			exit(EXIT_FAILURE);
		}
		free(ptr);
	}
	free(configurations);

	//DevicePath (requierd)
	if((configurations=read_config_unit(config_full_path, units_list[2])) == NULL) {
		if(errno==ENOENT) { //If file doesnt exists
			fprintf(stderr, "The configurations file doesn't exists.\n");
			fprintf(stderr, "Reminder: you can always generate new one using the -c option.\n");	
		}
		exit(EXIT_FAILURE);
	}
	if(*configurations == '\0') { //If configurations are empty 
		free(configurations);
		fprintf(stderr, "Error: The mandatory section '%s' is empty!\n", units_list[2]);
		exit(EXIT_FAILURE);
	}
	get_date(&backup_dir_name); //Get the date of today and pass it to dir_name
	
	//Create backup dir and get its path
	if((backup_path=make_backup_dir(configurations, backup_dir_name))==NULL) { 
		free(configurations);
		exit(EXIT_FAILURE);
	}
	free(configurations);

	memset(command_list, '\0', sizeof(command_list)); //Make sure array is empty
	
	//RsyncOpt (required)
	if((configurations=read_config_unit(config_full_path, units_list[3])) == NULL) {
		if(errno==ENOENT) { //If file doesnt exists
			fprintf(stderr, "The configurations file doesn't exists.\n");
			fprintf(stderr, "Reminder: you can always generate new one using the -c option.\n");	
		}
		free(backup_path);
		exit(EXIT_FAILURE);
	}
	
	if(*configurations == '\0') {
		free(backup_path);
		free(configurations);
		fprintf(stderr, "Error: The mandatory section '%s' is empty!\n", units_list[3]);
		exit(EXIT_FAILURE);
	}

	clean_line(configurations);
	command_list[0] = "sudo";
	command_list[1] = "rsync";
	command_list[2] = configurations;
	command_list[4] = backup_path;

	//DirsToBackup (required)
	if((configurations=read_config_unit(config_full_path, units_list[4])) == NULL) {
		if(errno==ENOENT) { //If file doesnt exists
			fprintf(stderr, "The configurations file doesn't exists.\n");
			fprintf(stderr, "Reminder: you can always generate new one using the -c option.\n");	
		}
		free(command_list[2]);
		free(command_list[4]);
		exit(EXIT_FAILURE);
	}
	if(*configurations == '\0') {
		free(configurations);
		free(command_list[2]);
		free(command_list[4]);
		fprintf(stderr, "Error: The mandatory section '%s' is empty!\n", units_list[4]);
		exit(EXIT_FAILURE);
	}
	//The actual Rsync command
	len = 0;
	while((status=get_name(configurations+len, dir, sizeof(dir))) != 1) { //Get dir name 
		if(status) { //If failed to get name
			free(command_list[2]);
			free(command_list[4]);
			free(configurations);
			exit(EXIT_FAILURE);
		}
		command_list[3] = dir;

		status = exec_command(command_list[0], command_list);
	
		if(status) {
			free(command_list[2]);
			free(command_list[4]);
			free(configurations);
			exit(EXIT_FAILURE);
		}
		
		len += strlen(dir); //To ommit the already backed up dirs
		len += BLANK; //Other wise skip BLANK and check next dir
		
		if(*(configurations+len)==' ') //If there is a mistaken white char 
			break;

		memset(dir, '\0', sizeof(dir));
	}
	free(command_list[2]);
	free(command_list[4]);
	free(configurations);
	return 0;
}
