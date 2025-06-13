#include <stdio.h>
#include <stdlib.h>
#include<string.h>
int check_echo(char  str[]){
	char hold[5];
	int i=0;
	while(i<strlen(str) && str[i]!=' '){
		hold[i]=str[i];
		i++;
	}
	hold[i]='\0';

	if(strcmp(hold,"echo")==0){
		return 1;
	}
	else{
		return 0;
	}
}

char* remove_command_and_get_string(char input[] ){
	int i=0;
	while(i<strlen(input)){
		if(input[i]!=' '){
		break;
		}
		i++;
	}
	i=i+1;
	int new_len=strlen(input) - i;
	int w=0;
	char string[new_len];
	while(w<new_len){
	string[w]=input[i];
	w++;
	i++;
	}
	string[w++]='\0';
	input=string;
	return input;
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
	  printf("%s\n",remove_command_and_get_string(input));
  }
  else{
  printf("%s: command not found\n", input);
  }
  }
  return 0;
}
