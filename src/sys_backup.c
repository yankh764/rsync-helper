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
#include <dirent.h>
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
		"{\n"
		"\tCommands for cleaning your sysem, like:\n"
		"\tsudo pacman -Sc (for deleting uninstalled packages\n"
		"\tfrom the cache in arch based distros)\n"
		"}", 
		"Your storage device path hdd, usb or whatever you use",  
		"sudo rsync -aAXHv --exclude={\"/dev/*\",\"/proc/*\",\"/sys/*\""
		",\"/tmp/*\",\"/run/*\",\"/mnt/*\",\"/media/*\",\"/lost+found\"} /", 
		"\0"
	};
	char *config_path = ".config/sys_backup";
	char *home = getenv("HOME");
	char config_full_path[strlen(home)+strlen(config_path)+1];
	char *configurations, *backup_path, *path_ptr;
	int opt, status;
	unsigned int i;
	date dir_name;

	/*Prepare configurations file's full path*/
	sprintf(config_full_path, "%s/%s", home, config_path);

	while((opt = getopt(argc, argv, ":c")) != EOF) {
		if(opt == 'c' && argc<3) {
			if(create_config_file(config_full_path, config_desc)==-1) 
				exit(EXIT_FAILURE);
			for(i=0; *units_desc[i]!='\0'; i++)
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
		fprintf(stderr, "Reminder: you can always generate new one using the -c argument.\n");
		exit(EXIT_FAILURE);
	}
	/*Read the configured units*/
	if((configurations=read_config_unit(config_full_path, units_list[0])) == NULL) //Unit wasnt found
		exit(EXIT_FAILURE);
	/*Separate each dir path on its own to delete it*/
	i = 0;
	while((path_ptr=split_dirs_paths(configurations, &i)) != NULL) {
		if(*path_ptr=='/' && path_ptr[1]=='\0'){ //If path is stand alone root tree
			fprintf(stderr, "Error: you can't remove root tree.\n");
			fprintf(stderr, "Please make sure that there is no sign of stand alone '/' in your configs.\n");
			exit(EXIT_FAILURE);
		}
		if((status=rm_dir(path_ptr)) == -1)
			exit(EXIT_FAILURE);
		else if(status) //If cant open dir
			printf("Couldn't open directory: %s\n", path_ptr);
		else
			printf("Removing: %s\n", path_ptr);
		i++;
	}

	if((configurations=read_config_unit(config_full_path, units_list[1])) == NULL) //Unit wasnt found
		exit(EXIT_FAILURE);
	system(configurations);

	if((configurations=read_config_unit(config_full_path, units_list[2])) == NULL) //Unit wasnt found
		exit(EXIT_FAILURE);
	get_date(&dir_name); //Get the date of today and pass it to dir_name

	if((backup_path=make_backup_dir(configurations, dir_name))==NULL) //Create backup dir and get its path
		exit(EXIT_FAILURE);
	
	if((configurations=read_config_unit(config_full_path, units_list[3])) == NULL) //Unit wasnt found
		exit(EXIT_FAILURE);
	backup_sys(configurations, backup_path); //Backup system in the created dir
	return 0;
}

/*
*Recursive function to delete directories.
*Returns -1 for failure, 1 if cant open dir, otherwise 0.
*/
int rm_dir(char *dir_path) {
	DIR *dr;
	struct dirent *dp;
	struct stat statbuf;
	unsigned int original_path_len, new_path_len;
	char *new_path;

	original_path_len = strlen(dir_path);
	if((dr=opendir(dir_path)) == NULL) //If failed opening dir -Probably doesnt exists-. Skip it
		return 1;

	//Loop in dir's content
	while((dp=readdir(dr))!=NULL) {
		//Ignore '.' and '..' directories
		if (strcmp(dp->d_name, ".")==0 || strcmp(dp->d_name, "..")==0)
            continue;
        new_path_len = original_path_len + strlen(dp->d_name) + 2; //2 = '\0' and '/'
        new_path = (char *) malloc(new_path_len);
        if(new_path == NULL) {
        	closedir(dr);
        	return -1;
        }
        sprintf(new_path, "%s/%s", dir_path, dp->d_name);
        //Get object status
        if(stat(new_path, &statbuf)==0) {
        	if(S_ISDIR(statbuf.st_mode)) { //If object is a directory 
        		if(rm_dir(new_path)==-1) { //Delete it 
        			free(new_path);
        			closedir(dr);
        			fprintf(stderr, "Error occured while removing directory: %s\n", new_path);
        			return -1;
        		}
        	}
        	else {
        		if(unlink(new_path)==-1) { //Delte file
        			free(new_path);
        			closedir(dr);
        			fprintf(stderr, "Error occured while removing file: %s\n", new_path);
        			return -1;
        		}
        	}
        	free(new_path);
        }
        else {
        	free(new_path);
        	closedir(dr);
        	fprintf(stderr, "An unknown error occured.\n");
        	return -1;
        }
    }
    closedir(dr);
    rmdir(dir_path);
    return 0;
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
	char dir_name[9];
	static char *backup_path; 

	//Convert the date_array to a string so will use it to name the dir in the device path
	sprintf(dir_name, "%02d-%02d-%02d", date_struct.day, date_struct.month, date_struct.year);
	//Prepare the full backup path 
	backup_path = (char *) malloc(sizeof(dir_name)+strlen(device_path)+1);
	if(backup_path == NULL) {
		fprintf(stderr, "Error occured while allocating memory.\n");
		return NULL;
	}
	sprintf(backup_path, "%s/%s", device_path, dir_name); 

	if(mkdir(backup_path, S_IRUSR|S_IWUSR)==-1) { //If failed
		fprintf(stderr, "Error occured while creating directory: %s\n", backup_path);
		free(backup_path); 
		return NULL;
	}
	return backup_path;
}

/*
*This function will make the full system backup using rsync to the passed dir path.
*/
void backup_sys(const char *command, char *backup_path) {
	char full_command[strlen(command)+strlen(backup_path)+sizeof("/")];

	/*Prepare command*/
	sprintf(full_command, "%s %s/", command, backup_path);
	free(backup_path);

	system(full_command); //Execute the command 
}

/*
*This funcion is going to split all the paths that were
*read in the DirsToClean unit then it will return a pointer
*to it so the program could delete one dir at a time.
*If finished converting all the paths return NULL.
*/
char *split_dirs_paths(char *paths, unsigned int *i_in_paths) {
	static char path[500];
	unsigned int i;

	if(paths[*i_in_paths]=='\0') //If the beginning of DirsToClean is terminated
		return NULL;

	memset(path, '\0', sizeof(path)); //Make sure the static array is empty
	for(i=0; paths[*i_in_paths]!='\0' && paths[*i_in_paths]!=' '; i++) { 
		path[i] = paths[*i_in_paths];
		*i_in_paths = *i_in_paths + 1;
	}
	return path;
} 