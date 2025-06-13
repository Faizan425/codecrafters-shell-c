#include <stdio.h>
#include <stdlib.h>
#include<string.h>
int check_echo(char  str[]){
	// check that the input contains echo
	if(strncmp(str, "echo",4)!=0) return 0;
	if(str[4] == '\0' || str[4] == ' ')
		return 1;
	return 0;
}

char* remove_command_and_get_string(char input[] ){
	int i=4;
	while(input[i] == ' '){
	i++;
	}
	return &input[i];
}
int main(int argc, char *argv[]) {
  // Flush after every printf
  int flag=1;
  while(flag){
  setbuf(stdout, NULL);

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
  else if(check_echo(input)){
	  char *arg = remove_command_and_get_string(input);
	  printf("%s\n",arg);
  }
  else{
  printf("%s: command not found\n", input);
  }
  }
  return 0;
}
