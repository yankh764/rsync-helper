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
int rm_dir(const char *dir_path) {
	DIR *dr;
	struct dirent *dp;
	struct stat statbuf;
	char *new_path;
	size_t original_path_len, new_path_len;

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
        	if(closedir(dr)==-1)
        		fprintf(stderr, "An error occured while closing directory: %s\n", dir_path);
        	fprintf(stderr, "An error occured while allocating memory\n");
        	return -1;
        }
        snprintf(new_path, new_path_len+1, "%s/%s", dir_path, dp->d_name);
        //Get object's status
        if(stat(new_path, &statbuf)==0) {
        	if(S_ISDIR(statbuf.st_mode)) { //If object is a directory 
        		if(rm_dir(new_path)==-1) { //Delete it 
        			free(new_path);
        			if(closedir(dr)==-1)
        				fprintf(stderr, "An error occured while closing directory: %s\n", dir_path);
        			fprintf(stderr, "Error occured while removing directory: %s\n", new_path);
        			return -1;
        		}
        	}
        	else {
        		if(unlink(new_path)==-1) { //Delte file
        			free(new_path);
        			if(closedir(dr)==-1)
        				fprintf(stderr, "An error occured while closing directory: %s\n", dir_path);
        			fprintf(stderr, "Error occured while removing file: %s\n", new_path);
        			return -1;
        		}
        	}
        	free(new_path);
        }
        else {
        	free(new_path);
        	if(closedir(dr)==-1)
        		fprintf(stderr, "An error occured while closing directory: %s\n", dir_path);
        	fprintf(stderr, "An unknown error occured.\n");
        	return -1;
        }
    }
    if(closedir(dr)==-1) {
        fprintf(stderr, "An error occured while closing directory: %s\n", dir_path);
        return -1;
    }
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
char *make_backup_dir(const char *device_path, date date_struct) {
	char dir_name[9];
	char *backup_path; 
	int backup_path_size = sizeof(dir_name)+strlen(device_path)+1;

	//Convert the date_array to a string so will use it to name the dir in the device path
	snprintf(dir_name, sizeof(dir_name)+1, "%02d-%02d-%02d", date_struct.day, date_struct.month, date_struct.year);
	//Prepare the full backup path 
	backup_path = (char *) malloc(backup_path_size);
	if(backup_path == NULL) {
		fprintf(stderr, "Error occured while allocating memory.\n");
		return NULL;
	}
	snprintf(backup_path, backup_path_size+1, "%s%s/", device_path, dir_name); 

	if(mkdir(backup_path, S_IRWXU)==-1) { //If failed
		fprintf(stderr, "Error occured while creating directory: %s\n", backup_path);
		free(backup_path); 
		return NULL;
	}
	printf("Created: %s\n", backup_path);
	return backup_path;
}

/*
*This function will fork the parent process to create a
*child process then execute commands using one of the exec
*familie's functions. Return -1 for fork failure, 1 for execv() 
*failure, otherwise 0.
*/
int exec_command(const char *prog_name, char *commands[]) {
	char prog_path[strlen("/usr/bin/")+strlen(prog_name)];
	int status;
	pid_t pid;
	pid_t ret;

	snprintf(prog_path, sizeof(prog_path)+1, "/usr/bin/%s", prog_name);
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
