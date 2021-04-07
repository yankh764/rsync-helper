# Rsync Helper and a Time Saver
For every linux geek who can't stop tinkering the system, can't  
stop cleaning it and do all the house keeping to make it the best,  
this repo is for you.  
I just created rsync and few commad line tools helper and a wrraper program that can  
do all the full system backup and cleaning process automatically and could save  
you a lot of time.  
This program will read configurations from ~/.config/sys_backup, so just make  
sure to configure the file correctly and properly.

## Dependencies
1. rsync (Backup program).  
2. gcc (C compiler).   
3. git (to clone this repo).
     
You can install all these packages using your distribution's package manager.  

## Usage
1. Make sure all the dependencies are installed, if not install them.  
2. Clone this repo by typing:  

	```git clone https://github.com/yankh764/full-system-backup.git```

3. Move to the cloned directory and type:

	```make```   
	```sudo make install```   
	```make clean``` (optional)

4. Run the program using the -c option by typing in the terminal:    
	```backup -c```  
It'll create a configuration file in the ~/.config with the name sys_backup.
Configure it to your needs and prefrences -all instructions are written there-.  
Then make sure that the storage device is mounted, run the program one more time   
by typing:    
	```backup```    
It should execute and do all the cleaning process, create a backup directory  
in storage device then it'll backup all your system there.  

## Limitations
This program has few limitations that are good to note:
1. In the ```CleaningCommands``` section each command has a limited number of arguments;   
the program name and 7 arguments (each command consists of maximum 8 words).
2. Each line has 700 bytes (700 characters) limit and for a full configured section   
there is limit for 1000 characters.
3. Each program name in the ```CleaningCommands``` can consists of maximum 200 characters   
and each one of its argument has a buffer for maximum 100 bytes.   
4. Since the program is most likely to be executed using a non-root user the ```DirsToClean```   
section has a limitation of deleting only the directories that the user is permited to delete.

## Latest Updates
1. Simplified the syntax of the configuration file a little bit.
2. Added a make file to simplify the compilation process.
3. Added header guards.
4. Improved program's security.
5. Added the option to configure rsync command so you can backup whatever you wish.
6. Edited the config_example so it will be up to date and relevant.
7. Increased the number of arguments each program can take.
