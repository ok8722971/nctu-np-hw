#include<bits/stdc++.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
using namespace std;

#define MAX_CMD_CNT 2000

void setenv();
void printenv();
//vector<string> readPipeToToken();
void analyzeCmd ( vector<string>, int, int, int*, int, vector<int>, int );
vector<string> split( const string&, const string& );
void initEnv();
void redirect( char*[], string );
void execOneCmd( vector<string> );
void execCmdBySeq( vector<string> ,int );
vector<int> maintainNP();
int toNum( string );
int isNP( string );
void shell();

struct numberpipe {
   int cnt;//how many ins left
   int fd;//which pipefd to fdin
};

vector<numberpipe> np;
vector<int> pid_list;
vector<int> fd0_list;
vector<int> fd1_list;
int newsockfd = 0;
int main( int argc, char* argv[]){
    /*initEnv();
    while(1){
        vector<string> pipeToken = readPipeToToken();
        int n = -1;
        if( pipeToken.size() == 0){ continue; }
        if( pipeToken.size() >= 2 ){
            n = isNP( pipeToken.at( pipeToken.size() - 1 ) );			
        }
        if( n == -1 ){
            execCmdBySeq( pipeToken,-1 );
        }else{
            execCmdBySeq( pipeToken, n );
        }
    }*/
	int sockfd, newsockfd, childpid;
	struct sockaddr_in cli_addr, serv_addr;
	socklen_t clilen;
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		cerr <<"server erro!";
	bzero((char*)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));
	if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) <0) // bind sock & port .
		cerr << "bind error";
	listen(sockfd, 1);
	signal(SIGCHLD, SIG_IGN);
    while (1) {
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
		if (newsockfd < 0) {
			cerr << "Error: Accept failed";
		}
		cout << "New client's sockfd = " << newsockfd << endl;
		if((childpid = fork()) < 0)
			cerr << "Error: fork() failed";
		else if(childpid == 0)
			shell();
		else{
			cout << "Helper's pid = " << childpid << endl;
		    close(newsockfd);
		}
	}
	return 0;
}
void shell(){
	initEnv();
	while(1){
		char buffer[20000];	
		memset(&buffer, '\0', sizeof(buffer));
		write(newsockfd, "% ", strlen("% "));
		read(newsockfd, buffer, sizeof(buffer));
		string cmd(buffer);
		vector<string> pipeToken = split(cmd,"|");
		int n = -1;
		if( pipeToken.size() == 0){ continue; }
		if( pipeToken.size() >= 2 ){
			n = isNP( pipeToken.at( pipeToken.size() - 1 ) );
		}
		dup2( newsockfd, 1 );
		dup2( newsockfd, 2 );
		if( n == -1 ){
			execCmdBySeq( pipeToken,-1 );
		}else{
		    execCmdBySeq( pipeToken, n );
		}
	}	
} 

/*vector<string> readPipeToToken(){
    string cmd;
    vector<string> cmdToken;
    cout<< "% ";
    getline(cin,cmd);
    cmdToken = split(cmd,"|");

    return cmdToken;
}*/

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

