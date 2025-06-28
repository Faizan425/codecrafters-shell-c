#include <stdio.h>
#include <stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<ctype.h>
#include<unistd.h>
#include <limits.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<errno.h>
#include<readline/readline.h>
#include<readline/history.h>
# define MAX 100
# define HISTORY_MAX 1000
//static char *history[HISTORY_MAX];
//static int history_count=0;
const char *histfile;
static int last_appended=0;
static bool in_pipeline_child=false;
int retrieve_command(char input[], char *command, size_t cmd_buf_size);
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
/*int store_history(char *input){
	if(history_count== HISTORY_MAX){
		free(history[0]);
		memmove(&history[0],&history[1],sizeof(char*)*(HISTORY_MAX-1));
		history_count=HISTORY_MAX-1;
	}
	history[history_count++]=strdup(input);
	if(!history[history_count-1]){
		perror("strdup");
		return 1;
	}
	return 0;
}*/
int execute_history_append(const char *path){
	int total=where_history();
	int to_write=total-last_appended+1;
	if(to_write >0){
		if(append_history(to_write,path)!=0){
			perror("history -a");
			return 1;
		}
		last_appended=total+1;
	}
	return 0;
}
int execute_history_write(const char *path){
	if(write_history(path)!=0){
		perror("history -w");
		return 1;
	}
	return 0;
}
int execute_history_read(const char *path){
	if(read_history(path)<0){
		perror("history -r");
		return 1;
	}
	return 0;
}
int execute_history(char *input){
	char *argv[MAX];
	int argc=get_arguments(input,argv,MAX);
	HIST_ENTRY **hl=history_list();
	int start=0;
	if(argc==1){
		for(int i=start; hl[i]; i++){
			printf("%5d %s\n",i+1,hl[i]->line);
		}
		return 0;
	}
	else if(argc==2){
		char *endptr;
		long n=-1;
		n=strtol(argv[1],&endptr,10);
		if(*endptr !='\0' || n<0){
			fprintf(stderr,"history: invalid number: %s\n",argv[1]);
			return 1;
		}
		int total=where_history();
		if(n >0 && n<total) start = total-n+1;
		for(int i=start; hl[i]; i++){
			printf("%5d %s\n",i+1,hl[i]->line);
		}

	}
	else{
		if(strcmp(argv[1],"-r")==0){
		return execute_history_read(argv[2]);
		}
		else if(strcmp(argv[1],"-w")==0){
			return execute_history_write(argv[2]);
		}
		else if(strcmp(argv[1],"-a")==0){
			return execute_history_append(argv[2]);
		}
		else{
			fprintf(stderr,"history: invalid arguments %s\n",argv[1]);
			return 1;
		}
		
	}
	return 0;
}

