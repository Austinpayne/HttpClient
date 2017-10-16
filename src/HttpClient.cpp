#include "HttpClient.h"

/*
 *  constructor
 */
HttpClient::HttpClient() {}

/*
 *  send header with char* value
 */
void HttpClient::sendHeader(const char* key, const char* val) {
    #ifdef LOGGING
    Serial.printf("sending header: '%s: %s'\n", key, val);
    #endif

    client.print(key);
    client.print(": ");
    client.print(val);
    client.print(CRLF);
}

/*
 *  send header with int value
 */
void HttpClient::sendHeader(const char* key, const int val) {
    #ifdef LOGGING
    Serial.printf("sending header: '%s: %d'\n", key, val);
    #endif

    client.print(key);
    client.print(": ");
    client.print(val);
    client.print(CRLF);
}

// clear response buffer in case it is reused
void HttpClient::clear_response(http_response_t &response) {
    #ifdef LOGGING
    Serial.printf("clearing headers\n");
    #endif
    int i;
    for (i=0; i < response.max_headers; i++) {
        response.headers[i].key = NULL;
        response.headers[i].val = NULL;
    }
    #ifdef LOGGING
    Serial.printf("clearing response buffer\n");
    #endif
    memset(response.buffer, 0, sizeof(response.max_buffer_size));
    response.version = NULL;
    response.status  = 0;
    response.reason  = NULL;
    response.body    = NULL;
    #ifdef LOGGING
    Serial.printf("done clearing response\n");
    #endif
}

int HttpClient::parse_response(http_response_t &response) {
    #ifdef LOGGING
    Serial.println("parsing response...");
    #endif

    if (strlen(response.buffer) >= HTTP_VERSION_LEN+HTTP_STATUS_CODE_LEN+1) {
        char *header_end;
        char *str, *tmp;
        char *line, *key, *val;
        int content_len = 0;

        int i;
        char *crlf = CRLF CRLF;
        // find header_end to know when to stop parsing headers (and start parsing body)
        for (i=0; i<2; i++) { // try twice in case server use "\n\n"
            tmp = strstr(response.buffer, crlf);
            if (tmp && strlen(tmp) > strlen(crlf)) {
                header_end = tmp;
                break;
            }
            crlf = LF LF;
        }

        // parse status line
        str = strtok_r(response.buffer, " ", &tmp);
        if (str)
            response.version = str;
        str = strtok_r(NULL, " ", &tmp);
        if (str)
            response.status = atoi(str);
        str = strtok_r(NULL, CRLF, &tmp);
        if (str)
        response.reason = str;

        #ifdef LOGGING
        Serial.printf("got status line: %s %d %s\n", response.version, response.status, response.reason);
        #endif

        // parse headers
        i = 0;
        while ((line = strtok_r(NULL, CRLF, &tmp)) != NULL && i < response.max_headers) {
            key = strtok(line, ":");
            val = strtok(NULL, CRLF);

            if (!key || !val) // skip empty keys or vals
                continue;

            response.headers[i].key = key;
            response.headers[i].val = val;

            if (strcasecmp(key, "Content-Length") == 0)
                content_len = atoi(val);

            if (*(response.headers[i].val) == ' ')
                response.headers[i].val++;

            #ifdef LOGGING
            Serial.printf("got header: %s: %s\n", response.headers[i].key, response.headers[i].val);
            #endif

            if (header_end && *header_end == '\0') // end of headers, stop
                break;

            i++;
        }

        // parse body
        if (header_end) {
            response.body = header_end+strlen(crlf);
            *(response.body+content_len) = '\0';

            #ifdef LOGGING
            Serial.printf("got body: %s\n", response.body);
            #endif
        }

        #ifdef LOGGING
        Serial.println("parsing response suceeded!");
        #endif

        return 0;
    }

    #ifdef LOGGING
    Serial.println("parsing response failed!");
    #endif

    return -1;
}

/*
 *  send request
 */
