#ifndef SYS_BACKUP_H
#define SYS_BACKUP_H

typedef struct {
	int day;
	int month;
	int year;
} date;

int rm_dir(char *);
void get_date(date *);
char *make_backup_dir(char *, date);
char *split_configs(char *, unsigned int *);
int exec_command(char *, char **);
char *get_name(const char *);
void clean_line(char *);

#endif