void analyzeCmd ( vector<string> CmdToken, int fd_in, int fd_out, int* pipes_fd, int pipeType, vector<int> tmp, int C){
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
    }else if( CmdToken.at(0) == "printenv"){
	if( CmdToken.size() > 1 ){
	    char* tmp_char = new char[CmdToken.at(1).size() + 1 ];
	    copy(CmdToken.at(1).begin(),CmdToken.at(1).end(),tmp_char);
	    tmp_char[CmdToken.at(1).size()] = '\0';
	    if( getenv(tmp_char) != NULL )
	        cout << getenv(tmp_char)<<endl;
		}

    }else{
        pid_t pid;
        while( ( pid = fork() ) < 0){//fork failed
            waitpid(-1, NULL, 0);
			//cerr << "fork failed" << endl;
        }
		if( pid == 0 ) {//child process
            if( fd_in != STDIN_FILENO ) { dup2(fd_in, STDIN_FILENO); }
            if( fd_out != STDOUT_FILENO ) { dup2(fd_out, STDOUT_FILENO); }
            
            if( tmp.size() > 0 ){
                char content[100000] = {'\0'};
                char buffer[2] = {'\0'};
                int len;

                for(int i = 0; i < tmp.size(); i++){
                    while( (len = read( tmp.at(i), buffer, 1)) > 0  ){
                        strcat( content, buffer );
                    }
                    close( tmp.at(i) );
                }
                int np_pipe[2];
                if (pipe(np_pipe) == -1){
                    cerr << "can't create pipe" << endl;
                }
                write( np_pipe[1], content, strlen(content) );
                close( np_pipe[1] );
                dup2( np_pipe[0], STDIN_FILENO );
            }
            if( pipeType == 1 ){
                dup2(fd_out, STDERR_FILENO);
            }   
			if(pipes_fd[0] != 0){
            	close(pipes_fd[0]);
				close(pipes_fd[1]);
            }
			if( C > 0 ){
				close(fd0_list.at(C-1));
				close(fd1_list.at(C-1));	
			}
			execOneCmd( CmdToken );
        }else{//parent process
			pid_list.push_back(pid);
			if(pipes_fd[0] != 0){
				fd0_list.push_back(pipes_fd[0]);
				fd1_list.push_back(pipes_fd[1]);
			}
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
    while( ( pid2 = fork() ) < 0 ){//failed
        waitpid(-1, NULL, 0);
		//cerr << "fork failed" << endl;
    }
	if( pid2 == 0 ){//child
        close( pipe_fd[0] );//close read end
        dup2( pipe_fd[1], STDOUT_FILENO );
        close( pipe_fd[1] );
        //after these three ins. child output to pipe instead of stdout
        if( execvp( args[0], args) == -1 ){
            cerr << "Unknown command: [" << args[0] << "]." <<endl;
            exit(0);
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
                cerr << "Unknown command: [" << args[0] <<"]." << endl;
                exit(0);
            }
        }else{//redirection
            redirect( args, CmdToken.at(args_cnt+2) );
        }

    }
} 

void execCmdBySeq(vector<string> pipeToken , int ori_np ){
    int tt = -1;    
    vector<int> tmp2 = maintainNP();
    pid_list.clear();
	fd0_list.clear();
	fd1_list.clear();
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
	int last_ins_fd = -1;//last instruction's fd
	int pipes_fd[2];
    for (int C = 0; C < cmd_count; C++)
    {
		pipes_fd[0] = 0;
		pipes_fd[1] = 0;
        if( C != cmd_count-1 ){
			if (pipe(pipes_fd) == -1){
					cerr << "can't create pipe" << endl;
			}
		}
		int fd_in = (C == 0) ? (STDIN_FILENO) : (last_ins_fd);
		int fd_out = (C == cmd_count - 1) ? (STDOUT_FILENO) : (pipes_fd[1]);
		last_ins_fd = pipes_fd[0];
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
            tmp.cnt = num ;
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
            tmp.cnt = num;
            tmp.fd = np_pipes_fd[0];
            np.push_back( tmp );

            CmdToken.erase( CmdToken.begin() + CmdToken.size() - 1 );
        }
        if( C == 0 ){
           analyzeCmd( CmdToken, fd_in, fd_out, &pipes_fd[0], pipeType, tmp2, C);
        }else{
           vector<int> ll;
           analyzeCmd( CmdToken, fd_in, fd_out, &pipes_fd[0], pipeType, ll, C); 
        }
        if( C > 0 ){
			close(fd0_list.at(C-1));
			close(fd1_list.at(C-1));
		}
    }
    if(tt != -1){ 
		close(tt);
		while( waitpid(-1 , NULL, WNOHANG) > 0 );
    }else{
		if( !pid_list.empty() ){
			int pid = pid_list.back();
			waitpid(pid, NULL, 0);
		}
		while( waitpid(-1 , NULL, WNOHANG) > 0 );	
	}
}

vector<int> maintainNP(){
    vector<int> tmp;
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
