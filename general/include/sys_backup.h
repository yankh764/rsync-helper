#ifndef SYS_BACKUP_H
#define SYS_BACKUP_H

typedef struct {
	int day;
	int month;
	int year;
} date;

void delete_dirs(char *);
void get_date(date *);
char *make_backup_dir(char *, date);
void backup_sys(const char *, char *);

#endif