#pragma once
#include <sstream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <ws2tcpip.h>

//requestdata
//uname=admin&psw=123456&remember=on
//uname=admin&psw=123456

using namespace std;

#define REQ_METHOD_SIZE 10
#define FILENAME_SIZE 100
#define HTTP_VER_SIZE 10
#define DATA_SIZE 1024*1024 //1mb
#define BUFFER_SIZE 1024*1024
#define RES_STATUS_SIZE 30
#define CONTENT_TYPE_SIZE 30
const char* FILE404 = "web_src/err404.html";
const char* FILE401 = "web_src/err401.html";

const string USERNAME = "admin";
const string PASSWORD = "123456";

struct Request {
    //nesscessary part
    char* method;
    char* contentLocation;//index.html
    char* http_ver;//http/1.1
    char* data;//username pw
    int data_len;
    bool close_connection = false;
};

struct Response
{
    char* http_ver;
    char* status; //200 ok, 404 not found, 401 unauthor
    char* content_type;
    char* contentLocation;
    int contentLength = 0;
    bool close_connection = false;
};


//read browser request and store in request struct
Request PairseRequest(char* browser_req, int buf_len)
{
    Request req = { NULL, NULL, NULL, NULL, 0, 0 };
    req.method = new char[REQ_METHOD_SIZE] {};
    req.contentLocation = new char[FILENAME_SIZE] {};
    req.http_ver = new char[HTTP_VER_SIZE] {};
    req.data = new char[DATA_SIZE] {};
    req.data_len = 0;

    stringstream ss(browser_req);
    //get method, contenLocation and http version
    char* sub_dir = new char [FILENAME_SIZE] {};
    ss >> req.method;
    ss >> sub_dir;
    ss >> req.http_ver;
    if (strcmp(sub_dir, "/") == 0) {
        sprintf_s(req.contentLocation, FILENAME_SIZE, "web_src/index.html");
    }
    else {
        sprintf_s(req.contentLocation, FILENAME_SIZE, "web_src%s", sub_dir);
    }
    delete[] sub_dir;

    ss.ignore(); //ignore '\n'


    //find connection and reach to data
    string line;
    string close("Connection: close");
    do {
        getline(ss, line, '\n');
        if (line.find(close) != std::string::npos)
            req.close_connection = 1;
    } while (line.length() != 1);

    int idx = 0;

    //getdata and datalength
    while (!ss.eof())
    {
        req.data[idx] = ss.get();
        idx++;
    }

    req.data[--idx] = '\0'; //donknow why stringstream is 1 character longer
    req.data_len = idx;
    return req;
}

bool falseLog(char* data) {
    // change account here
    const string user = "admin";
    const string pass = "123456";

    const string rightLog = "uname=" + user + "&psw=" + pass;

    string infoLog = data;

    if (infoLog.size() < rightLog.size())
        return 1; // login false

    infoLog.resize(rightLog.size());

    if (infoLog == rightLog)
        return 0; // login success

    return 1; // login false
}

//read request struct to create request struct
Response CreateResponseStruct(Request req) //nhớ check login, handlecontenttype
{
    Response res{ NULL,NULL, NULL, NULL, 0 };
    res.http_ver = new char[HTTP_VER_SIZE] {};
    res.status = new char[RES_STATUS_SIZE] {};
    res.contentLocation = new char [FILENAME_SIZE] {};
    res.content_type = new char [CONTENT_TYPE_SIZE] {};


    //get htttp version
    sprintf_s(res.http_ver, HTTP_VER_SIZE, req.http_ver);


    //check error, get status, contenttype and contentLocation
    ifstream inf;
    inf.open(req.contentLocation, ios::binary);
    if (!inf) {
        sprintf_s(res.status, RES_STATUS_SIZE, "404 Not Found");    //status
        sprintf_s(res.contentLocation, FILENAME_SIZE, FILE404);      //file error
        sprintf_s(res.content_type, CONTENT_TYPE_SIZE, "text/html"); //contenttype
        inf.open(FILE404, ios::binary);
        inf.seekg(0, inf.end);
        res.contentLength = inf.tellg();
        inf.close();
    }
    else if (strcmp(req.method, "POST") == 0 && falseLog(req.data)) {
        sprintf_s(res.status, RES_STATUS_SIZE, "401 Unauthorized"); //status
        sprintf_s(res.contentLocation, FILENAME_SIZE, FILE401);      //file error
        sprintf_s(res.content_type, CONTENT_TYPE_SIZE, "text/html"); //contenttype
        inf.close();
        inf.open(FILE401, ios::binary);
        inf.seekg(0, inf.end);
        res.contentLength = inf.tellg();
        inf.close();
    }
    else {
        sprintf_s(res.status, RES_STATUS_SIZE, "200 OK");
        sprintf_s(res.contentLocation, FILENAME_SIZE, req.contentLocation);
        inf.seekg(0, inf.end);
        res.contentLength = inf.tellg();

        //find file_tail
        string filename(req.contentLocation);
        string file_tail = filename.substr(filename.find(".") + 1);

        //get content type
        if (file_tail == "txt")
            sprintf_s(res.content_type, CONTENT_TYPE_SIZE, "text/plain");
        else if (file_tail == "html" || file_tail == "htm")
            sprintf_s(res.content_type, CONTENT_TYPE_SIZE, "text/html");
        else if (file_tail == "css")
            sprintf_s(res.content_type, CONTENT_TYPE_SIZE, "text/css");
        else if (file_tail == "jpg" || file_tail == "jpeg")
            sprintf_s(res.content_type, CONTENT_TYPE_SIZE, "image/jpeg");
        else if (file_tail == "png")
            sprintf_s(res.content_type, CONTENT_TYPE_SIZE, "image/png");
        else
            sprintf_s(res.content_type, CONTENT_TYPE_SIZE, "application/octet-stream");
    }
    inf.close();

    //close connection
    res.close_connection = req.close_connection;
    return res;
}

