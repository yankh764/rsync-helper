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
#include "config_man.h" //For config files managment 
#include "sys_backup.h"

int main(int argc, char *argv[]) {
	const char *config_desc = {
		"################################################################\n"
		"# This configuration file purposes are to provide to the       #\n"
		"# sys_backup.c program the right cleaning process and storage  #\n"
		"# device path, customized to your own needs and preferences.   #\n"
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
	char *config_path = ".config/sys_backup";
	char *home = getenv("HOME");
	char config_full_path[strlen(home)+strlen(config_path)+1];
	char *configurations, *backup_path, *ptr, *prog_name;
	char *command_arg, *commands_list[7];
	int opt, status;
	unsigned int i, len;
	date dir_name;

	/*Prepare configurations file's full path*/
	sprintf(config_full_path, "%s/%s", home, config_path);

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
	if(check_file_existence(config_full_path) == 0) { //Check if config file exists
		fprintf(stderr, "Reminder: you can always generate new one using the -c option.\n");
		exit(EXIT_FAILURE);
	}
	/*Read the configured units*/
	//DirsToClean
	if((configurations=read_config_unit(config_full_path, units_list[0])) == NULL) //Unit wasnt found
		exit(EXIT_FAILURE);
	/*Separate each dir path on its own to delete it*/
	i = 0;
	while((ptr=split_configs(configurations, &i)) != NULL) {
		if(*ptr=='/' && ptr[1]=='\0'){ //If path is stand alone root tree
			fprintf(stderr, "Error: you can't remove root tree.\n");
			fprintf(stderr, "Please make sure that there is no sign of stand alone '/' in your configs.\n");
			free(configurations);
			exit(EXIT_FAILURE);
		}
		clean_line(ptr);
		if((status=rm_dir(ptr)) == -1) {
			free(configurations);
			exit(EXIT_FAILURE);
		}
		else if(status) //If cant open dir skip it 
			;
		else
			printf("Removing: %s\n", ptr);
	}
	free(configurations);

	//CleaningCommands
	if((configurations=read_config_unit(config_full_path, units_list[1])) == NULL) //Unit wasnt found
		exit(EXIT_FAILURE);
	/*Separate each cleaning command on its own to execute it*/
	i = 0;
	memset(commands_list, '\0', sizeof(commands_list)); //Make sure array is empty
	while((ptr=split_configs(configurations, &i)) != NULL) {
		if((prog_name=get_name(ptr))==NULL) { //If no program name was found
			fprintf(stderr, "An unknown error occured\n");
			free(configurations);
			exit(EXIT_FAILURE);
		}
		/*Prepare command*/
		if(strcmp(prog_name, "sudo") == 0) { //If command starts with sudo
			char new_prog_name[200+strlen("/usr/bin/")];
			
			commands_list[0] = "sudo";
			prog_name = get_name(ptr+strlen("sudo ")); //Get the name after sudo
			sprintf(new_prog_name, "/usr/bin/%s", prog_name); 
			commands_list[1] = new_prog_name; //Insert it with the full path
			command_arg = ptr+strlen("sudo ")+strlen(prog_name) + 1; //skip all the read characters
			clean_line(command_arg);
			commands_list[2] = command_arg; 
			printf("Warning: Executing the command '%s' with root privileges!\n", ptr);
			status = exec_command(commands_list[0], commands_list);
			if(status==-1) {
				free(configurations);
				exit(EXIT_FAILURE);
			}
			if(status)
				_Exit(127);
		} 
		else {
			command_arg = ptr+strlen(prog_name)+1; //+1 to ommit the space
			clean_line(command_arg);
			commands_list[0] = prog_name;
			commands_list[1] = command_arg;
			status = exec_command(commands_list[0], commands_list);
			if(status==-1) {
				free(configurations);
				exit(EXIT_FAILURE);
			}
			if(status)
				_Exit(127);
		}
	}
	free(configurations);

	//DevicePath
	if((configurations=read_config_unit(config_full_path, units_list[2])) == NULL) //Unit wasnt found
		exit(EXIT_FAILURE);
	get_date(&dir_name); //Get the date of today and pass it to dir_name
	if((backup_path=make_backup_dir(configurations, dir_name))==NULL) { //Create backup dir and get its path
		free(configurations);
		exit(EXIT_FAILURE);
	}
	free(configurations);

	/*Prepare Rsync commands*/
	memset(commands_list, '\0', sizeof(commands_list)); //Make sure array is empty
	//RsyncOpt
	if((configurations=read_config_unit(config_full_path, units_list[3])) == NULL) {
		free(backup_path);
		exit(EXIT_FAILURE);
	}
	clean_line(configurations);

	commands_list[0] = "sudo";
	commands_list[1] = "/usr/bin/rsync";
	commands_list[2] = configurations;
	commands_list[4] = backup_path;

	//DirsToBackup
	if((configurations=read_config_unit(config_full_path, units_list[4])) == NULL) {
		free(commands_list[2]);
		free(commands_list[4]);
		exit(EXIT_FAILURE);
	}

	len = 0;
	while(1) {
		ptr = get_name(configurations+len);
		if(ptr != NULL) {
			commands_list[3] = ptr;

			status = exec_command(commands_list[0], commands_list);
			if(status==-1) {
				free(commands_list[2]);
				free(configurations);
				free(commands_list[4]);
				exit(EXIT_FAILURE);
			}
			if(status)
				_Exit(127);
			len += strlen(ptr) + 1; //to ommit the already checked names 
		}
		else {
			free(commands_list[2]);
			free(configurations);
			free(commands_list[4]);
			break;
		}
	}
	return 0;
}