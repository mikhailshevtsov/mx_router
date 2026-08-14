#include <string>

#include <mx/router.hpp>

#include <gtest/gtest.h>

using namespace mx;

TEST(RouterTest, AddRootHandler)
{
    router<std::string> r;

    r.at("/") = "handler";

    std::string handler;
    handler = *r.at("/");

    EXPECT_EQ(handler, "handler");
}



TEST(RouterTest, AddHandler)
{
    router<std::string> r;

    r.at("/home") = "handler";

    std::string handler;
    handler = *r.at("/home");

    EXPECT_EQ(handler, "handler");
}

TEST(RouterTest, ContainsRootHandler)
{
    router<std::string> r;

    r.at("/") = "handler";

    EXPECT_TRUE(r.contains("/"));

    r.unmount("/");

    EXPECT_FALSE(r.contains("/"));
}



TEST(RouterTest, ContainsHandler)
{
    router<std::string> r;

    r.at("/home") = "handler";

    EXPECT_TRUE(r.contains("/home"));

    r.unmount("/home");

    EXPECT_FALSE(r.contains("/home"));
}



TEST(RouterTest, MountRoot)
{
    router<std::string> r;
    router<std::string> m;
    
    m.at("/home") = "handler";

    auto res = r.mount("/", m);

    std::string handler;
    handler = *r.at("/home");

    EXPECT_EQ(handler, "handler");
}




TEST(RouterTest, Mount)
{
    router<std::string> r;

    r.at("/home") = "handler";

    router<std::string> m;
    
    m.at("/") = "auth";

    auto res = r.mount("/home/auth", m);

    std::string handler;
    handler = *r.at("/home/auth");

    EXPECT_EQ(handler, "auth");
}



TEST(RouterTest, UnmountRoot)
{
    router<std::string> r;

    r.at("/") = "handler";

    router<std::string> m = r.unmount("/");

    std::string handler;

    EXPECT_FALSE(r.contains("/"));

    handler = *m.at("/");
    EXPECT_EQ(handler, "handler");
}



TEST(RouterTest, Unmount)
{
    router<std::string> r;

    r.at("/home") = "handler";

    router<std::string> m = r.unmount("/home");

    std::string handler;

    EXPECT_FALSE(r.contains("/home"));

    handler = *m.at("/");
    EXPECT_EQ(handler, "handler");
}


TEST(RouterTest, UseCase)
{
    router<std::string> api;

    api.at("/service/auth/test") = "TEST";
    api.at("/service/auth") = "ROOT";

    router<std::string> auth;

    auth.at("/") = "AUTH";
    auth.at("/login") = "AUTH_LOGIN";
    auth.at("/register") = "AUTH_REGISTER";

    router<std::string> r = api.mount("/service/auth", auth);

    std::string handler;

    handler = "";
    handler = *api.at("/service/auth");
    EXPECT_EQ(handler, "AUTH");

    handler = "";
    handler = *api.at("/service/auth/login");
    EXPECT_EQ(handler, "AUTH_LOGIN");

    handler = "";
    handler = *api.at("/service/auth/register");
    EXPECT_EQ(handler, "AUTH_REGISTER");

    handler = "";
    handler = *r.at("/");
    EXPECT_EQ(handler, "ROOT");

    handler = "";
    handler = *r.at("/test");
    EXPECT_EQ(handler, "TEST");
}

TEST(RouterTest, PathParams)
{
    router<std::string> r;

    r.at("/home/:user_id/smth") = "handler";

    std::string handler;

    std::pair<std::string, std::string> user_id;
    auto on_path_param = [&user_id](std::string_view key, std::string_view value)
    {
        user_id = {std::string(key), std::string(value)};
    };
    handler = *r.at("/home/42/smth", on_path_param);
    EXPECT_EQ(handler, "handler");
    EXPECT_EQ(user_id.first, "user_id");
    EXPECT_EQ(user_id.second, "42");
}



