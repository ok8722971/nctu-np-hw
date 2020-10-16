#include<bits/stdc++.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
using namespace std;

#define MAX_CMD_CNT 512

void setenv();
void printenv();
vector<string> readPipeToToken();
void analyzeCmd (vector<string> , int, int, int,int pipes_fd[][2]);
vector<string> split( const string&, const string&);
void initEnv();
void redirect( char*[], string );
void execOneCmd(vector<string>);
void execCmdBySeq(vector<string>);

int main(){
    initEnv();
    while(1){
        vector<string> pipeToken = readPipeToToken();
        execCmdBySeq( pipeToken );
    }
    return 0;
}

vector<string> readPipeToToken(){
    string cmd;
    vector<string> cmdToken;
    cout<< "% ";
    getline(cin,cmd);
    cmdToken = split(cmd,"|");

    return cmdToken;
}

void initEnv(){
    char* mypath = new char[1024];
    strcpy(mypath,"PATH=bin:.");
    putenv( mypath );
    //delete[] mypath;
}

vector<string> split(const string& str, const string& delim) {
	vector<string> res;
	if("" == str) return res;
	char * strs = new char[str.length() + 1] ;
	strcpy(strs, str.c_str());
	char * d = new char[delim.length() + 1];
	strcpy(d, delim.c_str());
	char *p = strtok(strs, d);
	while(p) {
		string s = p; 
		res.push_back(s); 
		p = strtok(NULL, d);
	}
	return res;
}

void analyzeCmd ( vector<string> CmdToken, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2]){
    if( CmdToken.empty()){
    }else if( CmdToken.at(0) == "exit"){
        exit(0);
    }else if( CmdToken.at(0) == "setenv"){
        string tmp = CmdToken.at(1) + "=" + CmdToken.at(2);
        //putenv(tmp.c_str()); 
        //can't do this cuz c_str return const and this func require none const char*
        char* tmp_char = new char[tmp.size() + 1 ];
        copy(tmp.begin(),tmp.end(),tmp_char);
        tmp_char[tmp.size()] = '\0';
        putenv(tmp_char);
        //delete[] tmp_char; 
    }else if( CmdToken.at(0) == "printenv"){
        char* tmp_char = new char[CmdToken.at(1).size() + 1 ];
        copy(CmdToken.at(1).begin(),CmdToken.at(1).end(),tmp_char);
        tmp_char[CmdToken.at(1).size()] = '\0';
        cout << getenv(tmp_char)<<endl;
        //delete[] tmp_char;
    }else{
        pid_t pid;
        int status;
        if( ( pid = fork() ) < 0){//fork failed
            cerr << "fork failed" << endl;
        }else if( pid == 0 ) {//child process
            if (fd_in != STDIN_FILENO) { dup2(fd_in, STDIN_FILENO); }
            if (fd_out != STDOUT_FILENO) { dup2(fd_out, STDOUT_FILENO); }

            for (int P = 0; P < pipes_count; P++){
                close(pipes_fd[P][0]);
                close(pipes_fd[P][1]);
            }
            execOneCmd( CmdToken );
        }
    }
}


void redirect( char* args[], string fileName){
    pid_t pid2;
    int pipe_fd[2];
    if ( pipe(pipe_fd) == -1){//create pipe
        fprintf(stderr, "Error: Unable to create pipe.\n");
        exit(EXIT_FAILURE);
    }
    if( ( pid2 = fork() ) < 0 ){//failed
        cerr << "fork failed" << endl;
    }else if( pid2 == 0 ){//child
        close( pipe_fd[0] );//close read end
        dup2( pipe_fd[1], STDOUT_FILENO );
        close( pipe_fd[1] );
        //after these three ins. child output to pipe instead of stdout
        if( execvp( args[0], args) == -1 ){
            cerr << "wrong command" << endl;
        }
    }else{//parent
        close( pipe_fd[1] );//close write end
        int fd_out = open( fileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
        /*if (fd_out < 0){//still dont know why i cant use it
            fprintf(stderr, "Error: Unable to open the output file.\n");
            exit(EXIT_FAILURE);
        }else{
            dup2( fd_out, STDOUT_FILENO );
            close(fd_out);
        }*/
        char buffer[1024];
        sleep(1);
        int len;
        while( (len = read( pipe_fd[0], buffer, 1024)) > 0  ){
            write( fd_out, buffer, len);
        }
        close(fd_out);
    }
    exit(0);    
}

void execOneCmd(vector<string> CmdToken){
    int args_cnt = 1;
    const char* cmd = CmdToken.at(0).c_str();
    if( CmdToken.size() == 1 ){
        if( execlp( cmd, cmd, NULL, NULL ) == -1 ){
            cerr << "wrong command" << endl;
        }
    }else if( CmdToken.at(1) == ">" ){
        char* args[2];
        args[0] = strdup( CmdToken.at(0).c_str() );
        args[1] = NULL;
        redirect( args, CmdToken.at(2) );
    }else{
        bool isRedir = false;
        for( int i = 2; i < CmdToken.size(); i++){
            if( CmdToken.at(i) == ">" ){
                isRedir = true;
                break;
            }
            else{
                args_cnt++;
            }
        }
        char* args[args_cnt+2];
        args[0] = strdup( CmdToken.at(0).c_str() );
        for( int i = 0; i < args_cnt ; i++ ){
            args[i+1] = strdup( CmdToken.at(i+1).c_str() );
        }
        args[args_cnt+1] = NULL;
        if( !isRedir ){//not redirection
            if( execvp( args[0], args) == -1 ){
                cerr << "wrong command" << endl;
            }
        }else{//redirection
            redirect( args, CmdToken.at(args_cnt+2) );
        }

    }
} 

void execCmdBySeq(vector<string> pipeToken){
    //analyzeCmd( cmdToken );
    int C,P;
    int cmd_count = pipeToken.size();
    int pipe_cnt = cmd_count - 1;
    int pipes_fd[MAX_CMD_CNT][2];
    for (P = 0; P < pipe_cnt; ++P){
        if (pipe(pipes_fd[P]) == -1){
            cerr << "can't create pipe" << endl;
        }
    }
    for (C = 0; C < cmd_count; ++C)
    {
        int fd_in = (C == 0) ? (STDIN_FILENO) : (pipes_fd[C - 1][0]);
        int fd_out = (C == cmd_count - 1) ? (STDOUT_FILENO) : (pipes_fd[C][1]);
        
        //create child process
        analyzeCmd( split( pipeToken[C], " "), fd_in, fd_out, pipe_cnt, pipes_fd);
    }
    /*parent don't need pipe*/
    for (P = 0; P < pipe_cnt; P++){
        close(pipes_fd[P][0]);
        close(pipes_fd[P][1]);
    }

    /* wait for all child process  */
    for (C = 0; C < cmd_count; C++){
        int status;
        wait(&status);
    }
}
