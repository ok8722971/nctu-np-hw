#include <stdlib.h> 
#include <iostream> 
using namespace std;
int main() { 
 
 
//set new shell environmental variable using putenv

while(1){
string tmp; 
getline(cin,tmp);
char* tmp_char = new char[tmp.size() + 1 ];
copy(tmp.begin(),tmp.end(),tmp_char);
tmp_char[tmp.size()] = '\0';
 
putenv( tmp_char ); 
 
cout << "PATH = " << getenv("PATH") << endl; 
}
return 0; 
 
}
