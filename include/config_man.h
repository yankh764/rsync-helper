#ifndef CONFIG_MAN_H
#define CONFIG_MAN_H

#define UNITS 701 //Max size of line in a unit 
#define CONFIGS 1001 //Maximum size of configurations
#define NAME_LEN 200

int create_config_file(const char *, const char*);
int write_config_unit(const char *, const char *, const char *, const char *);
char *read_config_unit(const char *, const char *);
int read_reg_syntax(const char *, char *);
int read_list_syntax(const char *, char *);

#endif