void freeReq(Request& req){
    delete[] req.contentLocation;
    delete[] req.data;
    delete[] req.method;
    delete[] req.http_ver;
}
void freeRes(Response& res) {
    delete[] res.contentLocation;
    delete[] res.http_ver;
    delete[] res.content_type;
    delete[] res.status;
}

void Communicate(SOCKET clt_sock, char* ip, unsigned short port) {

    printf("Accepted a connection to %s:%hu\n", ip, port);
    char* buffer = new char[BUFFER_SIZE] {};
    int buf_len = 0;

    do {
        memset(buffer, 0, BUFFER_SIZE);
        buf_len = 0;
        //receive http_msg
        buf_len = recv(clt_sock, buffer, BUFFER_SIZE, 0);
        if (buf_len > 0) {
            printf("Received %d byte from %s:%hu\n" , buf_len, ip, port);
        }
        else if(buf_len == 0) {
            printf("Received FIN from %s:%hu\n", ip, port);
            closesocket(clt_sock);
            printf("Close Connection to %s:%hu\n", ip, port);
            delete[] buffer;
            delete[] ip;
            return;
        }
        else {
            closesocket(clt_sock);
            printf("Close connection to %s:%hu on recv error\n", ip, port);
            delete[] buffer;
            delete[] ip;
            return;
        }
        //pairse request
        Request req = PairseRequest(buffer, buf_len);

        Response res = CreateResponseStruct(req);
        freeReq(req);

        //store message and send msg
        memset(buffer, 0, BUFFER_SIZE);
        //header
        buf_len = sprintf_s(buffer, BUFFER_SIZE, "%s %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n", res.http_ver, res.status, res.content_type, res.contentLength);
        if (res.close_connection)
            buf_len += sprintf_s(buffer + buf_len, BUFFER_SIZE - buf_len, "Connection: close\r\n");
        buf_len += sprintf_s(buffer + buf_len, BUFFER_SIZE - buf_len, "\r\n");

        //body
        ifstream inf;
        inf.open(res.contentLocation, ios::binary);
        if (!inf) {
            printf("Cannot open file!\n");
            delete[] buffer;
            delete[] ip;
            return;
        }
        int data_remain = res.contentLength;
        int buf_remain = BUFFER_SIZE - buf_len;
        char* buf_offset = buffer + buf_len;
        int sResult = 0;
        while (data_remain > 0) {
            if (data_remain <= buf_remain) {
                inf.read(buf_offset, data_remain);
                buf_len += data_remain;
                data_remain = 0;
                inf.close();
            }
            else {
                inf.read(buf_offset, buf_remain);
                data_remain -= buf_remain;
                buf_len = BUFFER_SIZE;
            }
            sResult = send(clt_sock, buffer, buf_len, 0);

            if (sResult < 0) {
                closesocket(clt_sock);
                printf("Close connection to %s:%hu on send() error\n", ip, port);
                delete[] buffer;
                delete[] ip;
                inf.close();
                return;
            }
            printf("Sent %d byte to %s:%hu\n", sResult, ip, port);
            memset(buffer, 0, BUFFER_SIZE);
            buf_len = 0;
            buf_remain = BUFFER_SIZE;
            buf_offset = buffer;
        }


        if (res.close_connection) {
            freeRes(res);
            closesocket(clt_sock);
            printf("Close connection to %s:%hu\n", ip, port);
            delete[] buffer;
            delete[] ip;
            return;
        }
        freeRes(res);
    } while (1);
    //closesocket(clt_sock);
    //printf("Close connection to %s:%hu\n", ip, port);
    //delete[] buffer;
    return;
}

