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

string replaceAll(string s, string from, string to) {
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != string::npos) {
        s.replace(pos, from.length(), to);
        pos += to.length();
    }
    return s;
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

string replaceEscape(string s) {
    s = replaceAll(s, "&", "&amp");
    s = replaceAll(s, "\"", "&quot");
    s = replaceAll(s, "'", "&#39");
	s = replaceAll(s, "<", "&lt");
	s = replaceAll(s, ">", "&gt");
    s = replaceAll(s, "\r", "");
    s = replaceAll(s, "\n", "<br>");
    return s;
}

class client
{
public:
    client(boost::asio::io_context& io_context)
        :socket_(io_context){
    }
    void outputResult(int id, string msg) {
        msg = replaceEscape(msg);
        cout << "<script>document.getElementById('" << id << "').innerHTML += '" << msg << "'; </script>" << endl;
    }
    void outputCmd(int id, string msg) {
        msg = replaceEscape(msg);
        cout << "<script>document.getElementById('" << id << "').innerHTML += '<font color=\"blue\">" << msg << "</font>'</script>" << endl;
    }

    void start(tcp::resolver::results_type endpoints){
        fstream file;
        char buffer[2000];
        filename_ = "test_case/" + filename_ ;
        file.open(filename_.c_str(), ios::in);
        do{
            file.getline(buffer, sizeof(buffer));
            cmd_.push_back(buffer);
        }while(!file.eof());
        file.close();
        //cout << ip_ << ":" << port_ << "<br>" << endl;
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
            send_request();
        }
    }
    void send_request() {
        unsigned char req[8];

        req[0] = 4;
        req[1] = 1;
        req[2] = stoi(port_) / 256;
        req[3] = stoi(port_) % 256;

        vector<string> ip = split(ip_, ".");
        req[4] = (unsigned char)stoi(ip.at(0));
        req[5] = (unsigned char)stoi(ip.at(1));
        req[6] = (unsigned char)stoi(ip.at(2));
        req[7] = (unsigned char)stoi(ip.at(3));

        boost::asio::async_write(socket_, boost::asio::buffer(req, 8),
            bind(&client::receive_reply, this, _1));
    }
    void receive_reply(const boost::system::error_code& error) {
        if (!error) {
            socket_.async_read_some(boost::asio::buffer(reply, 8),
                [this](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    //outputCmd(id_, host_ + ":" + port_ + "\n");
                    start_read();
                }
            });
        }
    }

    void start_read(){
        boost::asio::async_read_until(socket_,
          boost::asio::dynamic_buffer(input_buffer_), boost::regex("[%]"),
          std::bind(&client::handle_read, this, _1, _2));
    }


    void handle_read(const boost::system::error_code& error, std::size_t n){
        if (stopped_) {
            //socket_.close();
            return;
        }
        if (!error){
            std::string line(input_buffer_);

            if( !line.empty() ){
                outputResult(id_, line);
                input_buffer_.clear();
                start_write();
            }else {
                input_buffer_.clear();
                start_read();
            }

        }
    }

    void start_write(){

        if(stopped_){
            //socket_.close();
            return;
        }
        if( cmd_.at(i) == "exit" || cmd_.at(i) == "exit\r"){
            stop();
        }

        outputCmd(id_, cmd_.at(i) + "\n");
        cmd_.at(i) = cmd_.at(i) + "\n";
        cmd = cmd_.at(i);
        i++;
        size_t cmd_length = cmd.size();

        boost::asio::async_write(socket_, boost::asio::buffer(cmd, cmd_length),
            bind(&client::handle_write, this, _1));


    }

    void handle_write(const boost::system::error_code& error){
        if (stopped_){
            //socket_.close();
            return;
        }

        if (!error){
            cmd.clear();
            start_read();
        }
    }

