#ifndef PARSE_HTTP_H
#define PARSE_HTTP_H

#include <string>
#include <unordered_map>
#include "json.hpp"
enum class BodyType {
    NONE,
    JSON,
    FORM
};
class HttpRequest {
public:
    HttpRequest(const std::string strIn);
    ~HttpRequest() = default;

    bool isValid() const { return !method.empty() && !path.empty(); }
    std::string getMethod() const { return method; }
    std::string getPath() const { return path; }
    std::string getHeader(const std::string& key) const;
    
    std::string getParam(const std::string& key);
    const nlohmann::json& getJson();
private:
    bool body_parsed = false;
    std::string method;
    std::string path;
    std::unordered_map<std::string, std::string> headers;
    
    BodyType body_type = BodyType::NONE;
    std::string body;
    nlohmann::json json_body;
    std::unordered_map<std::string, std::string> form_body;

    void ParseBody();
    void ParseJsonBody();
    void ParseFormBody();
    static std::string UrlDecode(const std::string& str);
    static std::string Trim(const std::string& s);
};
#endif // PARSE_HTTP_H