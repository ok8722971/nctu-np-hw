#include <stdlib.h> 
#include <iostream> 
 
int main() { 
 
// get and print shell environmental variable home 
std::cout << "ORI PATH = " << getenv("PATH") << std::endl;  
 
//set new shell environmental variable using putenv 
char mypath[]="PATH=bin:."; 
putenv( mypath ); 
 
std::cout << "AC PATH = " << getenv("PATH") << std::endl; 
 
return 0; 
 
}
