#include <gtest/gtest.h>
#include <string>
#include "ParseHttp.h"

using namespace std;

class HttpRequestTest : public ::testing::Test {
protected:
    // 可在这里放置通用初始化数据
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(HttpRequestTest, ParseJsonRequest) {
    const string req =
        "POST /order/create HTTP/1.1\r\n"
        "Host: api.example.com\r\n"
        "Authorization: Bearer token123\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "{\"goods_id\": 123, \"count\": 2}";

    HttpRequest http(req);

    EXPECT_EQ(http.method, "POST");
    EXPECT_EQ(http.path, "/order/create");

    EXPECT_EQ(http.getHeader("Host"), " api.example.com");
    EXPECT_EQ(http.getHeader("Authorization"), " Bearer token123");
    EXPECT_EQ(http.getHeader("Content-Type"), " application/json");

    EXPECT_EQ(http.getBodyParam("goods_id"), "123");
    EXPECT_EQ(http.getBodyParam("count"), "2");

    const auto& j = http.getJson();
    EXPECT_TRUE(j.contains("goods_id"));
    EXPECT_TRUE(j.contains("count"));
    EXPECT_EQ(j["goods_id"].get<int>(), 123);
    EXPECT_EQ(j["count"].get<int>(), 2);
}

TEST_F(HttpRequestTest, ParseFormRequest) {
    const string req =
        "POST /submit HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 43\r\n"
        "\r\n"
        "a=1+b&name=John+Doe&plus=%2B&space=%20";

    HttpRequest http(req);

    EXPECT_EQ(http.method, "POST");
    EXPECT_EQ(http.path, "/submit");

    EXPECT_EQ(http.getHeader("Content-Type"), " application/x-www-form-urlencoded");

    EXPECT_EQ(http.getBodyParam("a"), "1 b");
    EXPECT_EQ(http.getBodyParam("name"), "John Doe");
    EXPECT_EQ(http.getBodyParam("plus"), "+");
    EXPECT_EQ(http.getBodyParam("space"), " ");

    const auto& j = http.getJson();
    EXPECT_TRUE(j.is_null() || j.empty());
}