public:
    bool stopped_ = false;
    tcp::resolver::results_type endpoints_;
    tcp::socket socket_;
    int id_;
    string input_buffer_;
    string host_;
    string ip_;
    string port_;
    string filename_;
    size_t i = 0;//remember which cmd i read
    vector<string> cmd_;
    string cmd;
    unsigned char reply[8];
};


void print_html( vector<string> tmp2 ) {
    cout <<
        "<!DOCTYPE html>\
        <html lang=\"en\">\
        <head>\
             <meta charset=\"UTF-8\" />\
        <title>NP Project 4</title>\
        <link\
          rel=\"stylesheet\"\
          href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\
          integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\
          crossorigin=\"anonymous\"\
        />\
        <link\
          href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\
          rel=\"stylesheet\"\
        />\
        <link\
          rel=\"icon\"\
          type=\"image/png\"\
          href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\
        />\
        <style>\
          * {\
            font-family: \'Source Code Pro\', monospace;\
            font-size: 1rem !important;\
          }\
          body {\
            background-color: #212529;\
          }\
          pre {\
            color: #cccccc;\
          }\
          b {\
            color: #01b468;\
          }\
        </style>\
        </head>"
        << endl;


    cout <<
        "<body>\
         <table class=\"table table-dark table-bordered\">\
         <thead>\
         <tr>"
        << endl;
    for (int i = 0; i < 5; i++) {
        if (tmp2.at(3 * i + 1) != "") {
            cout << "<th scope=\"col\">" << tmp2.at(i * 3 + 1) << ":" << tmp2.at(i * 3 + 2) << "</th>" << endl;
        }
    }

    cout <<
        "</tr>\
         </thead>\
         <tbody>\
         <tr>"
        << endl;
    for(int i = 0; i < 5; i++){
        cout << "<td><pre id=\"" << i << "\" class=\"mb-0\"></pre></td>" << endl;
    }

    cout <<
        "</tr>\
        </tbody>\
        </table>\
        </body>\
        </html>"
       << endl;
}

int main(int argc, char* argv[]){
    try {
        cout << "Content-type: text/html" << endl << endl;
        string tmp(getenv("QUERY_STRING"));
        vector<string> tmp2 = split(tmp, "=");
        for (size_t i=0; i < tmp2.size() - 1; i++) {
            tmp2.at(i) = tmp2.at(i).substr(0,tmp2.at(i).size() - 3);
        }
        /*int i = 0;
        for (string x : tmp2) {
            cout << i << " : "<<x << "<br>";
            i++;
        }*/
        print_html(tmp2);
        boost::asio::io_context io_context;
        tcp::resolver r(io_context);
        client c[5] = { client(io_context),client(io_context), client(io_context), client(io_context), client(io_context) };
        for (int k = 0; k < 5; k++) {
            if (tmp2.at(k * 3 + 1) != "") {
                tcp::resolver::query query(tmp2.at(k * 3 + 1).c_str(), tmp2.at(k * 3 + 2).c_str());
                tcp::resolver::iterator itbegin = r.resolve(query), itEnd;
                tcp::endpoint pt = itbegin->endpoint();
                //cout << "ip:" << pt.address() << pt.port() << endl;
                c[k].host_ = tmp2.at(k * 3 + 1);
                c[k].ip_ = pt.address().to_string();
                c[k].port_ = to_string(pt.port());
                c[k].filename_ = tmp2.at(k * 3 + 3);
                c[k].id_ = k;
                c[k].start(r.resolve(tmp2.at(16).c_str(), tmp2.at(17).c_str()));
                /*cout << "k: " << k << endl;
                cout << "host: " << c[k].host_ << endl;
                cout << "ip: " << c[k].ip_ << endl;
                cout << "port: " << c[k].port_ << endl;
                cout << "file: " << c[k].filename_ << endl;
                cout << "socks_ip: " << tmp2.at(16).c_str() << " socks_port:" << tmp2.at(17).c_str() << endl;
                cout << "<br>" << endl;*/
            }
            else {
                break;
            }
        }
        io_context.run();
    }
    catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
