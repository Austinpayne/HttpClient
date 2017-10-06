#ifndef __HTTP_CLIENT_H_
#define __HTTP_CLIENT_H_

#include "application.h"
#include "spark_wiring_string.h"
#include "spark_wiring_tcpclient.h"
#include "spark_wiring_usbserial.h"

#define HTTP_METHOD_GET    "GET"
#define HTTP_METHOD_POST   "POST"
#define HTTP_METHOD_PUT    "PUT"
#define HTTP_METHOD_DELETE "DELETE"
#define HTTP_METHOD_PATCH  "PATCH"

#define SERVER_RESPOND_WAIT 200 // ms to wait for server to respond
#define TIMEOUT 5000 // ms
#define CRLF "\r\n"
#define LF   "\n"
#define HTTP_VERSION_LEN 8
#define HTTP_STATUS_CODE_LEN 3

// http_header arrays are null terminated
// when making a request
typedef struct http_header {
    char *key;
    char *val;
} http_header_t;

typedef struct http_request {
    char *hostname;
    int  port;
    char *path;
    char *body;
} http_request_t;

// buffer is used to store entire response (headers+body)
// after request, it should not be touched as version, reason,
// body, and header keys, vals reference char* contained in
// the buffer
typedef struct http_response {
    char *buffer;
    int  max_buffer_size;
    http_header_t *headers;
    int  max_headers;
    char *version;
    int  status;
    char *reason;
    char *body;
} http_response_t;

class HttpClient {
public:
    TCPClient client;

    // constructor
    HttpClient(void);

    // 2xx
    int ok(int status) {
        return status_between(status, 200, 300);
    }

    // 3xx
    int redirect(int status) {
        return status_between(status, 300, 400);
    }

    // 4xx
    int client_error(int status) {
        return status_between(status, 400, 500);
    }

    // 5xx
    int server_error(int status) {
        return status_between(status, 500, 600);
    }

    void get(http_request_t &req, http_response_t &res) {
        request(req, res, NULL, HTTP_METHOD_GET);
    }

    void post(http_request_t &req, http_response_t &res) {
        request(req, res, NULL, HTTP_METHOD_POST);
    }

    void put(http_request_t &req, http_response_t &res) {
        request(req, res, NULL, HTTP_METHOD_PUT);
    }

    void del(http_request_t &req, http_response_t &res) {
        request(req, res, NULL, HTTP_METHOD_DELETE);
    }

    void get(http_request_t &req, http_response_t &res, http_header_t headers[]) {
        request(req, res, headers, HTTP_METHOD_GET);
    }

    void post(http_request_t &req, http_response_t &res, http_header_t headers[]) {
        request(req, res, headers, HTTP_METHOD_POST);
    }

    void put(http_request_t &req, http_response_t &res, http_header_t headers[]) {
        request(req, res, headers, HTTP_METHOD_PUT);
    }

    void del(http_request_t &req, http_response_t &res, http_header_t headers[]) {
        request(req, res, headers, HTTP_METHOD_DELETE);
    }

    void patch(http_request_t &req, http_response_t &res, http_header_t headers[]) {
        request(req, res, headers, HTTP_METHOD_PATCH);
    }

private:
    void request(http_request_t &request, http_response_t &response, http_header_t headers[], const char* method);
    void sendHeader(const char* key, const char* val);
    void sendHeader(const char* key, const int val);
    void clear_response(http_response_t &response);
    int parse_response(http_response_t &response);
    int status_between(int status, int low, int high) {
        if (status >= low && status < high)
            return 1;
        return 0;
    }
};

#endif /* __HTTP_CLIENT_H_ */
