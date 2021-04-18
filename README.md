# Rsync Helper and a Time Saver
For every linux geek who can't stop tinkering the system, can't  
stop cleaning it and do all the house keeping to make it the best,  
this repo is for you.  
I just created rsync and few commad line tools helper and a wrraper program that can  
do all the full system backup and cleaning process automatically and could save  
you a lot of time.  
This program will read configurations from ```~/.config/sys_backup```, so just make  
sure to configure the file correctly and properly.

## Dependencies
1. rsync (Backup program).  
2. gcc (C compiler).   
3. git (to clone this repo).
     
You can install all these packages using your distribution's package manager.  

## Usage
1. Make sure all the dependencies are installed, if not install them.  
2. Clone this repo by typing:  

	```git clone https://github.com/yankh764/rsync-helper.git```

3. Move to the cloned directory and type:

	```make```   
	```make install``` (with root privileges)   
	```make clean``` (optional)   
Now the program would be installed to ```/usr/local/bin```

4. Run the program using the -c option (to generate config file) by typing in the terminal:    
	```backup -c```  
It'll create a configuration file in the ```~/.config``` with the name ```sys_backup```.
Configure it to your needs and prefrences -all instructions are written there-.  
Then make sure that the storage device is mounted, run the program one more time   
by typing:    
	```backup```    
It should execute and do all the cleaning process, create a backup directory  
in storage device then it'll backup your system there.  

## Tips and Tricks
There are some tips and tricks to the program I want to share that completed    
the program for me and made it to overcome few of the limitations:
1. Since ```DirsToClean``` can't delete directories that belongs to the root    
it's good to note and remind you that you can always create a bash script in    
```/usr/local/bin``` that deletes those dirs, make it executable then adding it    
to the ```Commands``` section with ```sudo``` prepended. For example:
#### clean_dirs  
	#!/bin/bash    
	    
	rm -rf /var/tmp/portage/*    
	rm -rf /var/cache/edb/*

Now make it executable by typing: 
	```sudo chmod +x /usrlocal/bin/clean_dirs```

## Limitations
This program has few limitations that are good to note:
1. In the ```Commands``` section each command has a limited number of arguments:   
the program name and 7 arguments (each command consists of maximum 8 words).
2. Each line has 700 bytes (700 characters) limit and for a full configured section   
there is limit for 1000 characters.    
3. Each program name in the ```Commands``` can consists of maximum 200 characters   
and each one of its argument has a buffer for maximum 150 bytes.   
4. Since the program is going to be executed using a non-root user the ```DirsToClean```   
section has a limitation of deleting only the directories that the user is permited to delete.    
5. The program can't be executed with root privileges for security concerns. But you can always    
execute any command with root privileges using this program by adding it to the ```Commands```    
section and prepending ```sudo``` to it.     
6. The ```Commands``` section has another limitation. You can't use in it any connector operator   
such as ```&```, ```|``` and all redirection operators.

## Latest Updates
1. Simplified the syntax of the configuration file a little bit.
2. Added a make file to simplify the compilation process.
3. Added header guards.
4. Improved program's security.
5. Added the option to configure rsync command so you can backup whatever you wish.
6. Edited the config_example so it will be up to date and relevant.
7. Increased the number of arguments each program can take.
