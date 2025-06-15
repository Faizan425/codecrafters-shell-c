#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include<ctype.h>
#include<unistd.h>
#include <limits.h>
#include<sys/types.h>
#include<sys/wait.h>

int  get_arguments(char* inputs_copy,char **argument, int max_args);
char* build_path(const char *path_token, const char *command,char* candidate_path, size_t candidate_path_size){
	//if(strlen(path_token)+1+strlen(command)+1 > candidate_path_size){return NULL;}
	//snprintf(candidate_path, candidate_path_size,"%s", path_token);
	//if(path_token[0]=='\0'){
	//	snprintf(candidate_path, candidate_path_size,"%s./%s",candidate_path,command);
	//}
	//else{
	//	snprintf(candidate_path, candidate_path_size,"%s/%s",candidate_path,command);
	//}
	//return candidate_path;
	const char *use_dir =(path_token[0]== '\0')?".":path_token;
	size_t dir_len= strlen(use_dir);
	size_t cmd_len=strlen(command);
	if(dir_len +1 + cmd_len +1 > candidate_path_size){
	return NULL;
	}
	snprintf(candidate_path, candidate_path_size, "%s/%s", use_dir, command);
	return candidate_path;
}
int check_valid_command(char* command){
	if(strcmp(command,"echo")==0 || strcmp(command, "type")==0  || strcmp(command,"exit")==0){
		return 1;
	}
	else {
		return 0;
	}
}
static int skip_leading_spaces(const char *s){
	int i=0;
	while(s[i] != '\0' && isspace((unsigned char)s[i])){
		i++;
	}
	return i;
}
char* remove_command_and_get_string(char input[]);
int check_echo(char  str[]){
	// check that the input contains echo
	if(strncmp(str, "echo",4)!=0) return 0;
	if(str[4] == '\0' || str[4] == ' ')
		return 1;
	return 0;
}


void execute_echo(char input[]){
	char *arg = remove_command_and_get_string(input);
	printf("%s\n",arg);
}
void execute_type_deprecated(char input[]){
	char *arg = remove_command_and_get_string(input);
	if(arg[0]=='\0'){
		printf("type : usage : type NAME\n");
	}
	else{
	if(check_valid_command(arg)){
		printf("%s is a shell builtin\n",arg);
	}
	else{
		printf("%s: not found\n",arg);
	}
	}
}

void execute_type(char input []){
	
	char *arg=remove_command_and_get_string(input);
	if(check_valid_command(arg)){
		execute_type_deprecated(input);
		return;
	}
	int flag=0; //flag to indicate if we found the command or not.
	
	if(strchr(arg,'/')!=NULL){
		if(access(arg,X_OK)==0){
			printf("%s\n", arg);
		}
		else{
			printf("%s: not found\n",arg);
		}
		return;
		
	}
else{
char candidate_path[PATH_MAX];
char *path_env= getenv("PATH");
if(path_env== NULL||path_env[0]=='\0'){
if(build_path("", arg, candidate_path, sizeof(candidate_path)) !=NULL && access(candidate_path, X_OK)==0){
	printf("%s is %s\n",arg, candidate_path);
}
else{
	printf("%s: not found\n",arg);
}
return;
}

char  *path_copy= strdup(path_env);
if(!path_copy){
	perror("strdup");
	exit(1);
}
char *p=path_copy;

char *pointer_to_path_token;
while((pointer_to_path_token = strsep(&p, ":")) !=NULL){
	if(build_path(pointer_to_path_token,arg,candidate_path,sizeof(candidate_path))==NULL)
		continue;
	
	if(access(candidate_path, X_OK)==0){
		flag=1;
		break;
	}
}
free(path_copy);
if(flag){
	printf("%s is %s\n",arg,candidate_path);
}
else{
	printf("%s: not found\n",arg);
}
}

}

int get_arguments(char* inputs_copy, char** argument,int max_args){
	int argc = 0;
	char *p;
	while(*p && isspace((unsigned char)*p)) p++;
	while(*p !='\0' && argc < max_args){
		argument[argc++] = p;
		while(*p && !isspace((unsigned char)*p)) p++;
		if(*p=='\0') break;
		*p='\0';
		p++;
		while(*p && isspace((unsigned char)*p)) p++;

	
	}
	argument[argc]=NULL;




}
void  execute_custom(char input[]){
	char *argument[100];
	char *inputs_copy=strdup(input);
		if(!inputs_copy){
			perror("strdup");
			return;
		}
		int argc=get_arguments(inputs_copy,argument,100);
		if(argc==0){
			free(inputs_copy);
			return;
		}
		pid_t pid=fork();
		if(pid<0){
			perror("fork");
			return;
		}
		if(pid==0){
			execvp(argument[0], argument);
			perror(argument[0]);
			return;
		}
		else{
			int status;
			if(waitpid(pid, &status,0)<0){
				perror("waitpid");
			}
			free(inputs_copy);
		}
			
}

char* remove_command_and_get_string(char input[] ){
	int i=skip_leading_spaces(input);
	while(input[i] != '\0' && !isspace((unsigned char)input[i])){
	i++;
	}
	while(input[i] != '\0' && isspace((unsigned char)input[i])){
		i++;
	}
	return &input[i];
}
int retrieve_command(char input[], char* command, size_t cmd_buf_size){
	int i=skip_leading_spaces(input);
	if(input[i]=='\0'){
	command[0]='\0';
	return 0;
	}
	
	size_t len =0;
	while(input[i] != '\0' && !isspace((unsigned char)input[i])  && len+1 < cmd_buf_size){
		command[len++] = input[i++];
	}
	command[len] = '\0';
	return 1;
}

void execute_command(char *command,char input[]){
	if(strcmp(command,"echo")==0){
		execute_echo(input);
	}
	else if(strcmp(command, "type")==0){
		execute_type(input);
	}
	else{
		execute_custom(input);
	}
}

int main(int argc, char *argv[]) {
  // Flush after every printf
  int flag=1;
  setbuf(stdout, NULL);
  while(flag){

  // Uncomment this block to pass the first stage
  printf("$ ");

  // Wait for user input
  char input[100];
  if(fgets(input, 100, stdin) == NULL){
	  printf("\n");
  }
  input[strcspn(input, "\n")]= '\0';
  if(input[0]=='\0'){
	  continue;
  }
  if(strcmp(input, "exit 0")==0){
	  flag=0;
  }
 // else if(check_echo(input)){
	 // char *arg = remove_command_and_get_string(input);
	 // printf(%s,arg);
 // }
 // else{
 // printf(%s: command not found, input);
 // }
  else{
  char command[100];
  if(!retrieve_command(input, command, sizeof(command))){
	  continue;
  }
  execute_command(command, input);
  }
  }
  
  return 0;
}

