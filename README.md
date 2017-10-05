# HttpClient for Particle Photon | Arduino

This is a fork of nmattisson's HttpClient for Spark Core, Arduino. I found it to be a good starting point for an http client and really liked the API that nmattison created. However, I have reworked many parts of it to complete basic funcionality. Additions include:

* use macros where it makes sense
* rename many variables for readability
* use user supplied buffer instead of internal, static buffer
* use char * instead of String (i.e. prefer C where possible)
* remove IPAddress member in request and just use hostname
* add version, reason, and headers to request
* complete parsing of requests, including headers
* clean-up (use printf where possible) and add more logging
* add http status check methods (ok, redirect, error, etc.)

### Example usage

```c
#include "HttpClient.h"

#define RES_SIZE 1024
#define HEADERS_SIZE 16

HttpClient http;
char response_buffer[RES_SIZE];
http_header_t headers[HEADERS_SIZE];
http_header_t default_headers[] = {
    { "Content-Type", "application/json" },
    { "Accept" , "application/json" },
    { "Cache-Control" , "no-cache"},
    { NULL } // headers are null terminated
};

void setup() {
    Serial.begin(9600);
}

void loop() {
    http_response_t response = {response_buffer, RES_SIZE, headers, HEADERS_SIZE};
    http_request_t request = {"http://httpbin.org", 80, "/ip"};

    // can also send a body:
    // char *body = "{\"key\":\"value\"}";
    // http_request_t request = {"http://httpbin.org", 80, "/post", body};

    http.get(request, response, default_headers);
    Serial.printf("%s %d %s\n", response.version, response.status, response.reason);
    Serial.printf("got %s\n", response.body);

    delay(2000);
}
```
