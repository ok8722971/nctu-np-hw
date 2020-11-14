#include <bits/stdc++.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iterator>
#include <sstream>
#include <fstream>
#include <arpa/inet.h>
using namespace std;

void printenv();
void analyzeCmd ( vector<string>, int, int, int*, int, vector<int>, int );
vector<string> split( const string&, const string& );
void redirect( char*[], string );
void execOneCmd( vector<string> );
void execCmdBySeq( vector<string> ,int );
vector<int> maintainNP();
int toNum( string );
int toNum2( string );
int isNP( string );
void shell();
void initClientInfo();
int add_client( int, struct sockaddr_in );
void broadcast( string );
void print_welcome( int );

void yell();
void who();
void name();
void tell();


struct numberpipe {
   int cnt;//how many ins left
   int fd;//which pipefd to fdin
};

struct user_info{
    int sock;
    string name;
    vector<numberpipe> np;
    vector<int> pid_list;
    vector<int> fd0_list;
    vector<int> fd1_list;
    string port;
    string ip;
    char* path = (char *)malloc(100);
};

//global variable
struct user_info user[31];
int sockfd,cid;//which client is served
fd_set fds;

vector<string> SplitWithSpace(const string &source){
    stringstream ss(source);
    vector<string> vec( (istream_iterator<string>(ss)), istream_iterator<string>() );
    return vec;
}

int main( int argc, char* argv[]){

    initClientInfo();

    struct sockaddr_in cli_addr, serv_addr;
    socklen_t clilen;
    //create socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            cerr <<"server erro!";
    //set server
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
    //let port can be reused
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
    //bind socket and port
    if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) <0)
            cerr << "bind error";

    listen(sockfd, 30);
    //kill zombie
    signal(SIGCHLD, SIG_IGN);

    int maxfd = sockfd;

    while (1) {
        //initial every time
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        //spec says max client will be 30
        for( int i = 1; i <= 30; i++ ){
            if( user[i].sock != -1 )
                FD_SET( user[i].sock, &fds );
            // update max
            if( user[i].sock > maxfd )
                maxfd = user[i].sock;
        }

        int n = select(maxfd + 1, &fds, NULL, NULL, NULL);

        if( FD_ISSET(sockfd, &fds) ){
            int newsockfd;
            clilen = sizeof(cli_addr);
            newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);

            int n = add_client(newsockfd, cli_addr);
            print_welcome(newsockfd);
            
            string tmp = "*** User '(no name)' entered from " + user[n].ip + ":" + user[n].port +". ***";
            
			broadcast(tmp);
			dup2( user[n].sock, STDOUT_FILENO );
			cout << "% " << flush;
        }
        for(int i = 1; i <= 30; i++){
            if( !FD_ISSET( user[i].sock, &fds) )
                continue;
			cid = i; //user id i is using
			//initial user environment
			putenv(user[cid].path);
			shell();
			if(user[cid].sock !=-1)
        		cout << "% " << flush;
		}

    }
    close(sockfd);
    return 0;
}