TEST(RouterTest, ManyPathParams)
{
    router<std::string> r;

    r.at("/a/b/c/:param1/d/:param2") = "handler";

    std::string handler;

    std::unordered_map<std::string, std::string> path_params;

    auto on_path_param = [&path_params](std::string_view key, std::string_view value)
    {
        path_params[std::string(key)] = std::string(value);
    };
    
    handler = *r.at("/a/b/c/37/d/42", on_path_param);
    EXPECT_EQ(handler, "handler");
    EXPECT_EQ(path_params.size(), 2);
    EXPECT_EQ(path_params["param1"], "37");
    EXPECT_EQ(path_params["param2"], "42");
}

TEST(RouterTest, Ambiguous)
{
    router<std::string> r;

    r.at("/hello/:a/world") = "handler1";
    r.at("/hello/:b/world/:c") = "handler2";
    r.at("/hello/:d/world/:e/:f") = "handler3";

    std::string handler;

    std::unordered_map<std::string, std::string> path_params;

    auto on_path_param = [&path_params](std::string_view key, std::string_view value)
    {
        path_params[std::string(key)] = std::string(value);
    };
    
    handler = *r.at("/hello/100/world", on_path_param);
    EXPECT_EQ(handler, "handler1");
    EXPECT_EQ(path_params.size(), 1);
    EXPECT_EQ(path_params["a"], "100");

    handler = "";
    path_params.clear();
    handler = *r.at("/hello/200/world/300", on_path_param);
    EXPECT_EQ(handler, "handler2");
    EXPECT_EQ(path_params.size(), 2);
    EXPECT_EQ(path_params["b"], "200");
    EXPECT_EQ(path_params["c"], "300");


    handler = "";
    path_params.clear();
    handler = *r.at("/hello/400/world/500/600", on_path_param);
    EXPECT_EQ(handler, "handler3");
    EXPECT_EQ(path_params.size(), 3);
    EXPECT_EQ(path_params["d"], "400");
    EXPECT_EQ(path_params["e"], "500");
    EXPECT_EQ(path_params["f"], "600");
}



TEST(RouterTest, Wildcard)
{
    router<std::string> r;

    r.at("/home/*wildcard?") = "handler";

    std::string handler;

    std::pair<std::string, std::string> wildcard;
    auto on_path_param = [&wildcard](std::string_view key, std::string_view value)
    {
        wildcard = {std::string(key), std::string(value)};
    };
    handler = *r.at("/home/long/path/to/smth", on_path_param);
    EXPECT_EQ(handler, "handler");
    EXPECT_EQ(wildcard.first, "wildcard");
    EXPECT_EQ(wildcard.second, "long/path/to/smth");
}



TEST(RouterTest, WildcardRoot)
{
    router<std::string> r;

    r.at("/*wildcard") = "handler";

    std::string handler;

    std::pair<std::string, std::string> wildcard;
    auto on_path_param = [&wildcard](std::string_view key, std::string_view value)
    {
        wildcard = {std::string(key), std::string(value)};
    };
    handler = *r.at("/home/long/path/to/smth", on_path_param);
    EXPECT_EQ(handler, "handler");
    EXPECT_EQ(wildcard.first, "wildcard");
    EXPECT_EQ(wildcard.second, "home/long/path/to/smth");
}

TEST(RouterTest, WildcardBetween)
{
    router<std::string> r;

    r.at("/hello/world") = "Hello, World!";
    r.at("/hello/*wildcard") = "handler";

    std::string handler;

    std::pair<std::string, std::string> wildcard;
    auto on_path_param = [&wildcard](std::string_view key, std::string_view value)
    {
        wildcard = {std::string(key), std::string(value)};
    };
    handler = *r.at("/hello/world/long/path/to/smth", on_path_param);
    EXPECT_EQ(handler, "handler");
    EXPECT_EQ(wildcard.first, "wildcard");
    EXPECT_EQ(wildcard.second, "world/long/path/to/smth");
}



