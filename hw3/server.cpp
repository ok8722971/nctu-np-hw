#include <boost/asio.hpp>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

char exec[100];
boost::asio::io_context io_context;

using boost::asio::ip::tcp;


using namespace std;
vector<string> split( const string &str, const string &delim) {
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


class session
	: public std::enable_shared_from_this<session>{
public:
	session(tcp::socket socket)
		: socket_(std::move(socket)){
	}

	void start(){
    	do_read();
  	}

private:
	void do_read(){
		auto self(shared_from_this());
    	socket_.async_read_some(boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length){			
						
			if (!ec){
				//read data_ 
				string tmp2;
				tmp2.assign(data_);
				vector<string> tmp = split(tmp2, " ");
				//processing str
				strcpy(REQUEST_METHOD, tmp.at(0).c_str());
				strcpy(REQUEST_URI, tmp.at(1).c_str());
				strcpy(SERVER_PROTOCOL, tmp.at(2).c_str());
				strcpy(HTTP_HOST, tmp.at(4).c_str());
				strcpy(SERVER_ADDR, socket_.local_endpoint().address().to_string().c_str());
				snprintf(SERVER_PORT, sizeof(SERVER_PORT), "%d", socket_.local_endpoint().port());
				strcpy(REMOTE_ADDR, socket_.remote_endpoint().address().to_string().c_str());
				snprintf(REMOTE_PORT, sizeof(REMOTE_PORT), "%d", socket_.remote_endpoint().port());
				//setup QUERY_STRING here
				//find and substr
				size_t found = tmp.at(1).find('?');
				if (found != std::string::npos) {
					strcpy(QUERY_STRING, tmp.at(1).substr(found + 1).c_str());
					//setup execlp path
					string tmp3 = "." + tmp.at(1).substr(0, found);
					strcpy(exec, tmp3.c_str());
				}
				else {
					string tmp3 = "." + tmp.at(1);
					strcpy(exec, tmp3.c_str());
				}
				//setup env		
				setenv("REQUEST_METHOD", REQUEST_METHOD, 1);
				setenv("REQUEST_URI", REQUEST_URI, 1);
				setenv("SERVER_PROTOCOL", SERVER_PROTOCOL, 1);
				setenv("HTTP_HOST", HTTP_HOST, 1);
				setenv("SERVER_ADDR", SERVER_ADDR, 1);
				setenv("SERVER_PORT", SERVER_PORT, 1);
				setenv("REMOTE_ADDR", REMOTE_ADDR, 1);
				setenv("REMOTE_PORT", REMOTE_PORT, 1);
				setenv("QUERY_STRING", QUERY_STRING, 1);

				do_write(length);
			}

			
        });
  	}

	void do_write(std::size_t length){
		auto self(shared_from_this());
		boost::asio::async_write(socket_, boost::asio::buffer(status, strlen(status)),
		[this, self](boost::system::error_code ec, std::size_t /*length*/){
			if (!ec){
				io_context.notify_fork(boost::asio::io_context::fork_prepare);
				int pid = fork();
				if( pid == 0 ){
					io_context.notify_fork(boost::asio::io_context::fork_parent);
                	int sock = socket_.native_handle();
                	dup2(sock, STDIN_FILENO);
                	dup2(sock, STDOUT_FILENO);
                	dup2(sock, STDERR_FILENO);
					if(execlp(exec, exec, NULL) < 0) {
	                	cerr << "execlp failed" << endl;
	            	}
				}else{
					io_context.notify_fork(boost::asio::io_context::fork_child);
					socket_.close();
				}
				
				do_read();
			}
		});
	}

	tcp::socket socket_;
	enum { max_length = 1024 };
	char status[20] = "HTTP/1.1 200 OK\n";
	char data_[max_length];
	char REQUEST_METHOD[5];
	char REQUEST_URI[500];
	char QUERY_STRING[500];
	char SERVER_PROTOCOL[10];
	char HTTP_HOST[100];
	char SERVER_ADDR[50];
	char SERVER_PORT[50];
	char REMOTE_ADDR[50];
	char REMOTE_PORT[50];
};

class server{
public:
	server(short port)
		: acceptor_(io_context, tcp::endpoint(tcp::v4(), port)){
    		do_accept();
	}

private:
	void do_accept(){
    	acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket){
        	if (!ec){
				// create an object(session) and call its start() fuction
            	std::make_shared<session>(std::move(socket))->start();
        	}

        	do_accept();
        });
	}

	tcp::acceptor acceptor_;
};

int main(int argc, char* argv[]){
	try{
		if (argc != 2){
			std::cerr << "Usage: async_tcp_echo_server <port>\n";
      		return 1;
    	}
		
    	server s(std::atoi(argv[1]));

    	io_context.run();
  	}
  	catch (std::exception& e){
    	std::cerr << "Exception: " << e.what() << "\n";
  	}

  	return 0;
}
