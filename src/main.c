/*
*This program will make some cleaning that you regularly do 
*before the full system backup. And then itll create new dir 
*in your storage device with date, to make the system backup in
*it useng rsync. The program will use system() to connect all the
*command line tools together and automate this process. Lastly 
*its good to note that tho program reads all the commands and 
*your customaized cleaning process from a config file with the 
*following path: ~/.config/sys_backup
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "config_man.h" //For config files managment 
#include "sys_backup.h"

#define BLANK 1 //To represent size of blank char

int main(int argc, char *argv[]) {
	const char *config_desc = {
		"################################################################\n"
		"# This configuration file purposes are to provide the backup   #\n"
		"# program the right cleaning process and storage device path,  #\n"
		"# customized to your own needs and preferences.                #\n"
		"################################################################"
	};
	const char *units_list[] = {
		"DirsToClean", "CleaningCommands", "DevicePath", 
		"RsyncOpt", "DirsToBackup", 
		"\0"
	};
	const char *units_desc[] = {
		"Dirs path you regularly clean, like some cache dirs",
		"{\n"
		"\tCommands for cleaning your sysem, like:\n"
		"\tsudo pacman -Sc (for deleting uninstalled packages\n"
		"\tfrom the cache in arch based distros)\n"
		"}", 
		"Yous storage device's path",
		"Rsync backup option, for example: -aAXHv",
		"Directories to backup", 
		"\0"
	};
	const char *config_path = ".config/sys_backup";
	const char *home = getenv("HOME");
	char *sudo = "sudo";
	char config_full_path[strlen(home)+strlen(config_path)+1];
	char *configurations, *backup_path, *ptr;
	char *command_arg, *commands_list[7], dir_to_backup[200];
	char prog_name[200], new_prog_name[sizeof(prog_name)+strlen("/usr/bin/")];
	int opt, status;
	unsigned int i, len;
	date backup_dir_name;

	/*Prepare configurations file's full path*/
	snprintf(config_full_path, sizeof(config_full_path)+1, "%s/%s", home, config_path);

	while((opt = getopt(argc, argv, ":c")) != EOF) {
		if(opt == 'c' && argc<3) {
			if(create_config_file(config_full_path, config_desc)==-1) 
				exit(EXIT_FAILURE);
			for(i=0; *units_list[i]!='\0'; i++)
				if(write_config_unit(config_full_path, units_list[i], units_desc[i])==-1) 
					exit(EXIT_FAILURE);
			printf("%s was succesfully generated.\n", config_full_path);
			return 0;
		}
		else if(argc > 3) {
			fprintf(stderr, "Too many arguments were given.\n");
			exit(EXIT_FAILURE);
		}
		else {
			fprintf(stderr, "The argument %s is invalid.\n", argv[1]);
			exit(EXIT_FAILURE);
		}
	}

	/*Read the configured units*/
	//DirsToClean
	if((configurations=read_config_unit(config_full_path, units_list[0])) == NULL) {
		if(errno==ENOENT) { //If file doesnt exists
			fprintf(stderr, "The configurations file doesn't exists.\n");
			fprintf(stderr, "Reminder: you can always generate new one using the -c option.\n");	
		}
		exit(EXIT_FAILURE);
	}

	//Separate each dir path on its own to delete it
	while(1) {
		ptr = split_configs(configurations);
		if(ptr==NULL) {
			if(errno==ENOMEM) { //If error is memory error (calloc error)
				free(configurations);
				exit(EXIT_FAILURE);
			}
			free(ptr);
			break;
		}
		if(*ptr=='/' && ptr[1]=='\0'){ //If path is stand alone root tree
			fprintf(stderr, "Error: you can't remove root tree.\n");
			fprintf(stderr, "Please make sure that there is no sign of stand alone '/' in your configs.\n");
			free(configurations);
			free(ptr);
			exit(EXIT_FAILURE);
		}

		clean_line(ptr); //Clean the end of the line from white space chars 
		if((status=rm_dir(ptr)) == -1) {
			free(configurations);
			free(ptr);
			exit(EXIT_FAILURE);
		}
		else if(status) //If cant open dir skip it 
			;
		else
			printf("Removing: %s\n", ptr);
		free(ptr);
	}
	free(configurations);

	//CleaningCommands
	if((configurations=read_config_unit(config_full_path, units_list[1])) == NULL) {
		if(errno==ENOENT) { //If file doesnt exists
			fprintf(stderr, "The configurations file doesn't exists.\n");
			fprintf(stderr, "Reminder: you can always generate new one using the -c option.\n");	
		}
		exit(EXIT_FAILURE);
	}
	
	/*Separate each cleaning command on its own to execute it*/
	memset(commands_list, '\0', sizeof(commands_list)); //Make sure array is empty
	while(1) {
		ptr = split_configs(configurations);
		if(ptr==NULL) {
			if(errno==ENOMEM) { //If error is memory error (malloc error)
				free(configurations);
				exit(EXIT_FAILURE);
			}
			free(ptr);
			break;
		}
		if(get_name(ptr, prog_name, sizeof(prog_name))==-1) { //If no program name was found
			free(configurations);
			free(ptr);
			exit(EXIT_FAILURE);
		}

		/*Prepare the actual command*/
		if(strcmp(prog_name, sudo) == 0) { //If command starts with sudo
			if(ptr[strlen(sudo)+BLANK]=='\0') { //If sudo is the whole command 
				fprintf(stderr, "Please append a command for the sudo in your configurations file.\n");
				free(configurations);
				free(ptr);
				exit(EXIT_FAILURE);
			}
			get_name((ptr+strlen(sudo)+BLANK), prog_name, sizeof(prog_name)); //Get the name after sudo
			snprintf(new_prog_name, sizeof(new_prog_name)+1, "/usr/bin/%s", prog_name); 
			command_arg = (ptr+strlen(sudo)+BLANK) + (strlen(prog_name)+BLANK); //skip all the read characters
			clean_line(command_arg);
			commands_list[0] = sudo;
			commands_list[1] = new_prog_name; //Insert it with the full path to avoid vulnerabilities
			commands_list[2] = command_arg; 
			printf("Warning: Executing the command '%s' with root privileges!\n", ptr);
			status = exec_command(commands_list[0], commands_list);
			if(status==-1) {
				free(configurations);
				free(ptr);
				exit(EXIT_FAILURE);
			}
			if(status)
				_Exit(127);
		} 
		else {
			if(ptr[strlen(prog_name)+BLANK]=='\0') { //If program has no arguments
				commands_list[0] = prog_name;
				status = exec_command(commands_list[0], commands_list);
				
				if(status==-1) {
					free(configurations);
					free(ptr);
					exit(EXIT_FAILURE);
				}
				if(status)
					_Exit(127);
			}

			command_arg = ptr+strlen(prog_name)+BLANK; 
			clean_line(command_arg);
			commands_list[0] = prog_name;
			commands_list[1] = command_arg;
			status = exec_command(commands_list[0], commands_list);
			if(status==-1) {
				free(configurations);
				free(ptr);
				exit(EXIT_FAILURE);
			}
			if(status)
				_Exit(127);
		}
		free(ptr);
	}
	free(configurations);

	//DevicePath
	if((configurations=read_config_unit(config_full_path, units_list[2])) == NULL) {
		if(errno==ENOENT) { //If file doesnt exists
			fprintf(stderr, "The configurations file doesn't exists.\n");
			fprintf(stderr, "Reminder: you can always generate new one using the -c option.\n");	
		}
		exit(EXIT_FAILURE);
	}
	
	get_date(&backup_dir_name); //Get the date of today and pass it to dir_name
	//Create backup dir and get its path
	if((backup_path=make_backup_dir(configurations, backup_dir_name))==NULL) { 
		free(configurations);
		exit(EXIT_FAILURE);
	}
	free(configurations);

	/*Prepare Rsync commands*/
	memset(commands_list, '\0', sizeof(commands_list)); //Make sure array is empty
	//RsyncOpt
	if((configurations=read_config_unit(config_full_path, units_list[3])) == NULL) {
		if(errno==ENOENT) { //If file doesnt exists
			fprintf(stderr, "The configurations file doesn't exists.\n");
			fprintf(stderr, "Reminder: you can always generate new one using the -c option.\n");	
		}
		free(backup_path);
		exit(EXIT_FAILURE);
	}
	
	clean_line(configurations);
	commands_list[0] = sudo;
	commands_list[1] = "/usr/bin/rsync";
	commands_list[2] = configurations;
	commands_list[4] = backup_path;

	//DirsToBackup
	if((configurations=read_config_unit(config_full_path, units_list[4])) == NULL) {
		if(errno==ENOENT) { //If file doesnt exists
			fprintf(stderr, "The configurations file doesn't exists.\n");
			fprintf(stderr, "Reminder: you can always generate new one using the -c option.\n");	
		}
		free(commands_list[2]);
		free(commands_list[4]);
		exit(EXIT_FAILURE);
	}

	//The actual Rsync command
	len = 0;
	while(get_name(configurations+len, dir_to_backup, sizeof(dir_to_backup))!=-1) { //Get dir name 
		commands_list[3] = dir_to_backup;

		status = exec_command(commands_list[0], commands_list);
		if(status==-1) {
			free(commands_list[2]);
			free(commands_list[4]);
			free(configurations);
			exit(EXIT_FAILURE);
		}
		if(status)
			_Exit(127);
		len += strlen(dir_to_backup) + BLANK; //To ommit the already backed up dirs
	}
	free(commands_list[2]);
	free(commands_list[4]);
	free(configurations);
	return 0;
}