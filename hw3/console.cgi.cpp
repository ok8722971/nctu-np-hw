#include <bits/stdc++.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/regex.hpp>

using boost::asio::ip::tcp;
using std::placeholders::_1;
using std::placeholders::_2;
using namespace std;

enum { max_length = 1024 };


class client
{
public:
    client(boost::asio::io_context& io_context)
        : socket_(io_context){
    }
    void start(tcp::resolver::results_type endpoints){
        endpoints_ = endpoints;
        start_connect(endpoints_.begin());
    }
    void stop(){
        stopped_ = true;
        boost::system::error_code ignored_error;
        socket_.close(ignored_error);
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
          boost::asio::dynamic_buffer(input_buffer_), boost::regex("[\r\n%]"),
          std::bind(&client::handle_read, this, _1, _2));
    }

    void handle_read(const boost::system::error_code& error, std::size_t n){
        if (stopped_) return;
        if (!error){
            std::string line(input_buffer_.substr(0, n - 1));
            input_buffer_.erase(0, n);
            if (!line.empty()){
                cout << line << "<br>";
                cout.flush();
                start_read();
            }else{
                cout << "%";
                cout.flush();
                start_write();
            }
        }
        else{
            cout << "Error on receive: " << error.message() << "\n";
            stop();
        }
    }

    void start_write(){
        if (stopped_) return;

        char request[max_length];
        strcpy(request, "ls\n");
        size_t request_length = strlen(request);
		cout << request << "<br>";
		cout.flush();
		boost::asio::async_write(socket_, boost::asio::buffer(request, request_length),
            bind(&client::handle_write, this, _1));

    }

    void handle_write(const boost::system::error_code& error){
        if (stopped_) return;

        if (!error){
            start_read();
        }
        else{
            cout << "Error on heartbeat: " << error.message() << "\n";
            stop();
        }
    }

private:
    bool stopped_ = false;
    tcp::resolver::results_type endpoints_;
    tcp::socket socket_;
    std::string input_buffer_;
};

int main(int argc, char* argv[]){
    try{
        cout << "Content-type: text/html" << endl << endl;

        boost::asio::io_context io_context;
        tcp::resolver r(io_context);
        client c(io_context);

        c.start(r.resolve("nplinux12.cs.nctu.edu.tw", "8763"));

        io_context.run();
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
