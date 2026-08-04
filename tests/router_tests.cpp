#include <string>

#include <mx/router.hpp>

#include <gtest/gtest.h>

using namespace mx;

TEST(RouterTest, AddRootHandler)
{
    router<std::string> r;

    r.add("GET", "/", "handler");

    auto handler = r.route("GET", "/");

    EXPECT_EQ(handler, "handler");
}



TEST(RouterTest, AddHandler)
{
    router<std::string> r;

    r.add("GET", "/home", "handler");

    auto handler = r.route("GET", "/home");

    EXPECT_EQ(handler, "handler");
}



TEST(RouterTest, RemoveRootHandler)
{
    router<std::string> r;

    r.add("GET", "/", "handler");
    r.remove("GET", "/");

    auto handler = r.route("GET", "/");

    EXPECT_EQ(handler, "");
}



TEST(RouterTest, RemoveHandler)
{
    router<std::string> r;

    r.add("GET", "/home", "handler");
    r.remove("GET", "/home");

    auto handler = r.route("GET", "/home");

    EXPECT_EQ(handler, "");
}



TEST(RouterTest, ContainsRootHandler)
{
    router<std::string> r;

    r.add("GET", "/", "handler");

    EXPECT_TRUE(r.contains("GET", "/"));

    r.remove("GET", "/");

    EXPECT_FALSE(r.contains("GET", "/"));
}



TEST(RouterTest, ContainsHandler)
{
    router<std::string> r;

    r.add("GET", "/home", "handler");

    EXPECT_TRUE(r.contains("GET", "/home"));

    r.remove("GET", "/home");

    EXPECT_FALSE(r.contains("GET", "/home"));
}



TEST(RouterTest, MountRoot)
{
    router<std::string> r;

    router<std::string> m;
    
    m.add("GET", "/home", "handler");

    auto result = r.mount("/", m);
    auto handler = r.route("GET", "/home");

    EXPECT_TRUE(result);
    EXPECT_EQ(handler, "handler");
}




TEST(RouterTest, Mount)
{
    router<std::string> r;

    r.add("GET", "/home", "handler");

    router<std::string> m;
    
    m.add("GET", "/", "auth");

    auto result = r.mount("/home/auth", m);
    auto handler = r.route("GET", "/home/auth");

    EXPECT_TRUE(result);
    EXPECT_EQ(handler, "auth");
}



TEST(RouterTest, UnmountRoot)
{
    router<std::string> r;

    r.add("GET", "/", "handler");

    auto m = r.unmount("/");

    std::string handler;

    handler = r.route("GET", "/");
    EXPECT_EQ(handler, "");

    handler = m.route("GET", "/");
    EXPECT_EQ(handler, "handler");
}




TEST(RouterTest, Unmount)
{
    router<std::string> r;

    r.add("GET", "/home", "handler");

    auto m = r.unmount("/home");

    std::string handler;

    handler = r.route("GET", "/home");
    EXPECT_EQ(handler, "");

    handler = m.route("GET", "/");
    EXPECT_EQ(handler, "handler");
}



TEST(RouterTest, PathParams)
{
    router<std::string> r;

    r.add("GET", "/home/{user_id}/smth", "handler");

    std::string handler;

    std::pair<std::string, std::string> user_id;
    auto on_path_param = [&user_id](const std::string& key, const std::string& value)
    {
        user_id = {key, value};
    };
    handler = r.route("GET", "/home/42/smth", on_path_param);
    EXPECT_EQ(handler, "handler");
    EXPECT_EQ(user_id.first, "user_id");
    EXPECT_EQ(user_id.second, "42");
}



TEST(RouterTest, ManyPathParams)
{
    router<std::string> r;

    r.add("GET", "/a/b/c/{param1}/d/{param2}", "handler");

    std::string handler;

    std::unordered_map<std::string, std::string> path_params;

    auto on_path_param = [&path_params](const std::string& key, const std::string& value)
    {
        path_params[key] = value;
    };
    handler = r.route("GET", "/a/b/c/37/d/42", on_path_param);
    EXPECT_EQ(handler, "handler");
    EXPECT_EQ(path_params.size(), 2);
    EXPECT_EQ(path_params["param1"], "37");
    EXPECT_EQ(path_params["param2"], "42");
}



TEST(RouterTest, Wildcard)
{
    router<std::string> r;

    r.add("GET", "/home/*wildcard?", "handler");

    std::string handler;

    std::pair<std::string, std::string> wildcard;
    auto on_path_param = [&wildcard](const std::string& key, const std::string& value)
    {
        wildcard = {key, value};
    };
    handler = r.route("GET", "/home/long/path/to/smth", on_path_param);
    EXPECT_EQ(handler, "handler");
    EXPECT_EQ(wildcard.first, "wildcard");
    EXPECT_EQ(wildcard.second, "long/path/to/smth");
}



TEST(RouterTest, WildcardRoot)
{
    router<std::string> r;

    r.add("GET", "/*wildcard", "handler");

    std::string handler;

    std::pair<std::string, std::string> wildcard;
    auto on_path_param = [&wildcard](const std::string& key, const std::string& value)
    {
        wildcard = {key, value};
    };
    handler = r.route("GET", "/home/long/path/to/smth", on_path_param);
    EXPECT_EQ(handler, "handler");
    EXPECT_EQ(wildcard.first, "wildcard");
    EXPECT_EQ(wildcard.second, "home/long/path/to/smth");
}



