#ifndef CONFIG_MAN_H
#define CONFIG_MAN_H

int check_file_existence(char *);
void create_config_file(char *, const char*);
void write_config_unit(char *, const char *, const char *);
char *read_config_unit(char *, const char *);
int check_unit_existence(const char *, char *);
void read_reg_syntax(char *, char *, int);
int read_list_syntax(char *, char *, int);

#endif