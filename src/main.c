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
#include "str_man.h" //for string and line manipulation 
#include "sys_backup.h"

#define MAX_ARGS 8 //Maximum number of arguments commands_list can take  
#define MAX_LEN 700 //Maximum line lenght 
#define BLANK  1 //To represent size of blank char

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
        "{\n"
        "\tDirs path you regularly clean, like some cache dirs\n"
        "\tPlease be aware in case the path is something like this:\n"
        "\t/path/to/dir\n"
        "\tIt will delete the dir and its content, so if you want\n"
        "\tto keep the dir just append a slash to the path.\n"
        "}",
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
	char config_full_path[strlen(home)+strlen(config_path)+1];
	char *configurations, *backup_path, *ptr;
	char *command_list[MAX_ARGS], dir_to_backup[200];
	int opt, status;
	unsigned int i, len, dir_removal_status;
	date backup_dir_name;

	/*Prepare configurations file's full path*/
	snprintf(config_full_path, sizeof(config_full_path)+1, "%s/%s", home, config_path);

	while((opt = getopt(argc, argv, ":c")) != EOF) {
		if(opt == 'c' && argc<3) {
			if(create_config_file(config_full_path, config_desc)) 
				exit(EXIT_FAILURE);
			for(i=0; units_list[i]!=NULL; i++)
				if(write_config_unit(config_full_path, units_list[i], units_desc[i])) 
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

	//DirsToClean
	if((configurations=read_config_unit(config_full_path, units_list[0])) == NULL) {
		if(errno==ENOENT) { //If file doesnt exists
			fprintf(stderr, "The configurations file doesn't exists.\n");
			fprintf(stderr, "Reminder: you can always generate new one using the -c option.\n");	
		}
		exit(EXIT_FAILURE);
	}

	//Separate each dir path on its own to delete it
	while((ptr=split_paragraph(configurations, MAX_LEN))!=NULL) {
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
	memset(command_list, '\0', sizeof(command_list)); //Make sure array is empty
    while((ptr=split_paragraph(configurations, MAX_LEN))!=NULL) {
		if(prepare_command(ptr, command_list, MAX_ARGS)) {
			free(ptr);
			free(configurations);
			exit(EXIT_FAILURE);
		}
		if(strcmp(command_list[0], "sudo")==0)
			printf("\nWarning: Executing the command '%s' with root privileges!\n", ptr);
		
		status = exec_command(command_list[0], command_list);
		if(status==-1) {
			free(configurations);
			free(ptr);
			for(i=0; command_list[i]!=NULL; i++)
				free(command_list[i]);
			exit(EXIT_FAILURE);
		}
		else if(status)
			_Exit(127);
			
		free(ptr);
		for(i=0; command_list[i]!=NULL; i++)
			free(command_list[i]);
	}
	if(errno==ENOMEM) { //If error is memory error (malloc error)
		free(configurations);
		exit(EXIT_FAILURE);
	}
	free(ptr);
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
	clean_line(configurations);
	//Create backup dir and get its path
	if((backup_path=make_backup_dir(configurations, backup_dir_name))==NULL) { 
		free(configurations);
		exit(EXIT_FAILURE);
	}
	free(configurations);

	/*Prepare Rsync commands*/
	memset(command_list, '\0', sizeof(command_list)); //Make sure array is empty
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
	command_list[0] = "sudo";
	command_list[1] = "rsync";
	command_list[2] = configurations;
	command_list[4] = backup_path;

	//DirsToBackup
	if((configurations=read_config_unit(config_full_path, units_list[4])) == NULL) {
		if(errno==ENOENT) { //If file doesnt exists
			fprintf(stderr, "The configurations file doesn't exists.\n");
			fprintf(stderr, "Reminder: you can always generate new one using the -c option.\n");	
		}
		free(command_list[2]);
		free(command_list[4]);
		exit(EXIT_FAILURE);
	}

	//The actual Rsync command
	len = 0;
	while((status=get_name(configurations+len, dir_to_backup, sizeof(dir_to_backup)))!=1) { //Get dir name 
		if(status) { //If failed to get name
			free(command_list[2]);
			free(command_list[4]);
			free(configurations);
			exit(EXIT_FAILURE);
		}
		command_list[3] = dir_to_backup;

		status = exec_command(command_list[0], command_list);
		if(status==-1) {
			free(command_list[2]);
			free(command_list[4]);
			free(configurations);
			exit(EXIT_FAILURE);
		}
		if(status)
			_Exit(127);
			
		len += strlen(dir_to_backup); //To ommit the already backed up dirs
		if(configurations[len] == '\0') //If reached end of configurations
			break;
		len += BLANK; //Other wise skip BLANK and cheak next dir_to_backup
		
		memset(dir_to_backup, '\0', sizeof(dir_to_backup));
	}
	free(command_list[2]);
	free(command_list[4]);
	free(configurations);
	return 0;
}
