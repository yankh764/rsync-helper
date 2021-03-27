#ifndef SYS_BACKUP_H
#define SYS_BACKUP_H

typedef struct {
	int day;
	int month;
	int year;
} date;

int rm_dir(const char *);
void get_date(date *);
char *make_backup_dir(const char *, date);
int exec_command(const char *, char **);

#endif