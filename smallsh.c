/*******************************************************************************************************
* Program Name: Program 3 - smallsh.c
* Class: CS 344-400
* Written By: L. Herrick 
* Description: shell similar to bash shell. Supports the built in commands exit, cd, and status as well
* 		as common UNIX commands.  * 
*
********************************************************************************************************/
#include <sys/types.h> //process id type
#include <sys/wait.h> //waitpid()
#include <unistd.h> //fork, exec, pid type 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

char** parseCommands(char** args);
int doCommands(char** args, int *exitStatus);
void printSig(int *exitSignal);


int main(void){
	int running = 1;
	int exitStatus = 0;
	char* args[513] = {NULL};
	pid_t bgd_pid;

	//loop for each line
	while(running){

		printf(": ");
		fflush(stdout);		
		
		//parsing line
		parseCommands(args);	

		//execute line
		running = doCommands(args, &exitStatus);
		
		//check if background is completed
		bgd_pid = waitpid(-1, &exitStatus, WNOHANG);
		if(bgd_pid > 1){
			//exit
			if(exitStatus == 0){
				printf("Background process %d completed with status of %d \n", bgd_pid, exitStatus);			

			}
			else{	//terminated
				printf("Background process %d ", bgd_pid);
			}
		}//catch error signals for cmd c, kill, etc
		if(WIFEXITED(exitStatus) || WIFSIGNALED(exitStatus)){
			if(exitStatus > 0){
				printf("terminated with %d\n", exitStatus);
				exitStatus = 0;
			}
		}
	}	
	return 0;
}



/*-------------------------------------------------------------------------------------------------
* Function to:  Parse user input to prepare for command reading/recognition
* Param: array of chars*
* Pre: array exists
* Post: parses user input by spaces and puts each non-spaced section into array at index
*--------------------------------------------------------------------------------------------------*/
char** parseCommands(char** args){
	char* single;
	char* cmdLine = malloc(2049);
	int index;
	
	//get input
	fgets(cmdLine, 2048, stdin);

	//parse input into commands
	cmdLine = strtok(cmdLine, "\n");		//remove newline to end for reading purposes
	if(cmdLine != NULL){
		single = strtok(cmdLine, " ");
		index = 0;
		while (single != NULL){
			args[index] = single;			//store it
			single = strtok(NULL, " ");		//keep parsing
			index++;
		}
	}
	free(cmdLine);
	return args;
}





