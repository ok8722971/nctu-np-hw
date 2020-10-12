#include<bits/stdc++.h>
using namespace std;

void setenv();
void printenv();
vector<string> readCmdToToken();
void analyzeCmd (vector<string> CmdToken);
vector<string> split(const string& str,const string& delim);
void initEnv();

int main(){
    //initEnv();
    while(1){
        vector<string> cmdToken = readCmdToToken();
        analyzeCmd( cmdToken );
    }
    return 0;
}

vector<string> readCmdToToken(){
    string cmd;
    vector<string> cmdToken;
    cout<< "% ";
    getline(cin,cmd);
    cmdToken = split(cmd," ");
    /*for(int i = 0; i < cmdToken.size(); i++){
        cout << cmdToken[i] << endl;
    }*/

    return cmdToken;
}


/*void initEnv(){
    char mypath[]="PATH=bin:.";
    putenv( mypath );
    cout << getenv("PATH")<<endl;    
}*/

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


void analyzeCmd (vector<string> CmdToken){
    if( CmdToken.empty()){
    }else if( CmdToken.at(0) == "exit" && CmdToken.size() == 1){
        exit(0);
    }else if( CmdToken.at(0) == "setenv"){
        string tmp = CmdToken.at(1) + "=" + CmdToken.at(2);
        //putenv(tmp.c_str()); 
        //can't do this cuz c_str return const and this func require none const char*
        char* tmp_char = new char[tmp.size() + 1 ];
        copy(tmp.begin(),tmp.end(),tmp_char);
        tmp_char[tmp.size()] = '\0';
        putenv(tmp_char);
        delete[] tmp_char; 
    }else if( CmdToken.at(0) == "printenv"){
        cout << getenv(CmdToken.at(1).c_str())<<endl;
    }else{
        cout << "Unknown command: [" << CmdToken.at(0) << "]." << endl;
    }
    return ;
}

