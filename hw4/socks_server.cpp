#include <bits/stdc++.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <fstream>
#include <time.h>  

boost::asio::io_context io_context;
using std::placeholders::_1;
using std::placeholders::_2;
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
tcp::socket *csock;
int gport;//for bind mode
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
void do_nothing(const boost::system::error_code& error) {}

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

class client{
public:
	client(boost::asio::io_context& io_context)
		:socket_(io_context) {
	}

	void start() {
		tcp::resolver r(io_context);
		tcp::resolver::results_type endpoints = r.resolve(ginfo.dst_ip.c_str(), ginfo.dst_port.c_str());
		endpoints_ = endpoints;
		try {
			start_connect(endpoints_.begin());
		}
		catch (const exception& e) {
			cout << e.what() << endl;
		}
	}
private:
	void start_connect(tcp::resolver::results_type::iterator endpoint_iter) {
		if (endpoint_iter != endpoints_.end()) {
			socket_.async_connect(endpoint_iter->endpoint(),
				std::bind(&client::handle_connect, this, _1, endpoint_iter));
		}
	}
	void handle_connect(const boost::system::error_code& error,
		tcp::resolver::results_type::iterator endpoint_iter) {

		if (!socket_.is_open()) {
			std::cout << "Connect timed out<br>";
			start_connect(++endpoint_iter);
		}

		else if (error) {
			std::cout << "Connect error: " << error.message() << "\n";

			socket_.close();

			start_connect(++endpoint_iter);
		}

		else {
			start_read();
			start_read2();
		}
	}
	void start_read(){
		memset(data_, 0, sizeof(data_));
		socket_.async_read_some(
			boost::asio::buffer(data_, max_length),
			[this](boost::system::error_code ec, std::size_t length) {
			if (!ec) {
				do_write(length);
			}
			else {
				sd = true;
				if (cd) {
					(*csock).close();
					socket_.close();
				}
				//cout << "do_read ec:" << ec << endl;
			}
		});
	}
	void do_write(std::size_t length){
		boost::asio::async_write(*csock, boost::asio::buffer(data_, length),
			[this](boost::system::error_code ec, std::size_t /*length2*/) {
			if (!ec) {
				start_read();
			}
			/*else {
				
				cout << "do_write ec:" <<ec << endl;
			}*/
		});
	}
	void start_read2() {
		memset(data2_, 0, sizeof(data2_));
		(*csock).async_read_some(
			boost::asio::buffer(data2_, max_length),
			[this](boost::system::error_code ec, std::size_t length) {
			if (!ec) {
				do_write2(length);
			}
			else {
				//cout << "do_read2 ec:" << ec << endl;
				cd = true;
				if (sd) {
					(*csock).close();
					socket_.close();
				}
			}
		});
	}
	void do_write2(std::size_t length) {
		boost::asio::async_write(socket_, boost::asio::buffer(data2_, length),
			[this](boost::system::error_code ec, std::size_t /*length2*/) {
			if (!ec) {
				start_read2();
			}
			/*else {
				cout << "do_write2 ec:" << ec << endl;
			}*/
		});
	}
public:
	bool cd = false;//client done
	bool sd = false;//server done
	tcp::resolver::results_type endpoints_;
	tcp::socket socket_;
	enum { max_length = 10000 };
	char data_[max_length];
	char data2_[max_length];
};
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

