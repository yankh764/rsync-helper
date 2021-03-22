#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include "sys_backup.h"
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
	if((dr=opendir(dir_path)) == NULL) { //If failed opening dir
		if(errno==ENOENT) { //If dir doesnt exists
			fprintf(stderr, "Directory doesn't exists: %s\n", dir_path);
			return 1;
		}
		else {
			fprintf(stderr, "Error occured while opening directory: %s\n", dir_path);
			return -1;
		}
	}

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
	char *backup_path; 

	//Convert the date_array to a string so will use it to name the dir in the device path
	sprintf(dir_name, "%02d-%02d-%02d", date_struct.day, date_struct.month, date_struct.year);
	//Prepare the full backup path 
	backup_path = (char *) malloc(sizeof(dir_name)+strlen(device_path)+1);
	if(backup_path == NULL) {
		fprintf(stderr, "Error occured while allocating memory.\n");
		return NULL;
	}
	sprintf(backup_path, "%s%s/", device_path, dir_name); 

	if(mkdir(backup_path, S_IRWXU)==-1) { //If failed
		fprintf(stderr, "Error occured while creating directory: %s\n", backup_path);
		free(backup_path); 
		return NULL;
	}
	printf("Created: %s\n", backup_path);
	return backup_path;
}

/*
*This function is going to split a read list of configurations
*into a separated lines, then return a pointer to it. NULL for 
*failure.
*/
char *split_configs(char *configs, unsigned int *i_in_configs) {
	static char line[600];
	unsigned int i;

	if(configs[*i_in_configs]=='\0') //If the beginning of configs is terminated
		return NULL;

	memset(line, '\0', sizeof(line)); //Make sure the static array is empty
	for(i=0; configs[*i_in_configs]!='\0'; i++) { 
		if(configs[*i_in_configs]=='\n') {
			*i_in_configs = *i_in_configs + 1; //Skip it and break
			break;
		}
		line[i] = configs[*i_in_configs];
		*i_in_configs = *i_in_configs + 1;
	}
	return line;
} 

/*
*This function will fork the parent process to create a
*child process then execute commands using one of the exec
*familie's functions. Return -1 for fork failure, 1 for execv() 
*failure, otherwise 0.
*/
int exec_command(char *prog_name, char *commands[]) {
	char prog_path[strlen("/usr/bin/")+strlen(prog_name)];
	int status;
	pid_t pid;
	pid_t ret;

	sprintf(prog_path, "/usr/bin/%s", prog_name);
	pid = fork(); //Create a new child process
	if(pid == -1) {//If failed to create new child
		fprintf(stderr, "An error occured while creating a child process.\n");
		return -1;
	}
	else if(pid != 0) { //If child process didnt start
		while((ret = waitpid(pid, &status, 0)) == -1) { //Wait for child
			if(errno != EINTR) { //If the waitpid() error isnt an interrupte signal
				fprintf(stderr, "An error occured while waiting for the child process.\n");
				return -1;
			}
		}
	}
	else 
		if(execv(prog_path, commands)==-1) //If command wasnt found
			return 1;
	return 0; 
}

/*
*This function gets a pointer to a line and then it 
*gets the first word in yhr line and return a pointer to it.
*/
char *get_name(const char *line) {
	static char name[200];
	unsigned int i; 

	for(i=0; i<sizeof(name); i++) {
		if(line[i]==' ' || line[i]=='\n' || line[i]=='\t') {
			name[i] = '\0';
			break;
		}
		name[i] = line[i];
	}
	if(*name=='\0') //If name is empty
		return NULL;
	return name;
} 

/*
*This function is going to check if the line ends with any space, 
*and modify it if it is since the exec_command and make_backup_dir()
*functions are space sensitive.
*/
void clean_line(char *line) {
	unsigned int i = 1;
	int len = strlen(line);

	while(line[len-i] == ' ') { //Clean line from spaces at the end
		line[len-i] = '\0';
		i++;
	}
}