void HttpClient::request(http_request_t &request, http_response_t &response, http_header_t headers[], const char* method) {
    response.status = -1;
    bool connected = false;
    if (!request.hostname)
        return;

    #ifdef LOGGING
    Serial.printf("connecting to: '%s:%d'...\n", request.hostname, request.port);
    #endif

    int port = (request.port) ? request.port : 80;
    connected = client.connect(request.hostname, port);

    #ifdef LOGGING
    if (connected)
        Serial.println("connected!");
    else
        Serial.println("connection failed!");
    #endif

    if (!connected) {
        client.stop();
        return;
    }

    // send status line
    #ifdef LOGGING
    Serial.printf("sending: '%s %s HTTP/1.0'\n", method, request.path);
    #endif

    client.print(method);
    client.print(" ");
    client.print(request.path);
    client.print(" HTTP/1.0" CRLF);

    // send headers
    sendHeader("Connection", "close"); // not supporting keep-alive for now
    // Host: <hostname>:<port>
    #ifdef LOGGING
    Serial.printf("sending header: 'Host: %s:%d'\n", request.hostname, port);
    #endif
    client.print("Host"); client.print(": ");
    client.print(request.hostname); client.print(":"); client.print(port);
    client.print(CRLF);

    // TODO: is content-length a valid header for non-POST request?
    if (request.body) {
        sendHeader("Content-Length", (strlen(request.body)));
    } else if (strcmp(method, HTTP_METHOD_POST) == 0) { //Check to see if its a Post method.
        sendHeader("Content-Length", 0);
    }

    if (headers) {
        int i = 0;
        while (headers[i].key) {
            if (headers[i].val)
                sendHeader(headers[i].key, headers[i].val);
            i++;
        }
    }

    #ifdef LOGGING
    Serial.printf("done sending headers, sending final CRLF\n");
    #endif

    client.print(CRLF); // last CRLF
    client.flush();

    if (request.body) {
        #ifdef LOGGING
        Serial.printf("sending body: '%s'\n", request.body);
        #endif

        client.print(request.body);
    }

    #ifdef LOGGING
    Serial.printf("body sent, clearing response\n");
    #endif

    // clear response buffer
    clear_response(response);

    //
    // Receive HTTP Response
    //
    // The first value of client.available() might not represent the
    // whole response, so after the first chunk of data is received instead
    // of terminating the connection there is a delay and another attempt
    // to read data.
    // The loop exits when the connection is closed, or if there is a
    // timeout or an error.

    int i = 0;
    unsigned long lastRead = millis();
    unsigned long firstRead = millis();
    bool error = false;
    bool timeout = false;

    do {
        #ifdef LOGGING
        int bytes = client.available();
        if (bytes)
            Serial.printf("receiving %d bytes\n", bytes);
        #endif

        while (client.available()) {
            char c = client.read();
            #ifdef LOGGING
            Serial.print(c);
            #endif
            lastRead = millis();

            if (c == -1) {
                error = true;

                #ifdef LOGGING
                Serial.println("error: No data available");
                #endif

                break;
            }

            // Check that received character fits in buffer before storing.
            if (i < response.max_buffer_size-1) {
                response.buffer[i] = c;
            } else if (i == response.max_buffer_size-1) {
                response.buffer[i] = '\0'; // terminate
                client.stop();
                error = true;

                #ifdef LOGGING
                Serial.println("error: response body larger than buffer");
                #endif
            }
            i++;
        }
        response.buffer[i] = '\0'; // terminate

        timeout = millis() - lastRead > TIMEOUT;

        // Unless there has been an error or timeout wait 200ms to allow server
        // to respond or close connection.
        if (!error && !timeout) {
            delay(SERVER_RESPOND_WAIT);
        }
    } while (client.connected() && !timeout && !error);

    #ifdef LOGGING
    if (timeout) {
        Serial.println("error: timeout while reading response");
    }
    Serial.printf("\ntime to recieve response: %dms\n", millis()-firstRead);
    #endif
    client.stop();

    parse_response(response);
}
