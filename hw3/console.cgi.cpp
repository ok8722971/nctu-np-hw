#include <bits/stdc++.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/regex.hpp>
#include <fstream>

using boost::asio::ip::tcp;
using std::placeholders::_1;
using std::placeholders::_2;
using namespace std;

enum { max_length = 2048 };


class client
{
public:
    client(boost::asio::io_context& io_context, string ip, string port, string filename)
        : socket_(io_context), ip_(ip), port_(port), filename_(filename){

        fstream file;
        char buffer[2000];
        filename_ = "test_case/" + filename_ ;
        file.open(filename_.c_str(), ios::in);
        do{
            file.getline(buffer, sizeof(buffer));
            cmd_.push_back(buffer);
        }while(!file.eof());
        file.close();
        cout << ip << ":" << port << "<br>" << endl;
    }

    void start(tcp::resolver::results_type endpoints){
        endpoints_ = endpoints;
        start_connect(endpoints_.begin());
    }
    void stop(){
        stopped_ = true;
        //boost::system::error_code ignored_error;
        //socket_.close(ignored_error);
    }
private:
    void start_connect(tcp::resolver::results_type::iterator endpoint_iter)  {
        if(endpoint_iter != endpoints_.end()){
            socket_.async_connect(endpoint_iter->endpoint(),
              std::bind(&client::handle_connect, this, _1, endpoint_iter));
        }
        else{
            stop();
        }
    }

    void handle_connect(const boost::system::error_code& error,
        tcp::resolver::results_type::iterator endpoint_iter){
        if (stopped_) return;

        if (!socket_.is_open()){
            std::cout << "Connect timed out<br>";
            start_connect(++endpoint_iter);
        }

        else if (error){
            std::cout << "Connect error: " << error.message() << "\n";

            socket_.close();

            start_connect(++endpoint_iter);
        }

        else{
            start_read();
        }
    }

    void start_read(){
        boost::asio::async_read_until(socket_,
          boost::asio::dynamic_buffer(input_buffer_), boost::regex("[%]"),
          std::bind(&client::handle_read, this, _1, _2));
    }
    string ReplaceAll(string s, string from, string to) {
    size_t pos = 0;
    while((pos = s.find(from, pos)) != string::npos) {
        s.replace(pos, from.length(), to);
        pos += to.length();
    }
    return s;
}
    void handle_read(const boost::system::error_code& error, std::size_t n){
        if (stopped_) {
            return;
        }
        if (!error){
            std::string line(input_buffer_.substr(0, n - 1));
            input_buffer_.erase(0, n);
            line = ReplaceAll(line, std::string("\n"), std::string("<br>"));
            if( !line.empty() ){
                cout << line ;
                cout << "% ";
                cout.flush();
                start_write();
            }else {
                start_read();
            }
        }
    }

    void start_write(){

        if(stopped_){
            return;
        }
        if( cmd_.at(i) == "exit" || cmd_.at(i) == "exit\r"){
            stop();
        }

        char cmd[max_length];
        if(cmd_.at(i) == "exit\r") cmd_.at(i) == "exit";
        cmd_.at(i) = cmd_.at(i) + "\n";
        strcpy(cmd, cmd_.at(i).c_str());
        i++;
        size_t cmd_length = strlen(cmd);
        cout << cmd << "<br>";
        cout.flush();
        boost::asio::async_write(socket_, boost::asio::buffer(cmd, cmd_length),
            bind(&client::handle_write, this, _1));


    }

    void handle_write(const boost::system::error_code& error){
        /*if (stopped_){
            return;
        }*/

        if (!error){
            start_read();
        }
    }

private:
    bool stopped_ = false;
    tcp::resolver::results_type endpoints_;
    tcp::socket socket_;
    string input_buffer_;
    string ip_;
    string port_;
    string filename_;
    size_t i = 0;//remember which cmd i read
    vector<string> cmd_;
};

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

int main(int argc, char* argv[]){
    try{
        cout << "Content-type: text/html" << endl << endl;
        string tmp(getenv("QUERY_STRING"));
        vector<string> tmp2 = split( tmp, "=" );

        boost::asio::io_context io_context;
        tcp::resolver r(io_context);

        client c1(io_context
                , tmp2.at(1).substr(0,tmp2.at(1).size()-3)
                , tmp2.at(2).substr(0,tmp2.at(2).size()-3)
                , tmp2.at(3).substr(0,tmp2.at(3).size()-3));
        /*client c2(io_context);
        client c3(io_context);
        client c4(io_context);
        client c5(io_context);*/

        c1.start(r.resolve( tmp2.at(1).substr(0,tmp2.at(1).size()-3).c_str()
                            , tmp2.at(2).substr(0,tmp2.at(2).size()-3).c_str()));

        io_context.run();
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
