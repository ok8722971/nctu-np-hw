#include <boost/asio.hpp>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#include <fstream>

boost::asio::io_context io_context;

using boost::asio::ip::tcp;
using namespace std;


struct info{//record request information
	int mode; //1:connect 2:bind
	bool accept;//true = accept, false = reject
	string src_ip;
	string src_port;
	string dst_ip;
	string dst_port;
};

/*global variable*/
struct info ginfo;
/**/

void show_message() {
	cout << "<S_IP>: " << ginfo.src_ip << endl;
	cout << "<S_PORT>: " << ginfo.src_port << endl;
	cout << "<D_IP>: " << ginfo.dst_ip << endl;
	cout << "<D_PORT>: " << ginfo.dst_port << endl;
	if(ginfo.mode == 1)
		cout << "<Command>: CONNECT" << endl;
	else
		cout << "<Command>: BIND" << endl;
	if(ginfo.accept)
		cout << "<Reply>: Accept" <<endl;
	else
		cout << "<Reply>: Reject" << endl;
}
vector<string> split(const string& str, const string& delim) {
	vector<string> res;
	if ("" == str) return res;
	char* strs = new char[str.length() + 1];
	strcpy(strs, str.c_str());
	char* d = new char[delim.length() + 1];
	strcpy(d, delim.c_str());
	char* p = strtok(strs, d);
	while (p) {
		string s = p;
		res.push_back(s);
		p = strtok(NULL, d);
	}
	return res;
}
void firewall() {
	ginfo.accept = false;//false at first
	fstream file;
	char buffer[100];
	file.open("socks.conf", ios::in);
	if (!file) cout << "open file error";
	else {
		do {
			file.getline(buffer, sizeof(buffer));
			vector<string> line = split(buffer, " ");
			if ((line.at(1) == "c" && ginfo.mode == 1) || (line.at(1) == "b" && ginfo.mode == 2)) {
				vector<string> u1 = split(ginfo.dst_ip, ".");
				vector<string> u2 = split(line.at(2), ".");
				bool flag = true;
				for (int i = 0; i < 4; i++) {
					if (u2.at(i) == "*") continue;
					if (u1.at(i) != u2.at(i)) {
						flag = false;
						break;
					}
				}
				if (flag) ginfo.accept = true;
			}
		} while (!file.eof());
		file.close();
	}
}




class session
	: public std::enable_shared_from_this<session> {
public:
	session(tcp::socket socket)
		: socket_(std::move(socket)) {
	}

	void start() {
		do_read();
	}

private:
	void do_read() {
		auto self(shared_from_this());
		socket_.async_read_some(boost::asio::buffer(data_, max_length),
			[this, self](boost::system::error_code ec, std::size_t length) {

			if (!ec) {
				io_context.notify_fork(boost::asio::io_context::fork_prepare);
				int pid = fork();
				if (pid == 0) {
					io_context.notify_fork(boost::asio::io_context::fork_child);
					//set info here
					ginfo.mode = data_[1];
					ginfo.src_ip = socket_.remote_endpoint().address().to_string();
					ginfo.src_port = to_string(socket_.remote_endpoint().port());
					ginfo.dst_ip = to_string(data_[4]) + "." + to_string(data_[5]) + "." + to_string(data_[6]) + "." + to_string(data_[7]);
					ginfo.dst_port = to_string((data_[2] << 8 | data_[3]));
					//socks4a 
					if (to_string(data_[4]) == "0" && to_string(data_[5]) == "0" && to_string(data_[6]) == "0" && to_string(data_[7]) != "0") {
						string h = "";
						for (size_t i = 9; i < sizeof(data_); i++) {
							if ((int)data_[i] == 0 ) break;
							h += data_[i];
							//cout << i << ": " << data_[i];
						}
						
						tcp::resolver resolver(io_context);
						tcp::resolver::query query(h.c_str(), ginfo.dst_port.c_str());
						tcp::resolver::iterator it = resolver.resolve(query);

						while (it != tcp::resolver::iterator()){
							boost::asio::ip::address addr = (it++)->endpoint().address();
							if (addr.is_v6())	continue;
							else {
								ginfo.dst_ip = addr.to_string();
							}
						}
					}
					
					//firewall
					firewall();
					show_message();
					//do_write();
				}
				else {
					io_context.notify_fork(boost::asio::io_context::fork_parent);
					//socket_.close();
				}
				
			}
		});
	}

	void do_write() {
		auto self(shared_from_this());
		boost::asio::async_write(socket_, boost::asio::buffer(status, strlen(status)),
			[this, self](boost::system::error_code ec, std::size_t /*length*/) {
			if (!ec) {
				

				//do_read();
			}
		});
	}

	tcp::socket socket_;
	enum { max_length = 1024 };
	char status[20] = "HTTP/1.1 200 OK\n";
	unsigned char data_[max_length];
};

class server {
public:
	server(short port)
		: acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
		do_accept();
	}

private:
	void do_accept() {
		acceptor_.async_accept(
			[this](boost::system::error_code ec, tcp::socket socket) {
			if (!ec) {
				// create an object(session) and call its start() fuction
				std::make_shared<session>(std::move(socket))->start();
			}

			do_accept();
		});
	}

	tcp::acceptor acceptor_;
};

int main(int argc, char* argv[]) {
	try {
		if (argc != 2) {
			std::cerr << "Usage: socks_server <port>\n";
			return 1;
		}

		server s(std::atoi(argv[1]));

		io_context.run();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}
