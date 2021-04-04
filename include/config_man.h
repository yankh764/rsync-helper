#ifndef CONFIG_MAN_H
#define CONFIG_MAN_H

int check_file_existence(const char *);
int create_config_file(const char *, const char*);
int write_config_unit(const char *, const char *, const char *);
char *read_config_unit(const char *, const char *);
int read_reg_syntax(const char *, char *);
int read_list_syntax(const char *, char *);

#endif