void reply_client() {
	unsigned char res[8];
	res[0] = 0;
	if (ginfo.accept) { res[1] = 90; }
	else { res[1] = 91; }
	//res[2] = stoi(ginfo.dst_port) / 256;
	//res[3] = stoi(ginfo.dst_port) % 256;
	if (ginfo.mode == 1) {//connect
		res[2] = 0;
		res[3] = 0;
	}
	else {//bind
		srand(time(NULL));//generate a port for server to bind
		int x = rand() % 100;
		gport = 50000 + x;
		res[2] = gport / 256;
		res[3] = gport % 256;
	}
	res[4] = 0;
	res[5] = 0;
	res[6] = 0;
	res[7] = 0;
	

	boost::asio::async_write(*csock, boost::asio::buffer(res, 8),
		std::bind(do_nothing, std::placeholders::_1));

}
class session2
	: public std::enable_shared_from_this<session2> {
public:
	session2(tcp::socket socket)
		: socket_(std::move(socket)) {
	}

	void start() {
		unsigned char res[8];
		res[0] = 0;
		if (ginfo.accept) { res[1] = 90; }
		else { res[1] = 91; }
		res[2] = gport / 256;
		res[3] = gport % 256;
		res[4] = 0;
		res[5] = 0;
		res[6] = 0;
		res[7] = 0;
		boost::asio::async_write(socket_, boost::asio::buffer(res, 8),
			bind(&session2::receive_reply, this, _1));
	}
	void receive_reply(const boost::system::error_code& error) {
		if (!error) {
			start_read();
			start_read2();
		}
	}
private:
	void start_read() {
		memset(data_, 0, sizeof(data_));
		socket_.async_read_some(
			boost::asio::buffer(data_, max_length),
			[this](boost::system::error_code ec, std::size_t length) {
			if (!ec) {
				do_write(length);
			}
			else {
				sd = true;
				if (cd) {
					(*csock).close();
					socket_.close();
				}
				//cout << "do_read ec:" << ec << endl;
			}
		});
	}
	void do_write(std::size_t length) {
		boost::asio::async_write(*csock, boost::asio::buffer(data_, length),
			[this](boost::system::error_code ec, std::size_t /*length2*/) {
			if (!ec) {
				start_read();
			}
			/*else {

				cout << "do_write ec:" <<ec << endl;
			}*/
		});
	}
	void start_read2() {
		memset(data2_, 0, sizeof(data2_));
		(*csock).async_read_some(
			boost::asio::buffer(data2_, max_length),
			[this](boost::system::error_code ec, std::size_t length) {
			if (!ec) {
				do_write2(length);
			}
			else {
				//cout << "do_read2 ec:" << ec << endl;
				cd = true;
				if (sd) {
					(*csock).close();
					socket_.close();
				}
			}
		});
	}
	void do_write2(std::size_t length) {
		boost::asio::async_write(socket_, boost::asio::buffer(data2_, length),
			[this](boost::system::error_code ec, std::size_t /*length2*/) {
			if (!ec) {
				start_read2();
			}
			/*else {
				cout << "do_write2 ec:" << ec << endl;
			}*/
		});
	}

	bool cd = false;//client done
	bool sd = false;//server done
	tcp::socket socket_;
	enum { max_length = 1024 };
	char data_[max_length];
	char data2_[max_length];
};
class server2 {
public:
	server2(int port)
		: acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
		do_accept();
	}

private:
	void do_accept() {
		acceptor_.async_accept(
			[this](boost::system::error_code ec, tcp::socket socket) {
			if (!ec) {
				// create an object(session) and call its start() fuction
				std::make_shared<session2>(std::move(socket))->start();
			}
		});
	}

	tcp::acceptor acceptor_;
};
void connect_mode() {
	if (ginfo.mode == 1) {//connect mode
		//std::make_shared<client>(io_context)->start();
		//io_context.run();
		client c(io_context);
		c.start();
		io_context.run();
	}
	else {//bind mode
		server2 s(gport);
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
					//save client sock to global variable
					csock = &socket_;
					//set info here
					ginfo.mode = data_[1];
					ginfo.src_ip = socket_.remote_endpoint().address().to_string();
					ginfo.src_port = to_string(socket_.remote_endpoint().port());
					ginfo.dst_ip = to_string(data_[4]) + "." + to_string(data_[5]) + "." + to_string(data_[6]) + "." + to_string(data_[7]);
					ginfo.dst_port = to_string(data_[2] << 8 | data_[3]);
					//socks4a
					if ((to_string(data_[4]) == "0") && (to_string(data_[5]) == "0") && (to_string(data_[6]) == "0") && (to_string(data_[7]) != "0")) {
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
								break;
							}
						}
					}

					//firewall
					firewall();
					//show message for hw request
					show_message();
					//reply client
					reply_client();

					connect_mode();
				}
				else {
					io_context.notify_fork(boost::asio::io_context::fork_parent);
					socket_.close();
				}

			}
		});
	}

	tcp::socket socket_;
	enum { max_length = 1024 };
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