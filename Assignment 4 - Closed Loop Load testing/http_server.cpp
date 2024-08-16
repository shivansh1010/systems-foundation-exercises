#include "http_server.hh"

#include <vector>

#include <sys/stat.h>

#include <fstream>
#include <sstream>
using namespace std;

vector<string> split(const string &s, char delim) {
    vector<string> elems;

    stringstream ss(s);
    string item;

    while (getline(ss, item, delim)) {
        if (!item.empty())
            elems.push_back(item);
    }

    return elems;
}

HTTP_Request::HTTP_Request(string request) {
    vector<string> lines = split(request, '\n');
    vector<string> first_line = split(lines[0], ' ');

    this->HTTP_version = "1.0";

    this->method = first_line[0];
    this->url = first_line[1];

    if (this->method != "GET") {
        cerr << "Method '" << this->method << "' not supported" << endl;
        exit(1);
    }
}

HTTP_Response *handle_request(string req) {
    string root_path = "/home/sd/Desktop/web-server/";
    HTTP_Request *request = new HTTP_Request(req);

    HTTP_Response *response = new HTTP_Response();

    string url = root_path + string("html_files") + request->url;

    response->HTTP_version = "1.0";

    struct stat sb;
    if (stat(url.c_str(), &sb) == 0) // requested path exists
    {
        response->status_code = "200";
        response->status_text = "OK";
        response->content_type = "text/html";

        string body;

        if (S_ISDIR(sb.st_mode)) {
            url = url + "/index.html";
            if (stat(url.c_str(), &sb) != 0) {
                response->status_code = "404";
                response->status_text = "Not Found";
                response->content_type = "text/html";
                response->content_length = "0";
                delete request;
                return response;
            }
        }

        ifstream file;
        file.open(url, ios::in);
        if (file) {
            ostringstream line;
            line << file.rdbuf();
            response->body = line.str();
        }
        // cout << response->body << endl;
        int l = response->body.length();
        response->content_length = to_string(l);
    }

    else {
        response->status_code = "404";
        response->status_text = "Not Found";
        response->content_type = "text/html";
        response->content_length = "0";
    }

    delete request;

    return response;
}

string HTTP_Response::get_string() {
    string ret = "HTTP/" + this->HTTP_version + " " + this->status_code + " " + this->status_text + "\n";

    char temp[100];
    time_t curr = time(0);
    struct tm ttmm = *gmtime(&curr);
    strftime(temp, sizeof temp, "%a, %d %b %Y %H:%M:%S %Z", &ttmm);
    ret = ret + "Date: " + string(temp) + "\n";
    ret = ret + "Content-Length: " + this->content_length + "\n";
    if (this->status_code == "404")
        return ret;
    ret = ret + "Content-Type: " + this->content_type + "\n";

    ret = ret + "\n";
    ret = ret + this->body;
    return ret;
}