/*-------------------------------------------------------------------------------------------------
* Function to: read/recognize command line arguments and execute those commands.  If execution 
* 		gives an error, a proper error message is sent to the console.  Allows for 
* 		differentiation and execution of foreground and background commands.
* Param: array of char* 
* Pre: array is filled with current command line argument parsed by spaces
* Post: executes command and empties array
*--------------------------------------------------------------------------------------------------*/
int doCommands(char** args, int *exitStatus){
	int index=0;	
	int running = 1;
	int foreground = 1;					//bool for is foreground/background
	int inpFile = 0;					//bool for input file exists
	int outFile = 0;					//bool for output file exists
	pid_t pid, wpid;

	//handle signals
	struct sigaction act;
	act.sa_flags = 0;
	act.sa_handler = SIG_IGN;
	sigaction(SIGINT, &act, NULL);

	//handle arguments
	if(args[index] != NULL){	

		//if "#"
		if((strncmp(args[index], "#", 1)) == 0){
			//placeholder - do nothing	
		}		

		//if "exit"
		else if((strncmp(args[index], "exit", 4)) == 0){
			
			//update status to end main's loop
			running = 0;
		}
		
		//if "cd"
		else if((strncmp(args[index], "cd", 2)) == 0){
			//go to root
			if(args[index + 1] == NULL){
				char cwd[100];			
				chdir(getenv("HOME"));			//find home path
				getcwd(cwd, sizeof(cwd));
				printf("CWD: %s\n", cwd);
				
					
			}
			//go to arg directory
			else{
				chdir(args[index + 1]);			//cd to user input
				char cwd[100];
				getcwd(cwd, sizeof(cwd));
				fprintf(stdout, "CWD: %s\n", cwd);
			}
		}
		
		//if "status"
		else if((strncmp(args[index], "status", 6)) == 0){
			printSig(exitStatus);
		}

		//it is a command
		else{
			//handle files if applicable		
			int i;
			int fd, fd2;
			int wFileTrue = 0;
			int rFileTrue = 0;
					
			for(i=0; args[i+1] != NULL; i++){		
				//input file
				if((strncmp(args[i], ">", 1)) == 0){
					//open, write			
					fd = open(args[i+1], O_WRONLY|O_CREAT|O_TRUNC, 0644);
					if(fd == -1){
						//perror("open");
						//fflush(stdout);
						*exitStatus = 1;
					}else{ //success - prepare for exec
						args[1] = NULL;
						args[2] = NULL;				
						wFileTrue = 1;
					}
		
				}
				//output file
				else if((strncmp(args[i], "<", 1)) == 0){		
					//open, read
					fd = open(args[i+1], O_RDONLY);
					if(fd == -1){
						//perror("open");
						//fflush(stdout);
						*exitStatus = 1;
						args[0] = NULL;
						args[1] = NULL;
						args[0] = args[2];
						args[2] = NULL;
					}else{ //succes prepare for exec
						args[1] = NULL;
						args[2] = NULL;
						rFileTrue = 1;
					}
				}
			}

			//determine if bg/fb
			int x;
			for(x=0; args[x] != NULL; x++){
				if((strncmp(args[x], "&", 1)) == 0){
					foreground = 0;
					//get rid of & to read command
					args[x] = NULL;
				}
			}
			//fork
			pid = fork();

			//child - files and exec
			if (pid == 0){
				if(foreground == 1){
					//don't ignore signals
					act.sa_handler = SIG_DFL;
					act.sa_flags = 0;
					sigaction(SIGINT, &act, NULL);
					
					//handle files
					if(wFileTrue == 1){
						//redirect, close fd
						fd2 = dup2(fd, 1);
						if(close(fd) != 0){
							*exitStatus = 1;
							_Exit(1);
						}			
					}
					//redirect for opned readonly
					if(rFileTrue == 1){
						fd2 = dup2(fd, 0);
						if(close(fd) != 0){
							*exitStatus = 1;
							_Exit(1);
						}
					}					
				}
				//background
				else{
					//bg should redir input to /dev/null
					fd = open("/dev/null", O_RDONLY);
					if(fd ==  -1){
						perror("open");
						fflush(stdout);
						*exitStatus = 1;
						_Exit(1);
					}
					else{ //success - dup
						fd2 = dup2(fd, 0);
						close(fd);
					}		
					
				}
				//exec with error handling
				if(execvp(args[index], args) == -1){
					perror("smallsh");
					fflush(stdout);
					*exitStatus = 1;
					_Exit(1);
				}
				else{//success
					*exitStatus = 0;
				}
				//cleanup
				if(rFileTrue == 1 || wFileTrue == 1){
					close(fd2);
					rFileTrue = 0;
					wFileTrue = 0;
				}
				
			}
			//parent - wait
			else if(pid > 0){
				if(foreground == 1){	
					wpid = waitpid(-1, exitStatus, WUNTRACED);
					//no advanced signals, exit 1
					if(*exitStatus > 15){
						*exitStatus = 1;
					}
				}
				//the shell will print the process id of background process
				else{
					printf("PID of background process: %d\n", pid);
					wpid = waitpid(-1, exitStatus, WNOHANG);
			
				}
				
			}
			//error
			else{
				//change status to show error
				*exitStatus = 1;
				perror("fork");
				_Exit(1);
			}
		}
	} 

	//cleanup
	index = 0;
	while(args[index]!= NULL){
		args[index]=NULL;
		index++;
	}
	return running;
}


/*-------------------------------------------------------------------------------------------------
* Function to:  Prints the exit status of most recent command completed.
* Param: pointer to an int variable keeping track of current status 
* Pre: pointer does not point to null
* Post: prints exit status to console
*--------------------------------------------------------------------------------------------------*/
void printSig(int *exitStatus){
	//exit = 0
	printf("Exit status: %d\n", *exitStatus);
}




