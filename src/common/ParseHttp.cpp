#include "ParseHttp.h"
#include "json.hpp"
#include "LogM.h"

using namespace std;
/*
    POST /order/create HTTP/1.1\r\n
    Host: api.example.com\r\n
    Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9\r\n
    Content-Type: application/json\r\n
    Content-Length: 27\r\n
    \r\n
    {"goods_id": 123, "count": 2}
*/
HttpRequest::HttpRequest(const string strIn)
{
    size_t pos = strIn.find("\r\n\r\n");
    if (pos == string::npos) {
        LOG_INFO("Invalid HTTP request format");
        return;
    }

    string headerPart = strIn.substr(0, pos);
    body = strIn.substr(pos + 4);

    // 解析请求行
    size_t method_end = headerPart.find(' ');
    if (method_end != string::npos) {
        method = headerPart.substr(0, method_end);
        size_t path_end = headerPart.find(' ', method_end + 1);
        if (path_end != string::npos) {
            path = headerPart.substr(method_end + 1, path_end - method_end - 1);
        }
    }

    // 解析头部
    istringstream headerStream(headerPart);
    string line;
    while (getline(headerStream, line) && line != "\r") {
        size_t colon_pos = line.find(':');
        if (colon_pos != string::npos) {
            string key = line.substr(0, colon_pos);
            string value = line.substr(colon_pos + 1);
            headers[key] = value;
        }
    }
    // 懒加载---请求体默认不解析
}

string HttpRequest::getHeader(const string& key) const {
    auto it = headers.find(key);
    if (it != headers.end()) {
        return it->second;
    }
    return "";
}

string HttpRequest::getBodyParam(const string& key)
{
    if (!body_parsed) {
        ParseBody();
    }
    
    if (body_type == BodyType::JSON) {
        if (json_body.contains(key)) {
            auto& value = json_body[key];
            return value.is_string() ? value.get<string>() : value.dump();
        }
    } else if (body_type == BodyType::FORM) {
        auto it = form_body.find(key);
        if (it != form_body.end()) {
            return it->second;
        }
    }
    
    return "";
}

const nlohmann::json& HttpRequest::getJson()
{
    if (!body_parsed) {
        ParseBody();
    }
    return json_body;
}

//==================================Private methods============================================//

string HttpRequest::Trim(const string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == string::npos) {
        return "";
    }
    return s.substr(start, end - start + 1);
}

/*
    编码形式	解码结果	    说明
    %20	        空格	    十六进制编码
    +	        空格	    表单特有
    %2B	        +	        真正的加号
    %26	        &	        与号
    %3D	        =	        等号
*/
string HttpRequest::UrlDecode(const string& str)
{
    string result;
    result.reserve(str.size());
    
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int hex = 0;
            if (sscanf(str.substr(i + 1, 2).c_str(), "%x", &hex) == 1) {
                result += static_cast<char>(hex);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

void HttpRequest::ParseJsonBody()
{
    try {
        json_body = nlohmann::json::parse(body);
        body_type = BodyType::JSON;
    }
    catch (const exception& e) {
        LOG_ERROR("Failed to parse JSON body: {}", e.what());
        body_type = BodyType::NONE;
    }
}

void HttpRequest::ParseFormBody()
{
    string data = body;
    
    while (!data.empty()) {
        size_t ampPos = data.find('&');
        string pair = (ampPos != string::npos) 
            ? data.substr(0, ampPos) 
            : data;
        
        data = (ampPos != string::npos) 
            ? data.substr(ampPos + 1) 
            : "";
        
        if (pair.empty()) {
            continue;
        }
        
        size_t eqPos = pair.find('=');
        string key = (eqPos != string::npos) 
            ? Trim(pair.substr(0, eqPos)) 
            : Trim(pair);
        string value = (eqPos != string::npos) 
            ? Trim(pair.substr(eqPos + 1)) 
            : "";
        
        if (!key.empty()) {
            form_body[UrlDecode(key)] = UrlDecode(value);
        }
    }
    
    body_type = BodyType::FORM;
}


void HttpRequest::ParseBody()
{
    if (body.empty()) {
        body_type = BodyType::NONE;
        body_parsed = true;
        return;
    }

    string contentType = Trim(headers["Content-Type"]);

    if (contentType.find("application/json") != string::npos) {
        ParseJsonBody();
    } else {
        ParseFormBody();
    }

    body_parsed = true;
}