#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include<ctype.h>
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
void execute_type(char input[]){
	char *arg = remove_command_and_get_string(input);
	if(arg[0]=='\0'){
		printf("type : usage : type NAME\n");
	}
	else{
	printf("%s is a shell builtin\n",arg);}
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
		printf("%s: command not found\n",input);
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