void shell(){
    
    dup2( user[cid].sock, 0 );
    dup2( user[cid].sock, 1 );
    dup2( user[cid].sock, 2 );
    
	string cmd;
	getline(cin,cmd);
	vector<string> tmpToken = SplitWithSpace(cmd);
	string tmp;
	for( int i =0 ;i < tmpToken.size() ; i++ ){
		if( i !=  tmpToken.size()-1 ){
			tmp += tmpToken.at(i);
			tmp += " ";
		}else{
			tmp += tmpToken.at(i);
		}
	}
	vector<string> pipeToken = split(tmp,"|");
	int n = -1;
	if( pipeToken.size() > 0){
		if( pipeToken.size() >= 2 ){
			n = isNP( pipeToken.at( pipeToken.size() - 1 ) );
		}
		if( n == -1 ){
			execCmdBySeq( pipeToken,-1 );
		}else{
			execCmdBySeq( pipeToken, n );
		}
	}
}
void initClientInfo(){
    for( int i = 1 ; i <= 30 ; i++ ){
	    user[i].sock = -1;
    }
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
		dup2( sockfd,0 );
		dup2( sockfd,1 );
		dup2( sockfd,2 );
		close(user[cid].sock);
		//FD_CLR(user[cid].sock, &fds);
		string msg = "*** User " + user[cid].name + " left. ***";
		user[cid].sock = -1;
		broadcast(msg);
    }else if( CmdToken.at(0) == "setenv"){
        string tmp = CmdToken.at(1) + "=" + CmdToken.at(2);
        //putenv(tmp.c_str());
        //can't do this cuz c_str return const and this func require none const char*
        char* tmp_char = new char[tmp.size() + 1 ];
        copy(tmp.begin(),tmp.end(),tmp_char);
        tmp_char[tmp.size()] = '\0';
        putenv(tmp_char);
		//save env
		strcpy(user[cid].path, tmp_char);

    }else if( CmdToken.at(0) == "printenv"){
        if( CmdToken.size() > 1 ){
            char* tmp_char = new char[CmdToken.at(1).size() + 1 ];
            copy(CmdToken.at(1).begin(),CmdToken.at(1).end(),tmp_char);
            tmp_char[CmdToken.at(1).size()] = '\0';
            if( getenv(tmp_char) != NULL )
                cout << getenv(tmp_char) << endl;
		}
    }else if( CmdToken.at(0) == "who" ){
		dup2( user[cid].sock, STDOUT_FILENO );
		cout << "<ID>	" << "<nickname>	" << "ip:port	" << "indicate me" << endl;
		for( int i = 1; i <= 30 ; i++ ){
			if( user[i].sock != -1 ){
				if( i == cid ){
					cout << i << "	" << user[i].name << "	" << user[i].ip << ":" << user[i].port	<< "	<-me" << endl;
				}else{
					cout << i << "  " << user[i].name << "  " << user[i].ip << ":" << user[i].port  << endl;
				}	
			}	
		}
	}else if(  CmdToken.at(0) == "tell" ){
		int who = toNum2( CmdToken.at(1) );
		if( user[who].sock != -1 ){
			dup2( user[who].sock, STDOUT_FILENO );
			string msg = "";
			for( int i = 2; i < CmdToken.size(); i++ ){
				msg += CmdToken.at(i);
				if( i != CmdToken.size()-1)
					msg += " ";	
			}
			cout << "*** " << user[cid].name << " told you ***: " << msg << endl;
			dup2( user[cid].sock, STDOUT_FILENO );
		}else{
			cout << "*** Error: user #" << who << " does not exist yet. ***" << endl;	 
		}
	}else if( CmdToken.at(0) == "yell" ){
		string msg = "*** " + user[cid].name +" yelled ***:";
		for( int i = 1; i < CmdToken.size(); i++ ){
			msg += CmdToken.at(i);
			if( i != CmdToken.size()-1)
				msg += " ";
		}
		int temp = user[cid].sock;
		user[cid].sock = -1;
		broadcast( msg );
		user[cid].sock = temp;
		dup2( user[cid].sock, STDOUT_FILENO );		
	}else if( CmdToken.at(0) == "name" ){
		bool flag = true;
		for(int i = 1; i <= 30; i++){
			if( user[i].name == CmdToken.at(1) )
				flag = false;	
		}
		if( flag ){
			user[cid].name = CmdToken.at(1);
			string msg = "*** User from " + user[cid].ip + ":" + user[cid].port + "is named '" + CmdToken.at(1) + "'. ***";
			broadcast( msg );
			dup2( user[cid].sock, STDOUT_FILENO );
		}else{
			cout << "*** User '" << CmdToken.at(1)  <<"' already exists. ***" << endl;	
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
            close(user[cid].fd0_list.at(C-1));
            close(user[cid].fd1_list.at(C-1));
        }
        execOneCmd( CmdToken );
        }else{//parent process
            user[cid].pid_list.push_back(pid);
            if(pipes_fd[0] != 0){
            	user[cid].fd0_list.push_back(pipes_fd[0]);
                user[cid].fd1_list.push_back(pipes_fd[1]);
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
    
	//here may be problem
	user[cid].pid_list.clear();
    user[cid].fd0_list.clear();
    user[cid].fd1_list.clear();
    
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
            user[cid].np.push_back( tmp );

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
            user[cid].np.push_back( tmp );

            CmdToken.erase( CmdToken.begin() + CmdToken.size() - 1 );
        }
        if( C == 0 ){
            analyzeCmd( CmdToken, fd_in, fd_out, &pipes_fd[0], pipeType, tmp2, C);
        }else{
            vector<int> ll;
            analyzeCmd( CmdToken, fd_in, fd_out, &pipes_fd[0], pipeType, ll, C);
        }
        if( C > 0 ){
            close(user[cid].fd0_list.at(C-1));
            close(user[cid].fd1_list.at(C-1));
        }
    }
    if(tt != -1){
        close(tt);
        while( waitpid(-1 , NULL, WNOHANG) > 0 );
    }else{
        if( !user[cid].pid_list.empty() ){
            int pid = user[cid].pid_list.back();
            waitpid(pid, NULL, 0);
        }
        while( waitpid(-1 , NULL, WNOHANG) > 0 );
    }
}

vector<int> maintainNP(){
    vector<int> tmp;
    for( int i = 0; i < user[cid].np.size(); i++ ){
        user[cid].np.at(i).cnt--;
        if( user[cid].np.at(i).cnt == 0){
            tmp.push_back( user[cid].np.at(i).fd );
            user[cid].np.erase( user[cid].np.begin() + i );
            i--;//cuz erase will remove element
        }
    }
    return tmp;
}

//this is for numberpipe
int toNum(string s){
    int num = 0;
    for( int i = 1 ; i < s.size(); i++ ){
        num *= 10;
        num += s[i]-48;//ascii 0 is 48
    }
    return num;
}
//this is for tell
int toNum2( string s ){
	int num = 0;
	for( int i = 0 ; i < s.size(); i++ ){
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

int add_client(int sockfd, struct sockaddr_in address){
    int n = 0;
    for( int i = 1; i <= 30 ; i++){
        if( user[i].sock == -1 ){
            n = i;
			break;
		}
    }
    user[n].sock = sockfd;
    user[n].name = "(no name)";
    user[n].np.clear();
    user[n].pid_list.clear();
    user[n].fd0_list.clear();
    user[n].fd1_list.clear();
    user[n].port = to_string( ntohs(address.sin_port) );
    user[n].ip.assign( inet_ntoa(address.sin_addr) );
    strcpy(user[n].path, "bin:.");

	return n;
}
void broadcast( string  message ){
    for( int i = 1 ; i <= 30 ; i++ ){
        if(user[i].sock != -1){
            dup2(user[i].sock,STDOUT_FILENO);
            cout << message << endl;
        }
    }
}

void print_welcome( int sock ){
    dup2(sock,STDOUT_FILENO);
    cout << "***************************************" << endl;
    cout << "** Welcome to the information server **" << endl;
    cout << "***************************************" << endl;
}