int get_file(const char *args,  const char *file_name){
	int fd=-1;
	if(strcmp(args,">")==0 || strcmp(args,"1>")==0){
		fd=open(file_name, O_WRONLY | O_CREAT | O_TRUNC ,0666);
		if(fd==-1){
		perror("open");
		}
	
	}
	else if(strcmp(args,"2>")==0){
		fd=open(file_name,O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if(fd==-1){
			perror("open");
		}
	}
	else if(strcmp(args, ">>")==0 || strcmp(args, "1>>")==0){
		fd=open(file_name, O_WRONLY | O_CREAT | O_APPEND,0666);
		if(fd==-1){
			perror("open");
		}
	}
	else if(strcmp(args, "2>>")==0){
		fd=open(file_name , O_WRONLY | O_CREAT | O_APPEND, 0666);
		if(fd==-1){
			perror("open");
		}
	}
	return fd;
}
int handle_escaped_characters(const char *p, char buf[],int* position, size_t len){
	int i=1;
	if(!isdigit(*p) || *p>='8') {
		if(*position < len-1){
			buf[*position]=*p;
			(*position)++;
		}
		
	}
	else{
		
		while(isdigit(*(p+i))  && *(p+i)>='0' && *(p+i)<='7'){
		   i++;
		}
		int ascii_code=strtol(p,NULL,8);
		if(*position <len-1){
		buf[*position]=(char)ascii_code;
	        (*position)++;	
		}
	}
	return i;
}
int check_valid_command(char* command){
	if(strcmp(command,"echo")==0 || strcmp(command, "type")==0  || strcmp(command,"exit")==0 || strcmp(command,"pwd")==0 ||strcmp(command, "cd")==0 || strcmp(command,"history")==0){
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
int execute_pwd(char input []){
	char *cwd=getcwd(NULL,0);
	if(cwd==NULL){
		perror("pwd");
		return 1;
	}
	printf("%s\n",cwd);
	free(cwd);
	return 0;
}

int handle_redirections(char *argv[]){
int fd_out = -1;
int fd_err=-1;
int fd_in=-1;
// scan through argv
for(int i=0; argv[i]!=NULL; i++){
const char *tok= argv[i];
int is_out=0, is_append_out=0;
int is_err = 0, is_append_err = 0;
int is_in=0;
// identify token
 if(strcmp(tok,">")==0 ||  strcmp(tok, "1>")==0){
 is_out=1;
 }
 else if(strcmp(tok,">>") == 0 || strcmp(tok, "1>>")==0){
	 is_append_out=1;
 }
 else if(strcmp(tok,"2>")==0){
 is_err=1;
 }
 else if(strcmp(tok,"2>>")==0){
 is_append_err=1;
 }
 else if(strcmp(tok,"<")==0){
 is_in=1;
 }
 else { 
	 continue;
 }
 if(argv[i+1]==NULL){
 fprintf(stderr, "%s: syntax error: expected filename after %s\n", argv[0],tok);
 return -1;
 }
 const char *file = argv[i+1];
int newfd;
if(is_out){
	newfd=open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
}
else if(is_append_out){
	newfd=open(file, O_WRONLY | O_CREAT | O_APPEND,0666);
}
else if(is_err){
	newfd=open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
}
else if(is_append_err){
	newfd=open(file, O_WRONLY | O_CREAT | O_APPEND, 0666);
}
else if(is_in){
	newfd=open(file, O_RDONLY);
}
else {
	//should not happen
	continue;
}
if(newfd<0){
	perror(file);
	return -1;
}
// close previous fd if any
if(is_out || is_append_out){
	if(fd_out >=0) close (fd_out);
	fd_out= newfd;

}
else if(is_err || is_append_err){
	if(fd_err>=0) close(fd_err);
	fd_err=newfd;
}
else if(is_in){
if(fd_in>=0) close (fd_in);
fd_in=newfd;
}
int j=i;
while(argv[j+2]!=NULL){
	argv[j] = argv[j+2];
	j++;
}
argv[j] = NULL;
argv[j+1]=NULL;
i--; //check again
 
}
if(fd_in>=0){
	if(dup2(fd_in, STDIN_FILENO)<0){
		perror("dup2 stdin");
		close(fd_in);
		return -1;
	}
	close(fd_in);
}
if(fd_out >=0){
	if(dup2(fd_out, STDOUT_FILENO)<0){
		perror("dup2 stdout");
		close(fd_out);
		return -1;
	}
	close(fd_out);
}
if(fd_err>=0){
	if(dup2(fd_err, STDERR_FILENO)<0){
		perror("dup2 stderr");
		close(fd_err);
		return -1;
	}
	close(fd_err);
}
return 0;

}
const char *get_final_path(const char *rel_path){
	//1. Get current working directory
	char *cwd=getcwd(NULL,0);
	if(cwd == NULL){
		perror("getcwd");
		return NULL;
	}
	//2. Prepare components array (max number of segments)
	//Decide a maximum number of components; PATH_MAX is the upper bound on length
	char *copy=strdup(cwd);
	if(!copy){
		perror("strdup");
		free(cwd);
		return NULL;
	}
	char *components[PATH_MAX];
	int comp_count = 0;
	
	char *saveptr = NULL;
	char *tok=strtok_r(copy,"/", &saveptr);
	while(tok){
		// skip empty tokens , strtok won't deliver empty tokens for consecutive slashes
		components[comp_count++]=strdup(tok);
		if(!components[comp_count-1]){
			perror("strdup");
			free(copy);
			for(int i=0; i<comp_count-1; i++) free(components[i]);
			free(cwd);
			return NULL;
		}
		tok= strtok_r(NULL, "/",&saveptr);
	}
	free(copy);

	free(cwd);
	char *rp_copy=strdup(rel_path);
	if(!rp_copy){
		perror("strdup");
		for(int i=0; i<comp_count; i++) free(components[i]);
		return NULL;
	}
	char *saveptr2 = NULL;
	char *seg= strtok_r(rp_copy, "/", &saveptr2);
	while(seg){
		if(strcmp(seg,"")==0 || strcmp(seg,".")==0){
			
		}
		else if(strcmp(seg, "..")==0){
			if(comp_count>0){
				free(components[--comp_count]);
			}
		}
		else{
			components[comp_count++]=strdup(seg);
			if(!components[comp_count-1]){
				free(rp_copy);
				for(int i=0; i<comp_count-1; i++) free(components[i]);
				return NULL;
			}
		}
		seg=strtok_r(NULL, "/", &saveptr2);
	}
	free(rp_copy);

	// 5. Reconstruct newpath
	char newpath[PATH_MAX];
	if(comp_count==0){
		strcpy(newpath,"/");
	}
	else{
		newpath[0]='\0';
		for(int i=0; i< comp_count; i++){
			// check length concatenation
			size_t needed  = strlen(newpath)+1+strlen(components[i])+1;
			if(needed > PATH_MAX){
				fprintf(stderr, "cd: path too long\n");
				for(int j=0; j<comp_count; j++) free(components[j]);
				return NULL;
			}
			strcat(newpath, "/");
			strcat(newpath, components[i]);
		}
	}
	//6. validate existence
	struct stat st;
	if(stat(newpath, &st)!=0){
		fprintf(stderr,"cd: %s: %s\n", newpath, strerror(errno));
		for(int i=0; i<comp_count; i++) free(components[i]);
		return NULL;
	}
	if(!S_ISDIR(st.st_mode)){
		fprintf(stderr,"cd: %s: Not a directory\n", newpath);
		for(int i=0; i<comp_count; i++)free(components[i]);
		return NULL;
	}

	// 7. cleanup component strings
	for(int i=0; i< comp_count; i++) free(components[i]);
	//8. return a malloc'd copy of newpath
	return strdup(newpath);
}
/*
typedef struct{
	char *token;
	bool is_first;
	bool is_last;
}TokenInfo;
TokenInfo get_next_token(char **saveptr, char *str, const char *delim){
	TokenInfo result;
	result.is_first= (str !=NULL);
	result.token=strtok_r(str,delim,saveptr);
	result.is_last=false;
	if(result.token!=NULL){
		char *tmp = *saveptr;
		char *next = strtok_r(NULL,delim,saveptr);
		if(next== NULL) result.is_last = true;
		//restore saveptr so next call continues correctly
		*saveptr = tmp;
	}
	return result;
}
void execute_stage(char input[]){
	char *input_copy=strdup(input);
	if(!input_copy){
	perror("strdup");
	return;
	}
	char *saveptr;
	TokenInfo token;
	int pipefd[2];
	do{
		if(pipe(pipefd)==-1){
			perror("pipe");
			exit(EXIT_FAILURE);
		}
		token=get_next_token(&saveptr, input_copy,"|");
		input_copy=NULL;
		if(token.token){
			pid_t pid=fork();
			if(pid<0){
				perror("fork");
			}
			else if(pid ==0){ // child process
			if(token.is_first){ // first stage , read from stdout
				close(STDIN_FILENO);
				close(STDOUT_FILENO);
				if(dup2(pipefd[1],STDOUT_FILENO)<0){
					perror("dup2");
					close(pipefd[0]);
					close(pipefd[1]);
					free(input_copy);
					_exit(1);
				}
				close(pipefd[0]);
				close(pipefd[1]); // close unused write end
				execute_custom(token.token);
			}
			else if(token.is_last){
				close(STDIN_FILENO);
				if(dup2(pipefd[0], STDIN_FILENO)<0){
					close(pipefd[0]);
					close(pipefd[1]);
					free(input_copy);
					_exit(1);
				}
				execute_custom(token.token);

			}
			else{
				close(STDIN_FILENO);
				close(STDOUT_FILENO);
				if(dup2(pipefd[0],STDIN_FILENO)<0){
					perror("dup2");
					free(input_copy);
					close(pipefd[0]);
					close(pipefd[1]);
					_exit(1);
				}
				close(pipefd[0]);
				close(pipefd[1]);
				execute_custom(token.token);
			}
		}
			else{
				//parent process
			int status;
			waitpid(pid, &status,0);
			}
		}
	}while(token.token!=NULL);
	free(input_copy);
}
*/
int execute_cd(char input []){
#define MAX_ARGS 100
	char *argstr=remove_command_and_get_string(input);
	char *argstr_copy=strdup(argstr ? argstr : " ");
	if(!argstr_copy){
		perror("strdup");
		return 1;
	}
	char *buf[MAX_ARGS];
	int argc=get_arguments(argstr_copy, buf, MAX_ARGS);
	int status=0;
	if(argc==0 || strcmp(argstr_copy,"~")==0){
		const char *home=getenv("HOME");
		if(!home || home[0]=='\0'){
			fprintf(stderr,"cd: %s: %s\n",home,strerror(errno));
			free(argstr_copy);
			status = 1;
		}
		if(chdir(home)!=0){
			fprintf(stderr, "cd: %s: %s\n",home, strerror(errno));
			free(argstr_copy);
			status=1;
		}
		//free(argstr_copy);
	}
	else if(argc>1){
		//free(argstr_copy);
		fprintf(stderr,"%s: Too many arguments. \n","cd");
		status=1;
	}
	else {
	const char *path0=buf[0];
	const char *path=path0;
	if(path[0]!='/'){
		char *resolved=(char*)get_final_path(path);
		if(!resolved){
			status=1;
		}
		else {
			path=resolved;
		}
	}if(status==0){
	if(chdir(path)!=0){
		fprintf(stderr, "cd: %s: %s\n",path,strerror(errno));
		//free(argstr_copy);
		status = 1;
	}
	}
	if(path0[0]!='/') free((void*)path);
}
	
	free(argstr_copy);
	return status;
	
}
int execute_echo(char input[]){
#define MAX 100
	char *arg=remove_command_and_get_string(input);
	if(arg[0]=='\0'){
		printf("\n");
		return 0;
	}
	char *arg_copy=strdup(arg);
	if(!arg_copy){
		perror("strdup");
		return 1;
	}
	char *argv[MAX];
	int argc=get_arguments(arg_copy,argv,MAX);
	if(argc==0){
		printf("\n");
		return 0;
	}
	int fd_out=-1, fd_err=-1;
	int newfd=-1;
	const char *file_name=NULL;
	const char *tok;
	for(int i=0; argv[i]!=NULL; i++){
		tok=argv[i];
		if(strcmp(tok,">")==0 || strcmp(tok,">>")==0 || strcmp(tok,"1>>")==0 || strcmp(tok,"1>")==0){
			if(argv[i+1]==NULL){
				fprintf(stderr,"%s: syntax error: expected filename after %s\n",argv[0],tok);
				return 1;
			}
			file_name=argv[i+1];
			newfd=get_file(tok,file_name);
			if(newfd<0){
				fprintf(stderr,"file:%s open error\n",file_name);
				return 1;
			}
			if(fd_out >=0) close(fd_out);
			fd_out=newfd;
			int j=i;
			while(argv[j+2]!=NULL){
				argv[j]=argv[j+2];
				j++;
			}
			argv[j]=NULL;
			argv[j+1]=NULL;
			i--;
			argc-=2;
		}
		else if(strcmp(tok,"2>")==0 || strcmp(tok,"2>>")==0){
			if(argv[i+1]==NULL){
				return 1;
			}
			file_name=argv[i+1];
			newfd=get_file(tok,file_name);
			if(newfd<0){
				fprintf(stderr, "file:%s open error\n",file_name);
				return 1;
			}
			if(fd_err>=0) close(fd_err);
			fd_err=newfd;
			int j=i;
			while(argv[j+2]!=NULL){
				argv[j]=argv[j+2];
				j++;
			}
			argv[j]=NULL;
			argv[j+1]=NULL;
			i--;
			argc-=2;
		}
		
	}
	int saved_out=-1;
	int saved_err=-1;
	// final redirection
	if(fd_out>=0){
		saved_out=dup(STDOUT_FILENO);
		if(saved_out<0){
			perror("dup");
			close(fd_out);
			close(saved_out);
			return 1;
		}
		if(dup2(fd_out,STDOUT_FILENO)<0){
			perror("dup2");
			close(fd_out);
			close(saved_out);
			return 1;
		}
		close(fd_out);
	}
	if(fd_err>=0){
		saved_err= dup(STDERR_FILENO);
		if(saved_err<0){
			perror("dup");
			close(fd_err);
			close(fd_out);
			return 1;
		}
		if(dup2(fd_err, STDERR_FILENO)<0){
			perror("dup2");
			close(fd_err);
			close(saved_err);
			return 1;
		}
		close(fd_err);
	}
		// print the arguments
		for(int i=0; i <argc; i++){
			if(i>0) putchar(' ');
			fputs(argv[i],stdout);
		}
		putchar('\n');
		if(saved_out>=0){
			if(dup2(saved_out,STDOUT_FILENO)<0){
				perror("dup2 restore");
				close(saved_out);
				return 1;
			}
			close(saved_out);
		}
		if(saved_err>=0){
			if(dup2(saved_err, STDERR_FILENO)<0){
				perror("dup2 restore");
				close(saved_err);
				return 1;
			}
			close(saved_err);
		}
		
	
	free(arg_copy);
	return 0;
}
/*
int execute_echo(char input[]){
	char *arg = remove_command_and_get_string(input);
	if(arg[0]=='\0'){
		printf("\n");
		return 0;
	}
	char *arg_copy = strdup(arg);
	if(!arg_copy){
		perror("strdup");
		return 1;
	}
	char *argv[MAX];
	int argc=get_arguments(arg_copy, argv,MAX);
	if(argc==0){
		printf("\n");
	}
	else{
		const char *tok;
		int fd_out=-1;
		int newfd=-1;
		int fd_err=-1;
		const char *file_name=NULL;
		
		for(int i=0; argv[i]!=NULL; i++){
			tok=argv[i];
			if(strcmp(tok,">")==0 || strcmp(tok,"1>")==0 || strcmp(tok,">>")==0 || strcmp(tok,"1>>")==0){
			if(argv[i+1]==NULL){
				fprintf(stderr,"%s: syntax error: expected filename after %s\n", argv[0],tok);
				return;
			}
			file_name=argv[i+1];
			newfd=get_file(tok,file_name);
			if(newfd<0){
				perror(file_name);
				return;
			}
			if(fd_out >=0) close(fd_out);
			fd_out=newfd;
			int j=i;
			while(argv[j+2]!=NULL){
				argv[j]=argv[j+2];
				j++;
			}
			argv[j]=NULL;
			argv[j+1]=NULL;
			i--;
			argc=argc-2;  // adjust to get actual value of argc
			}
			else if(strcmp(tok,"2>")==0 || strcmp(tok,"2>>")==0){
			if(argv[i+1]==NULL){
				fprintf(stderr,"%s: syntax error: expected filename after %s\n", argv[0],tok);
				return;
			}
			file_name=argv[i+1];
			newfd=get_file(tok,file_name);
			if(newfd<0){
				perror(file_name);
				return;
			}
			if(fd_err>=0) close(fd_err);
			fd_err=newfd;
			int j=i;
			while(argv[j+2]!=NULL){
				argv[j]=argv[j+2];
				j++;
			}
			argv[j]=NULL;
			argv[j+1]=NULL;
			i--;
			argc=argc-2;
			}
		
			
		}
		int saved_out=-1;
		int saved_err=-1;
		//final redirections applied once
		if(fd_out>=0){
			saved_out=dup(STDOUT_FILENO);
			if(saved_out<0){
				perror("dup");
				close(fd_out);
				close(saved_out);
				return;
			}
		if(dup2(fd_out, STDOUT_FILENO)<0){
			perror("dup2");
			close(fd_out);
			close(saved_out);
			return;
		}
		close(fd_out);
		
		}
		if(fd_err>=0){
			saved_err = dup(STDERR_FILENO);
			if(saved_err<0){
				perror("dup");
				close(fd_err);
				close(fd_out);
				return;
			}
			if(dup2(fd_err, STDERR_FILENO)<0){
				perror("dup2");
				close(fd_err);
				close(saved_err);
				return;
			}
			close(fd_err);
		}

		for(int i=0; i<argc; i++){
			if(i>0) {putchar(' ');}
			
			fputs(argv[i], stdout);
			
		}
		putchar('\n');
		if(saved_out>=0){
			if(dup2(saved_out, STDOUT_FILENO)<0){
				perror("dup2 restore");
				close(saved_out);
				return;
			}
			close(saved_out);
		}
		if(saved_err>=0){
			if(dup2(saved_err, STDERR_FILENO)<0){
				perror("dup2 restore");
				close(saved_err);
				return;
			}
			close(saved_err);
			
		}
		
		
	}
	free(arg_copy);
}
*/
int execute_type_deprecated(char input[]){
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
	return 0;
}

int execute_type(char input []){
	int status=0;
	char *arg=remove_command_and_get_string(input);
	if(check_valid_command(arg)){
		if(execute_type_deprecated(input)==0){
		return 0;
		}
	}
	int flag=0; //flag to indicate if we found the command or not.
	
	if(strchr(arg,'/')!=NULL){
		if(access(arg,X_OK)==0){
			printf("%s\n", arg);
			status=0;
		}
		else{
			printf("%s: not found\n",arg);
			status=1;
		}
		return status;
		
	}
else{
char candidate_path[PATH_MAX];
char *path_env= getenv("PATH");
if(path_env== NULL||path_env[0]=='\0'){
if(build_path("", arg, candidate_path, sizeof(candidate_path)) !=NULL && access(candidate_path, X_OK)==0){
	printf("%s is %s\n",arg, candidate_path);
	status= 0;
}
else{
	printf("%s: not found\n",arg);
	status= 1;
}
return status;
}

char  *path_copy= strdup(path_env);
if(!path_copy){
	perror("strdup");
	//exit(1);
	_exit(1);
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
	status=0;
}
else{
	printf("%s: not found\n",arg);
	status=1;
}
}
return status;
}

/*int get_arguments(char* inputs_copy, char** argument,int max_args){
	int argc = 0;
	char *p= inputs_copy;
	while(*p && isspace((unsigned char)*p)) p++; // skips leading spaces
	while(*p !='\0' && argc < max_args){
		if(*p=='\'' && argc<max_args){
		
			p++;
			char *start =p;
			
			while(*p && *p!='\'') p++;
			if(*p == '\0'){
				argument[argc++]= start;
				break;
			}
			else{
				*p='\0';
				argument[argc] = start;
				if(*argument[argc]=='\''){
					*argument[argc]=' ';
				}
				argc++;
				
				p++;
			}
			
		}
		else{
			//unquoted token

			while(*p && isspace((unsigned char) *p)&& *p!='\'') p++; //skip spaces
			if(*p && *p!='\''){
			argument[argc++]=p;
			}
		while(*p &&*p!='\'' && !isspace((unsigned char)*p)) p++; // advance to the end of token
		if(*p=='\0') break;
		if(*p!='\''){
		*p='\0';
		p++;
		}
		}
		while(*p && isspace((unsigned char)*p)) p++; //skip spaces till the next token
		
	
	}
	argument[argc]=NULL;
	return argc;

}*/
int get_arguments(char *input, char **argv, int buf_size){
	int argc = 0;
	const char *p=input;
	// build tokens into a temporary buffer
	while(*p){
		//skip leading whitespaces
		while(*p && isspace((unsigned char)*p)) p++;
		if(*p=='\0') break;
		// build one token
		char buf[4096];
		int blen=0;
		while(*p && !isspace((unsigned char)*p)){
			if(*p=='\"'){
				p++; //skip opening
				while(*p && *p!='\"'){
					if(*p!='\\'){ //check for escaped characters
					if(blen+1 < (int)sizeof(buf)){
						buf[blen++]=*p;
					}
					p++; //keep moving until end of double quotes
					}
					else{ //store the escaped charatcer
					      
					/*	p++;// skip the escape character
						int advance=handle_escaped_characters(p,buf,&blen,4096);
						for(int i=0; i<advance; i++){
							p++;
						}
						continue; */
					if(*p && *p=='\\'){
					if(*(p+1) && (*(p+1)=='\"' || *(p+1)=='$' || *(p+1)=='`'|| *(p+1)=='\\')){
							p++;
						}
						if(blen+1 < (int)sizeof(buf)){
							buf[blen++]=*p;
						}
						*p++;
					}
					continue;
					}

				}
				if(*p && *p=='\"'){
				p++; // skip closing
				}
			}

			else if(*p=='\''){ //quoted segment
				p++;//skip opening
				while(*p && *p!='\''){
				//if(*p!='\\'){
				if(blen + 1<(int) sizeof(buf)){
					buf[blen++]=*p;
				}
				p++;
				//}
				//else{
				//	p++;
				//	continue;
				//}
				}
				if(*p=='\''){
					p++; //skip closing
					     //unlike before token continues forming
				}
			} else{
				//unquoted character
				if(*p && *p!='\\'){
				if(blen +1 <(int)sizeof(buf)){
					buf[blen++]= *p;
				}
				p++;
				}
				else{ /*
					p++;
					int advance=handle_escaped_characters(p,buf,&blen,4096);
					for(int i=0; i<advance; i++){
						p++;
					}
					continue; */
					if(*p && *p=='\\'){
							p++;
						if(blen +1 <(int)sizeof(buf)){
							buf[blen++]=*p;
						}
						p++;
						
					}
					continue;
				}
			}
			// continue until whitespace or end

		}
		buf[blen]= '\0';
		// store token
		if(argc < buf_size){
			argv[argc] = strdup(buf);
			if(!argv[argc]){
				perror("strdup");
				return argc;
			}
			argc++;
		}
		else{
		break;
		}
		while(*p && isspace((unsigned char)*p)) p++;
	}
	argv[argc]= NULL;
	return argc;
}
/*
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
			
			 const char *args;
			 int fd_out=-1;
			 int newfd=-1;
			 int fd_err=-1;
			 for(int i=0;argument[i]!=NULL; i++){
				 args=argument[i];
				 if(strcmp(args,"1>")==0 || strcmp(args,">>")==0 || strcmp(args,">")==0 || strcmp(args,">>")==0){
					 if(argument[i+1]==NULL){
						 fprintf(stderr, "%s: syntax error: expected filename after %s\n", argument[0],args);
						 _exit(1);
					 }
					 const char *file_name=argument[i+1];
					 newfd=get_file(args,file_name);

				       if(newfd<0){
				       perror("file");
				       _exit(1);
				       }
			       if(fd_out>=0) close(fd_out); // close previously open  file descriptor, only the last one wins
		               fd_out=newfd;
		                int j=i; //remove the tokens at index i and i+1
		 while(argument[j+2]!=NULL){
			 argument[j]=argument[j+2];
			 j++;
		 }
                  argument[j]=NULL;
                  argument[j+1]=NULL;//just in case
                  i--; //recheck this index		  
				 }
				 else if(strcmp(args,"2>")==0 || strcmp(args,"2>>")==0){
				 if(argument[i+1]==NULL){
					 fprintf(stderr,"%s: syntax error: expected filename after %s\n", argument[0], args);
					 _exit(1);
				 }
				 const char *file_name=argument[i+1];
				 newfd=get_file(args,file_name);
				 if(newfd<0){
					 perror("file");
					 _exit(1);
				 }
				 if(fd_err>=0) close (fd_err);
				 fd_err=newfd;
				 int j=i;
				 while(argument[j+2]!=NULL){
				 argument[j]=argument[j+2];
				 j++;
				 }
				 argument[j]=NULL;
				 argument[j+1]=NULL;
				 i--;
				 }	 
			 }
			 // final redirections
			 if(fd_out >=0){
				 if(dup2(fd_out, STDOUT_FILENO)<0){
					 perror("dup2 stdout");
					 close(fd_out);
					 _exit(1);
				 }
				 close(fd_out);
			 }
			 if(fd_err >=0){
				 if(dup2(fd_err, STDERR_FILENO)<0){
					 perror("dup2 stderr");
					 close(fd_err);
					 _exit(1);
				 }
				 close(fd_err);
			 }
			 
			if( handle_redirections(argument)!=0){
				_exit(1);
			}
			execvp(argument[0], argument);
			fprintf(stderr, "%s: command not found\n", argument[0]);
			_exit(127);
			
		}
		else{
			int status;
			if(waitpid(pid, &status,0)<0){
				perror("waitpid");
			}
			

			free(inputs_copy);
		} 
			
}
*/
char* remove_command_and_get_string(char input[] ){
	int i=skip_leading_spaces(input);
	while(input[i] != '\0' && !isspace((unsigned char)input[i])){ //moving through the command
	i++;
	}
	while(input[i] != '\0' && isspace((unsigned char)input[i])){ // remove spaces after command
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

int execute_command(char *command, char input[]){
	char *argv[MAX];
	char *inputs_copy=strdup(input);
	if(!inputs_copy){
		perror("strdup");
		return 1;
	}
	int argc=get_arguments(inputs_copy,argv,MAX);
	//store_history(inputs_copy);
	if(strcmp(command,"echo")==0){
		return execute_echo(inputs_copy);
	}
	else if(strcmp(command,"cd")==0){
		return execute_cd(inputs_copy);
	}
	else if(strcmp(command,"pwd")==0){
		if(handle_redirections(argv)<0) _exit(1);
		return execute_pwd(inputs_copy);
	}
	else if(strcmp(command,"type")==0){
		if(handle_redirections(argv)<0) _exit(1);
		return execute_type(inputs_copy);
	}
	else if(strcmp(command,"history")==0){
		if(handle_redirections(argv)<0) _exit(1);
		return execute_history(input);
	}
	else {
		if(in_pipeline_child){
			if(argc==0) _exit(0);
			if(handle_redirections(argv)<0) _exit(1);
			execvp(argv[0], argv);
			fprintf(stderr,"%s: command not found\n",argv[0]);
			free(inputs_copy);
			_exit(127);
		}
		else{
			pid_t pid=fork();
			if(pid<0){
				perror("fork");
				free(inputs_copy);
				return 1;
			}
			if(pid==0){
			argc=get_arguments(inputs_copy,argv,MAX);
			if(argc==0) _exit(0);
			if(handle_redirections(argv)<0) _exit(1);
			execvp(argv[0],argv);
			fprintf(stderr,"%s: command not found\n",argv[0]);
			free(inputs_copy);
			_exit(127);
			}
			else{
				int wstatus;
				if(waitpid(pid, &wstatus, 0)<0){
					perror("waitpid");
					return 1;
				}
				free(inputs_copy);
				if(WIFEXITED(wstatus)) return WEXITSTATUS(wstatus);
				else return 1;
			}
		}
	}
}
/*
void execute_command(char *command,char input[]){
	if(strcmp(command,"echo")==0){
		execute_echo(input);
	}
	else if(strcmp(command, "cd")==0){
		execute_cd(input);
	}
	else if(strcmp(command, "type")==0){
		execute_type(input);
	}
	else if(strcmp(command, "pwd")==0){
		execute_pwd(input);
	}
	else{
		execute_c(input);
	}
}*/
char *trim_whitespace( char *s){
	while(*s && isspace((unsigned char)*s)) s++;
	if(*s =='\0') return s;
	char *end = s+ strlen(s)-1;
	while(end > s && isspace((unsigned char)*end)){
		*end = '\0';
		end--;
	}
	return s;
}

void execute_pipeline(char *input){
	char *copy=strdup(input);
	char *stage_cmds[MAX];
	int stage_count=0;
	char *saveptr=NULL;
	char *tok=strtok_r(copy,"|", &saveptr);
	while(tok && stage_count < MAX){
		stage_cmds[stage_count++]=trim_whitespace(tok);
		tok=strtok_r(NULL,"|", &saveptr);
	}
	if(stage_count ==0){free(copy); return;}
	int prev_fd=-1;
	int pipefd[2];
	pid_t pids[MAX];
	for(int i=0; i<stage_count; i++){
		if(i< stage_count-1){
			if(pipe(pipefd)<0){perror("pipe"); return;}
		}
		else {
			pipefd[0]=pipefd[1]=-1;
		}
		pid_t pid=fork();
		if(pid<0){
			perror("fork");
			return;
		}
		if(pid==0){
			in_pipeline_child=true;
			if(prev_fd>=0){
				dup2(prev_fd, STDIN_FILENO);
			}
			if(i < stage_count-1){dup2(pipefd[1],STDOUT_FILENO);}
			if(prev_fd>=0) close(prev_fd);
			if(pipefd[0]>=0) close(pipefd[0]);
			if(pipefd[1]>=0) close(pipefd[1]);
			char cmd_name[MAX];
			if(!retrieve_command(stage_cmds[i], cmd_name,sizeof(cmd_name))){
				_exit(0);
			}
			int status=execute_command(cmd_name,stage_cmds[i]);
			_exit(status);
		}
		pids[i]=pid;
		if(prev_fd >=0) close(prev_fd);
		if(pipefd[1]>=0) close(pipefd[1]);
		prev_fd=(i<stage_count-1) ? pipefd[0]:-1;
	}
	if(prev_fd>=0) close(prev_fd);
	for(int i=0; i<stage_count; i++){
		int wstatus;
		waitpid(pids[i],&wstatus,0);
	}
	free(copy);
}
const char *builtins[]= {"echo", "exit","pwd","cd","history","type", NULL};
char *builtin_generator(const char *text, int state){
	static int idx, len;
	char *name;
	if(state==0){
		idx=0;
		len=strlen(text);
	}
	while((name= (char*)builtins[idx++])!=NULL){
		if(strncmp(name,text,len)==0){
			size_t n=strlen(name);
			char *match = malloc(n+1);
			strcpy(match,name);
			match[n]= '\0';
			return match;
		}
	}
	// No more matches
	return NULL;
}
char **my_completion(const char *text, int start, int end){
	//only attempt to complete at the start of the line
	if(start!=0){
		return NULL; // no other completions.
	}
	//rl_completion_matches will repeatedly call your generator
	return rl_completion_matches(text,builtin_generator);
}
int main(int argc, char *argv[]) {
  // Flush after every printf
  setbuf(stdout, NULL);
  using_history();
  stifle_history(1000);
  histfile=getenv("HISTFILE");
  if(histfile && *histfile){
	  if(read_history(histfile)!=0){
		  //file doesn't exist yet
	  }
  }
  // bind <TAB>
  rl_bind_key('\t', rl_complete);
  // set completion callback
  rl_attempted_completion_function = my_completion;
  char *line;
  while((line=readline("$ "))!=NULL){
	  if(*line){
		  add_history(line);
	  }

  // Wait for user input
 // char input[1024];
 /* if(fgets(input, sizeof(input), stdin) == NULL){
	  printf("\n");
  }*/
  //input[strcspn(input, "\n")]= '\0';
  if(line[0]=='\0'){
	  continue;
  }
  if(strcmp(line, "exit 0")==0){
	  //store_history(trim_whitespace(line));
	  if(histfile && *histfile){
		  if(write_history(histfile)!=0){
			  perror("history -w");
		  }
	  }
	  break;
  }
  if(strchr(line,'|')==NULL){
	  char cmd_name[100];
	  if(!retrieve_command(line,cmd_name,sizeof(cmd_name))) continue;
	  int status=execute_command(cmd_name,trim_whitespace(line));
  }
 
  else{
  execute_pipeline(trim_whitespace(line));
  }
  }
  
  return 0;
}

