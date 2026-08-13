#include <string>

#include <mx/router.hpp>

#include <gtest/gtest.h>

using namespace mx;

TEST(RouterTest, AddRootHandler)
{
    router<std::string> r;

    *r.insert("/") = "handler";

    std::string handler;
    handler = **r.find("/");

    EXPECT_EQ(handler, "handler");
}



TEST(RouterTest, AddHandler)
{
    router<std::string> r;

    *r.insert("/home") = "handler";

    std::string handler;
    handler = **r.find("/home");

    EXPECT_EQ(handler, "handler");
}

TEST(RouterTest, ContainsRootHandler)
{
    router<std::string> r;

    *r.insert("/") = "handler";

    EXPECT_TRUE(r.contains("/"));

    r.remove("/");

    EXPECT_FALSE(r.contains("/"));
}



TEST(RouterTest, ContainsHandler)
{
    router<std::string> r;

    *r.insert("/home") = "handler";

    EXPECT_TRUE(r.contains("/home"));

    r.remove("/home");

    EXPECT_FALSE(r.contains("/home"));
}



TEST(RouterTest, MountRoot)
{
    router<std::string> r;
    router<std::string> m;
    
    *m.insert("/home") = "handler";

    auto res = r.insert("/", m.root());

    std::string handler;
    handler = **r.find("/home");

    EXPECT_EQ(handler, "handler");
}




TEST(RouterTest, Mount)
{
    router<std::string> r;

    *r.insert("/home") = "handler";

    router<std::string> m;
    
    *m.insert("/") = "auth";

    auto res = r.insert("/home/auth", m.root());

    std::string handler;
    handler = **r.find("/home/auth");

    EXPECT_EQ(handler, "auth");
}



TEST(RouterTest, UnmountRoot)
{
    router<std::string> r;

    *r.insert("/") = "handler";

    router<std::string> m = r.remove("/");

    std::string handler;

    auto it = r.find("/");
    EXPECT_EQ(it, nullptr);

    handler = **m.find("/");
    EXPECT_EQ(handler, "handler");
}



TEST(RouterTest, Unmount)
{
    router<std::string> r;

    *r.insert("/home") = "handler";

    router<std::string> m = r.remove("/home");

    std::string handler;

    auto it = r.find("/home");
    EXPECT_EQ(it, nullptr);

    handler = **m.find("/");
    EXPECT_EQ(handler, "handler");
}


TEST(RouterTest, Goddamn)
{
    router<std::string> api;

    *api.insert("/service/auth/test") = "TEST";
    *api.insert("/service/auth") = "ROOT";

    router<std::string> auth;

    *auth.insert("/") = "AUTH";
    *auth.insert("/login") = "AUTH_LOGIN";
    *auth.insert("/register") = "AUTH_REGISTER";

    router<std::string> r = api.insert("/service/auth", auth.root());

    std::string handler;

    handler = "";
    handler = **api.find("/service/auth");
    EXPECT_EQ(handler, "AUTH");

    handler = "";
    handler = **api.find("/service/auth/login");
    EXPECT_EQ(handler, "AUTH_LOGIN");

    handler = "";
    handler = **api.find("/service/auth/register");
    EXPECT_EQ(handler, "AUTH_REGISTER");

    handler = "";
    handler = **r.find("/");
    EXPECT_EQ(handler, "ROOT");

    handler = "";
    handler = **r.find("/test");
    EXPECT_EQ(handler, "TEST");
}

TEST(RouterTest, PathParams)
{
    router<std::string> r;

    *r.insert("/home/:user_id/smth") = "handler";

    std::string handler;

    std::pair<std::string, std::string> user_id;
    auto on_path_param = [&user_id](std::string_view key, std::string_view value)
    {
        user_id = {std::string(key), std::string(value)};
    };
    handler = **r.find("/home/42/smth", on_path_param);
    EXPECT_EQ(handler, "handler");
    EXPECT_EQ(user_id.first, "user_id");
    EXPECT_EQ(user_id.second, "42");
}



TEST(RouterTest, ManyPathParams)
{
    router<std::string> r;

    *r.insert("/a/b/c/:param1/d/:param2") = "handler";

    std::string handler;

    std::unordered_map<std::string, std::string> path_params;

    auto on_path_param = [&path_params](std::string_view key, std::string_view value)
    {
        path_params[std::string(key)] = std::string(value);
    };
    
    handler = **r.find("/a/b/c/37/d/42", on_path_param);
    EXPECT_EQ(handler, "handler");
    EXPECT_EQ(path_params.size(), 2);
    EXPECT_EQ(path_params["param1"], "37");
    EXPECT_EQ(path_params["param2"], "42");
}



TEST(RouterTest, Wildcard)
{
    router<std::string> r;

    *r.insert("/home/*wildcard?") = "handler";

    std::string handler;

    std::pair<std::string, std::string> wildcard;
    auto on_path_param = [&wildcard](std::string_view key, std::string_view value)
    {
        wildcard = {std::string(key), std::string(value)};
    };
    handler = **r.find("/home/long/path/to/smth", on_path_param);
    EXPECT_EQ(handler, "handler");
    EXPECT_EQ(wildcard.first, "wildcard");
    EXPECT_EQ(wildcard.second, "long/path/to/smth");
}



TEST(RouterTest, WildcardRoot)
{
    router<std::string> r;

    *r.insert("/*wildcard") = "handler";

    std::string handler;

    std::pair<std::string, std::string> wildcard;
    auto on_path_param = [&wildcard](std::string_view key, std::string_view value)
    {
        wildcard = {std::string(key), std::string(value)};
    };
    handler = **r.find("/home/long/path/to/smth", on_path_param);
    EXPECT_EQ(handler, "handler");
    EXPECT_EQ(wildcard.first, "wildcard");
    EXPECT_EQ(wildcard.second, "home/long/path/to/smth");
}

TEST(RouterTest, WildcardBetween)
{
    router<std::string> r;

    *r.insert("/hello/world") = "Hello, World!";
    *r.insert("/hello/*wildcard") = "handler";

    std::string handler;

    std::pair<std::string, std::string> wildcard;
    auto on_path_param = [&wildcard](std::string_view key, std::string_view value)
    {
        wildcard = {std::string(key), std::string(value)};
    };
    handler = **r.find("/hello/world/long/path/to/smth", on_path_param);
    EXPECT_EQ(handler, "handler");
    EXPECT_EQ(wildcard.first, "wildcard");
    EXPECT_EQ(wildcard.second, "world/long/path/to/smth");
}



