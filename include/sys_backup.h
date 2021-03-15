#ifndef SYS_BACKUP_H
#define SYS_BACKUP_H

void delete_dirs(char *);
void get_date(int *);
char *make_backup_dir(char *, int *);
void backup_sys(const char *, char *);

#endif