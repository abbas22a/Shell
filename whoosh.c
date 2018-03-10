#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>


/** global variables to keep track of the indexes filled in args and path array **/
char** array;
int count=0;
char** paths;
int pathCounter=0;


/**  Prints standard error when error is encountered
*
**/
void reportError() {
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}


/**
*   Helper function that frees only slots that have been assigned memory in the array
*/
void freeArray(char** arr, int counter){

  int i=0; 
  while(i<counter){
    free(arr[i]);
    i++;
  }

}


/**
* Checks if the file path exists. Returns 0 if it does and returns 1 when it doesnot
*/
int fileExists(char* val){
  struct stat buf;
  if(stat(val,&buf)>=0){
    return 0;
  }
  return 1;
}

/**
* Executes external commands if they exist
*/
void executeExternalCommand()
{
  // counter to keep track of index in the Paths array
  int z=0;
  while(z < pathCounter){

    // concatenates the path and the command to be executed
    char* col = malloc(128*sizeof(char*));
    strcpy(col, paths[z]);
    col = strcat(strcat(col, "/"), array[0]);

    // if the command exists in the path
    if((fileExists(col))==0){

      // fork the parent process
      pid_t pid = fork();
     

      if (pid<0){
        return;
      }
      
      // if in the child process
      else if (pid == 0){   

        // excecute the external command, if it doesn't throw an  error.   
        if(execv(col, array)<0){
          reportError();
        }
      
        // free both the arguments and the paths array
        freeArray(array, count);
        freeArray(paths, pathCounter);
       
        free(array);
        free(paths);
        free(col);
        exit(0);
      }
      
      // if the parent process
      else {
        // wait for the child process to end
        wait(NULL);
        free(col);
    
      }
      break;
    }
    
    // if the file does not exist and the counter is at the last path in the paths array, report error   
    else if((fileExists(col)==1) & (z==pathCounter-1)){
      reportError();
    }
    free(col);
    z++;
  }
  
}


/**
*   This function frees all the memory used and then exists out of the program
*/
void shell_exit(){

  freeArray(array, count);
  freeArray(paths, pathCounter);
  free(array);
  free(paths);
  exit(0);
}

/**
*  Prints out the path of the current directory
*/
void shell_pwd(){
  char cwd[1024];
  getcwd(cwd, sizeof(cwd));
  printf("Dir: %s\n", cwd);
}


/**
*  Goes to the home directory if not the directory is not specififed, otherwise changes to the specified directory.
*/
void shell_cd(char* argv){
  if(argv==NULL){
    chdir(getenv("HOME"));
  }
  else{
    if(chdir(argv)==-1){
      reportError();
    }
  }
}

/**
*  Breaks the user input by space and saves every word in the stdin in a separate index in the array
*/
void readLine(char* line){
    
  // counter to keep track of the filled slots in the array
  count=0;
  array = malloc(sizeof(char*)*128);
    
  // break the parsed user input into separate words.
  char *ptr;
  ptr = strtok(line, " ");
  int i=0;
  while(ptr!=NULL){
    array[i]=strdup(ptr);
    ptr= strtok(NULL, " ");
    i++;
  }
   
  // set the last index as NULL so that the execve commands does not throw an error.
  array[i] = NULL;
  count =i;
}


/**
*  Prints the paths saved in the paths array
*/
void printPath(){
  int i=0;
  while(i < pathCounter){
    printf("%s ", paths[i]);
    i++;
  } 
  printf("\n");
}


/**
* Sets the default path to /bin and creates a paths array to hold the paths
*/
void setDefaultPath(){
  paths =  malloc(sizeof(char*)*128);
  char* d = "/bin";
  paths[0] = strdup(d);
  pathCounter++;
}


/**
*  Overwrites the previous paths and saves the paths in the paths array
*/
void setPath(char* p){
  paths[pathCounter] = strdup(p);
  pathCounter++;
}


/**
*  This function decides if the  passed in user command is build-in or an external command. This returns 0 if the command is build-in, otherwise returns 1
*/
int isbuildInCommand(){

  if((strcmp(array[0], "pwd")==0) | (strcmp(array[0], "cd")==0) | (strcmp(array[0], "setpath")==0)  | (strcmp(array[0], "printpath")==0) | (strcmp(array[0], "exit")==0)){
    return 0;
  }
  return 1;
}


/**
*  Executes the specified build-in command
*/
void executeBuildCommand(){
  

  if(strcmp(array[0], "cd")==0){
    shell_cd(array[1]);
  }

  else if((strcmp(array[0], "pwd")==0) & (count==1)){
    shell_pwd();
  }
    
  else if( (strcmp(array[0], "exit")==0) & (count==1)){
    shell_exit();
  }

  else if( (strcmp(array[0], "setpath") == 0) & (count>=2)){

    freeArray(paths, pathCounter);
    
    // reset the path counter  
    pathCounter=0;
    int j = 1;

    // add the path to the path array
    while(j < count){
      setPath(array[j]);
      j++;
    }
  }

  else if((strcmp(array[0], "printpath") == 0) & ((count)==1)){
    printPath();
  }

  else{
    reportError();
  }
   
}


/**
*  Create the shell by priniing the prompt whoosh in the while loop and executiong both external and in-build commands appropriately.
*/
void createShell(){

  while(1){

    printf("whoosh> :" );
    char line[500];
    if(sizeof(stdin)>128){
     reportError();
    }

    if(fgets(line,sizeof(line),stdin)==NULL){
      reportError();
    }
  
    char* str = strchr(line, '\n');   // find the newline character
    if (str){
      *str = '\0';
    }

    readLine(line);
    // if no stdin, continue 
    if(array[0]==NULL){
      continue;
    }

    if(isbuildInCommand()==0){
      executeBuildCommand();
    }

    else if(isbuildInCommand()==1){
      executeExternalCommand();
    } 

    // free memory allocated to the array
    freeArray(array, count); 
 }
}


/**
*  Create the shell and set the default path to /bin
*/
int main(int argc, char** argv){

  if(argc>1){
    reportError();
    exit(0);
  }
  setDefaultPath();
  createShell();
  return 0;
}