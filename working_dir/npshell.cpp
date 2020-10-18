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
void analyzeCmd ( vector<string>, int, int, int, int pipes_fd[][2], int );
vector<string> split( const string&, const string& );
void initEnv();
void redirect( char*[], string );
void execOneCmd( vector<string> );
void execCmdBySeq( vector<string> ,int );
vector<int> maintainNP();
int toNum( string );
int isNP( string );


struct numberpipe {
   int cnt;//how many ins left
   int fd;//which pipefd to fdin
};
vector<numberpipe> np;

int main(){
    initEnv();
    while(1){
        vector<string> pipeToken = readPipeToToken();
        int n = isNP( pipeToken.at( pipeToken.size() - 1 ) );
        if( n == -1 ){
            execCmdBySeq( pipeToken,-1 );
        }else{
            execCmdBySeq( pipeToken, n );
        }
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

vector<string> split( const string& str, const string& delim) {
	vector<string> res;
	if("" == str) return res;
	char* strs = new char[str.length() + 1] ;
	strcpy(strs, str.c_str());
	char* d = new char[delim.length() + 1];
	strcpy(d, delim.c_str());
	char*p = strtok(strs, d);
	while(p){
		string s = p; 
		res.push_back(s); 
		p = strtok(NULL, d);
	}
	return res;
}

void analyzeCmd ( vector<string> CmdToken, int fd_in, int fd_out, int pipes_count, int pipes_fd[][2], int pipeType){
    if( CmdToken.empty()){
    }else if( CmdToken.at(0) == "exit"){
        exit(0);
    }else if( CmdToken.at(0) == "setenv"){
        maintainNP();
        string tmp = CmdToken.at(1) + "=" + CmdToken.at(2);
        //putenv(tmp.c_str()); 
        //can't do this cuz c_str return const and this func require none const char*
        char* tmp_char = new char[tmp.size() + 1 ];
        copy(tmp.begin(),tmp.end(),tmp_char);
        tmp_char[tmp.size()] = '\0';
        putenv(tmp_char);
        //delete[] tmp_char; 
    }else if( CmdToken.at(0) == "printenv"){
        maintainNP();
        char* tmp_char = new char[CmdToken.at(1).size() + 1 ];
        copy(CmdToken.at(1).begin(),CmdToken.at(1).end(),tmp_char);
        tmp_char[CmdToken.at(1).size()] = '\0';
        cout << getenv(tmp_char)<<endl;
        //delete[] tmp_char;
    }else{
        vector<int> tmp = maintainNP();
        pid_t pid;
        if( ( pid = fork() ) < 0){//fork failed
            cerr << "fork failed" << endl;
        }else if( pid == 0 ) {//child process
            if( fd_in != STDIN_FILENO ) { dup2(fd_in, STDIN_FILENO); }
            if( fd_out != STDOUT_FILENO ) { dup2(fd_out, STDOUT_FILENO); close(fd_out); }
            
            if( tmp.size() > 0 ){
                char content[2048];
                char buffer[1024];
                int len;
                while( (len = read( tmp.at(0), buffer, 1024)) > 0  ){
                        strcat( content, buffer );
                }
                for(int i = 1; i < tmp.size(); i++){
                    while( (len = read( tmp.at(i), buffer, 1024)) > 0  ){
                        strcat( content, buffer );
                    }
                    close( tmp.at(i) );
                }
                int np_pipe[2];
                if (pipe(np_pipe) == -1){
                    cerr << "can't create pipe" << endl;
                }
                write( np_pipe[1],content, strlen(content) );
                close(np_pipe[1]);
                dup2( np_pipe[0], STDIN_FILENO );
            }
            if( pipeType ){
                dup2(fd_out, STDERR_FILENO);
                close(fd_out);
            }   
            
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
        
        char buffer[1024];
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
            cerr << "Unknown command: [" << cmd <<"]." << endl;
            exit(0);
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

void execCmdBySeq(vector<string> pipeToken , int ori_np ){
    int tt = -1;    
    
    //if cmd is "ls |2"
    //the pipeToken will be pipeToken[0] = ls, pipeToken[1] = 2
    //so we need to change pipeToken[0] -> ls |2
    if( ori_np != -1){
        string s;
        s = "|" + to_string(ori_np);        
        pipeToken.at( pipeToken.size() - 2 ) += s;
        pipeToken.erase( pipeToken.begin() + pipeToken.size() );
    }
    int cmd_count = pipeToken.size();
    int pipe_cnt = cmd_count - 1;
    int pipes_fd[MAX_CMD_CNT][2];
    for (int P = 0; P < pipe_cnt; P++){
        if (pipe(pipes_fd[P]) == -1){
            cerr << "can't create pipe" << endl;
        }
    }
    for (int C = 0; C < cmd_count; C++)
    {
        int fd_in = (C == 0) ? (STDIN_FILENO) : (pipes_fd[C - 1][0]);
        int fd_out = (C == cmd_count - 1) ? (STDOUT_FILENO) : (pipes_fd[C][1]);
        int pipeType = 0;

        vector<string> CmdToken = split( pipeToken[C], " ");
        //do pipe analyzing here
        //ex: ls !2 >> we need to erase!2 and create a pipe to save data
        if( CmdToken.at( CmdToken.size()-1 )[0] == '!' ){
            int np_pipes_fd[2];
            if (pipe(np_pipes_fd) == -1){
                cerr << "can't create pipe" << endl;
            }
            fd_out = np_pipes_fd[1];
            tt = fd_out;//when fork done parent need to close it
            pipeType = 1;// means it is ! type

            //this is child so it won't be record to parent
            //record np to vector
            int num = toNum( CmdToken.at( CmdToken.size()-1 ) );
            struct numberpipe tmp;
            tmp.cnt = num + 1;
            tmp.fd = np_pipes_fd[0];
            np.push_back( tmp );

            CmdToken.erase( CmdToken.begin() + CmdToken.size() - 1 );
       }else if( CmdToken.at( CmdToken.size()-1 )[0] == '|' ){
            int np_pipes_fd[2];
            if (pipe(np_pipes_fd) == -1){
                cerr << "can't create pipe" << endl;
            }
            fd_out = np_pipes_fd[1];
            tt = fd_out;//when fork done parent need to close it

            //this is child so it won't be record to parent
            //record np to vector
            int num = toNum( CmdToken.at( CmdToken.size()-1 ) );
            struct numberpipe tmp;
            tmp.cnt = num + 1;
            tmp.fd = np_pipes_fd[0];
            np.push_back( tmp );

            CmdToken.erase( CmdToken.begin() + CmdToken.size() - 1 );
       }
       analyzeCmd( CmdToken, fd_in, fd_out, pipe_cnt, pipes_fd , pipeType);
       
    }
    //parent don't need pipe
    for (int P = 0; P < pipe_cnt; P++){
        close(pipes_fd[P][0]);
        close(pipes_fd[P][1]);
    }
    if(tt != -1){ close(tt); }

    // wait for all child process  
    for (int C = 0; C < cmd_count; C++){
        int status;
        wait(&status);
    }
}

vector<int> maintainNP(){
    vector<int> tmp;
    /*for( int i = 0; i < np.size(); i++){
        cout << "i=" << i << ",i.cnt= " << np.at(i).cnt << " i.fd= " << np.at(i).fd << endl;   
    }*/
    for( int i = 0; i < np.size(); i++ ){
        np.at(i).cnt--;
        if( np.at(i).cnt == 0){
            tmp.push_back( np.at(i).fd );
            np.erase( np.begin() + i );
            i--;//cuz erase will remove element
        }   
    }
    return tmp;    
}

int toNum(string s){
    int num = 0;    
    for( int i = 1 ; i < s.size(); i++ ){
        num *= 10;
        num += s[i]-48;//ascii 0 is 48
    }
    return num;    
}

int isNP(string s){
    for(int i = 0; i < s.size(); i++){
        if ( !isdigit(s[i]) ){
            return -1;    
        }    
    }
    int ans = 0;
    for( int i = 0 ; i < s.size(); i++ ){
        ans*= 10;
        ans += s[i]-48;//ascii 0 is 48
    }
    return ans;
}
