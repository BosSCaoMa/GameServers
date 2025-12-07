#ifndef RSP_TO_FE_H
#define RSP_TO_FE_H

class HttpResponse {
public:
    int status_code;
    std::string status_message;
    std::map<std::string, std::string> headers;
    std::string body;
};

void sendResponse(int client_fd, int statusCode, const std::string& body) {
        const char* statusText = (statusCode == 200 ? "OK" : (statusCode == 400 ? "Bad Request" : (statusCode == 401 ? "Unauthorized" : "Error")));
        std::string resp =
            "HTTP/1.1 " + std::to_string(statusCode) + " " + statusText + "\r\n" +
            "Content-Type: application/json; charset=utf-8\r\n" +
            "Content-Length: " + std::to_string(body.size()) + "\r\n" +
            "Connection: close\r\n" +
            "\r\n" +
            body;
        ::write(client_fd, resp.data(), resp.size());
    };
#endif // RSP_TO_FE_H