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
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
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
		"RsyncCommand", "\0"
	};
	const char *units_desc[] = {
		"Dirs path you regularly clean, like some cache dirs",
		"#Commands for cleaning your sysem, like:\n"
		"#sudo pacman -Sc (for deleting uninstalled packages\n"
		"#from the cache in arch based distros)", 
		"Your storage device path hdd, usb or whatever you use",  
		"sudo rsync -aAXHv --exclude={\"/dev/*\",\"/proc/*\",\"/sys/*\""
		",\"/tmp/*\",\"/run/*\",\"/mnt/*\",\"/media/*\",\"/lost+found\"} / ", 
		"\0"
	};
	char *config_path = "/.config/sys_backup";
	char config_full_path[60];
	char *home, *configurations, *backup_path;
	date dir_name;
	int opt;
	unsigned int i;

	/*Get configuration's file path*/
	home = getenv("HOME"); //Get home path 
	//Check for overflow 
	if(strlen(home)+strlen(config_path) > sizeof(config_full_path)) {
		fprintf(stderr, "The size of your config path is invalid!\n");
		exit(EXIT_FAILURE);
	}
	strcpy(config_full_path, home);
	strcat(config_full_path, config_path);

	while((opt = getopt(argc, argv, ":c")) != EOF) {
		if(opt == 'c' && argc<3) {
			create_config_file(config_full_path, config_desc);
			for(i=0; *units_desc[i]!='\0'; i++)
				write_config_unit(config_full_path, units_list[i], units_desc[i]);
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
		fprintf(stderr, "Error occured due to missing configurations file.\n");
		fprintf(stderr, "Reminder: you can always generate new one using the -c argument.\n");
		exit(EXIT_FAILURE);
	}
	/*Read the configured units*/
	if((configurations=read_config_unit(config_full_path, units_list[0])) == NULL) { //Unit wasnt found
		fprintf(stderr, "Error occured while searching for unit: %s.\n", units_list[0]);
		exit(EXIT_FAILURE);
	}
	delete_dirs(configurations);

	if((configurations=read_config_unit(config_full_path, units_list[1])) == NULL) { //Unit wasnt found
		fprintf(stderr, "Error occured while searching for unit: %s.\n", units_list[1]);
		exit(EXIT_FAILURE);
	}
	system(configurations);

	if((configurations=read_config_unit(config_full_path, units_list[2])) == NULL) { //Unit wasnt found
		fprintf(stderr, "Error occured while searching for unit: %s.\n", units_list[2]);
		exit(EXIT_FAILURE);
	}
	get_date(&dir_name); //Get the date of today and pass it to the dir_name
	backup_path = make_backup_dir(configurations, dir_name); //Create backup dir and get its path

	if((configurations=read_config_unit(config_full_path, units_list[3])) == NULL) { //Unit wasnt found
		fprintf(stderr, "Error occured while searching for unit: %s.\n", units_list[3]);
		exit(EXIT_FAILURE);
	}
	backup_sys(configurations, backup_path); //Backup system in the created dir
	return 0;
}

/*
*This function will delete the given argument dirs.
*/
void delete_dirs(char *dirs_path) {
	char full_command[700];
	const char *command = "rm -rf ";
	const char *danger_message =
		"Nice try, but we wont let you destroy your system ;)\n"
		"Please make sure that DirsToClean unit is configured properly "
		"and there is no sign of standalone root tree (\"/\").";
	unsigned int i;

	//Loop in dirs_path to make sure no one removing the root tree by mistake
	for(i=0; i<strlen(dirs_path); i++)
		if(dirs_path[i]==' ' && dirs_path[i+1]=='/' && dirs_path[i+2]==' ') {
			fprintf(stderr, "%s\n", danger_message);
			exit(EXIT_FAILURE);
		}
	//Check for overflow
	if(strlen(dirs_path)+strlen(command) > sizeof(full_command)) {
		fprintf(stderr, "The size of the inserted string in sys_backup.c -> delete_dirs is invalid.\n");
		exit(EXIT_FAILURE);
	}
	//Copy command to buffer (rm -rf dirs_path) to delete dirs.
	strcpy(full_command, command);
	strcat(full_command, dirs_path);

	system(full_command); //Execute command
}

/*
*This function will get the date of today, and it'll insert it to the passed date array.
*/
void get_date(date *date_struct) {
	long int sec_since_epoch;
	struct tm current_time, *time_ptr;

	sec_since_epoch = time(0); 
	time_ptr = &current_time; //Set time pointer to the current_time struct
	localtime_r(&sec_since_epoch, time_ptr);

	//Pass today's date to the date struct  
	date_struct->day = time_ptr->tm_mday;
	date_struct->month = time_ptr->tm_mon + 1; //+1 because months range from 0 - 11
	date_struct->year =  time_ptr->tm_year - 100; //-100 because tm_year is number of passed years since 1900
}

/*
*A function that gets pointer to int array that contains the
*date of today and create a backup dir in the passed path 
*passed date. Then it will return the full path of the created dir. 
*/
char *make_backup_dir(char *device_path, date date_struct) {
	char dir_name[10];
	static char backup_path[150]; //The returned full backup path 

	//Convert the date_array to a string so will use it to name the dir in the device path
	sprintf(dir_name, "%02d-%02d-%02d", date_struct.day, date_struct.month, date_struct.year);
	//Check for overflow
	if(strlen(device_path)+strlen(dir_name) > sizeof(backup_path)) {
		fprintf(stderr, "The size of the device path is invalid.\n");
		exit(EXIT_FAILURE);
	}
	strcpy(backup_path, device_path);
	strcat(backup_path, dir_name); //Complete the full dir path 

	if(mkdir(backup_path, S_IRUSR|S_IWUSR)==-1) { //If failed
		fprintf(stderr, "Error occured while creating directory: %s\n", backup_path);
		exit(EXIT_FAILURE);
	}
	return backup_path;
}

/*
*This function will make the full system backup using rsync to the passed dir path.
*/
void backup_sys(const char *command, char *backup_path) {
	char full_command[500];

	/*Prepare command*/
	//Check for overflow
	if(strlen(command)+strlen(backup_path)+sizeof("/") > sizeof(full_command)) {
		fprintf(stderr, "The size of the inserted string in sys_backup.c -> backup_sys is invalid.\n");
		exit(EXIT_FAILURE);
	}
	strcpy(full_command, command);
	strcat(full_command, backup_path);
	strcat(full_command, "/");

	system(full_command); //Execute the command 
}