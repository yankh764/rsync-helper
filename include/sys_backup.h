#ifndef SYS_BACKUP_H
#define SYS_BACKUP_H

#define BLANK 1 //To represent a blank char 

typedef struct {
	int day;
	int month;
	int year;
} date;

int make_config_dir(const char *);
int rm_dir(const char *, unsigned int);
void get_date(date *);
char *make_backup_dir(char *, date);
int exec_command(const char *, char **);
int prepare_command(const char *, char **, unsigned int);
void help(const char *);

#endif
