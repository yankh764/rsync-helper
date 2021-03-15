# Full System Backup
For every linux geek who can't stop tinkering the system, can't  
stop cleaning it and do all the house keeping to make it the best,  
this repo is for you.  
I just created rsync and few commad line tools wrraper program that can  
do all the full system backup and cleaning process automatically and save  
a lot of time.  
This program will read configurations from ~/.config/sys_backup, so just make  
sure to configure the file correctly.

## Dependencies
1. rsync (Backup program).  
2. gcc (C compiler).   
3. git (to clone this repo).
You can install these packages using your distribution's package manager.  

## Usage
1. Make sure all the dependencies are installed, if not install them.  
2. Clone this repo by typing:  

	```git clone https://github.com/yankh764/full-system-backup.git```

3. Move to the cloned directory and type:

	```make```   
	```sudo make install```   
	```make clean```

4. Run the program from terminal by typing backup.  
It'll create a configuration file in the ~/.config with the name sys_backup.
Configure it to your needs and prefrences -all instructions are written there-.  
Then make sure that the storage device is mounted, run the program one more time,  
it should execute and do all the cleaning process, create a backup directory  
in storage device then it'll backup all your system there.  

## Latest Updates
1. Simplified the syntax of the configuration file a little bit.
2. Added a make file to simplify the compilation process.
3. Added header guards.
4. Added the option to configure rsync command so you can execlude whatever you want.
5. Edited the config_example so it will be up to date and relevant.
