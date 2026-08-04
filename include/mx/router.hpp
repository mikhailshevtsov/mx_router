#ifndef MX_ROUTER_HPP
#define MX_ROUTER_HPP

#include <string_view>
#include <string>
#include <unordered_map>
#include <memory>
#include <regex>

namespace mx
{

template <typename HandlerT>
class router
{
public:
    using handler = HandlerT;

    struct segment
    {
        enum class type { literal, parameter, wildcard };
        type type;
        std::string text;
        std::string param;
        std::string regex;

        segment();
        explicit segment(std::string_view text);
        segment(const char* data, std::size_t size);
    };

private:
    struct node
    {
        segment seg;
        std::unordered_map<std::string, handler> handlers{};
        std::unordered_map<std::string, std::shared_ptr<node>> children;

        node() = default;
        explicit node(std::string_view text) : seg(text) {}

        bool add(std::string_view method, std::string_view path, const handler& h);
        bool remove(std::string_view method, std::string_view path);
    
        template <typename OnPathParamT = void(*)(std::string_view, std::string_view)>
        handler route(std::string_view method, std::string_view path, const OnPathParamT& on_path_param = [](std::string_view, std::string_view){});

        bool contains(std::string_view method, std::string_view path) const;

        bool mount(std::string_view path, router r);
        router unmount(std::string_view path);
    };

    friend struct node;

    router(std::shared_ptr<node> _node);

    static const char* find_first_of(const char* data, const char* chars);

public:
    router();

    bool add(std::string_view method, std::string_view path, const handler& h);
    bool remove(std::string_view method, std::string_view path);

    template <typename OnPathParamT = void(*)(const std::string&, const std::string&)>
    handler route(std::string_view method, std::string_view path, const OnPathParamT& on_path_param = [](const std::string&, const std::string&){}) const;

    bool contains(std::string_view method, std::string_view path) const;

    bool mount(std::string_view path, const router& r);
    router unmount(std::string_view path);

    void clear();
    bool empty() const ;

    operator bool() const ;

public:
    void get(std::string_view path, const handler& h);
    void post(std::string_view path, const handler& h);
    void put(std::string_view path, const handler& h);
    void patch(std::string_view path, const handler& h);
    void head(std::string_view path, const handler& h);

private:
    std::shared_ptr<node> _node;
    std::unordered_map<std::string, handler> _fallbacks{};
};


template <typename HandlerT>
router<HandlerT>::segment::segment() = default;

template <typename HandlerT>
router<HandlerT>::segment::segment(std::string_view text)
    : text{text}
{
    if (text.size() >= 1 && text.front() == '{' && text.back() == '}')
    {
        type = type::parameter;
        const char* begin = text.data() + 1;
        const char* end = find_first_of(begin, ":}");
        if (end > begin)
            param = std::string(begin, end - begin);
        if (*end == ':')
        {
            begin = end + 1;
            end = find_first_of(begin, "}");
            if (end > begin)
                regex = std::string(begin, end - begin);
        }
    }
    else if (text.size() >= 1 && text.front() == '*')
    {
        type = type::wildcard;
        param = text.data() + 1;
    }
    else if (!text.empty())
        type = type::literal;
}

template <typename HandlerT>
router<HandlerT>::segment::segment(const char* data, std::size_t size)
    : segment(std::string_view(data, size))
{}

template <typename HandlerT>
router<HandlerT>::router()  = default;

template <typename HandlerT>
bool router<HandlerT>::node::add(std::string_view method, std::string_view path, const handler& h)
{
    const char* begin = path.data();
    if (*begin == 0 || *begin == '?' || path == "/")
    {
        handlers[method.data()] = h;
        return true;
    }
    
    const char* end = find_first_of(++begin, "/?");
    if (begin >= end)
        return true;
    std::string key(begin, end - begin);

    auto it = children.find(key);
    if (it == std::end(children))
        it = children.emplace(key, std::make_shared<node>(key)).first;

    return it->second->add(method, end, h);
}

template <typename HandlerT>
bool router<HandlerT>::node::remove(std::string_view method, std::string_view path)
{
    const char* begin = path.data();
    if (*begin == 0 || *begin == '?' || path == "/")
        return handlers.erase(method.data());

    const char* end = find_first_of(++begin, "/?");
    if (begin >= end)
        return false;
    std::string key(begin, end - begin);

    auto it = children.find(key);
    if (it == std::end(children))
        return false;

    return it->second->remove(method, end);
}

template <typename HandlerT>
template <typename OnPathParamT>
typename router<HandlerT>::handler router<HandlerT>::node::route(std::string_view method, std::string_view path, const OnPathParamT& on_path_param)
{
    const char* begin = path.data();
    if (*begin == 0 || *begin == '?' || path == "/" || seg.type == segment::type::wildcard)
    {
        auto it = handlers.find(method.data());
        if (it != std::end(handlers))
            return it->second;
        return {};
    }

    const char* end = find_first_of(++begin, "/?");
    if (begin >= end)
        return {};
    std::string key(begin, end - begin);

    auto it = children.find(key);

    std::vector<decltype(it)> param_children;
    if (it == std::end(children))
    {
        // search for parameterized children
        for (auto it = std::begin(children); it != std::end(children); ++it)
        {
            const auto& seg = it->second->seg;
            if (seg.type == segment::type::parameter &&
               (seg.regex.empty() || std::regex_match(key, std::regex(seg.regex.data()))
            ))
                param_children.push_back(it);
            if (param_children.size() > 1)
                break;
        }
        if (param_children.size() == 1)
        {
            const auto& seg = param_children[0]->second->seg;
            if (!seg.param.empty())
                on_path_param(seg.param, key);
            it = param_children[0];
        }
        else if (param_children.size() > 1)
            return {};
    }

    if (it == std::end(children))
    {
        // search for wildcard children
        for (auto it = std::begin(children); it != std::end(children); ++it)
        {
            const auto& seg = it->second->seg;
            if (seg.type == segment::type::wildcard)
                param_children.push_back(it);
            if (param_children.size() > 1)
                break;
        }
        if (param_children.size() == 1)
        {
            const auto& seg = param_children[0]->second->seg;
            const char* end = find_first_of(begin, "?");
            if (!seg.param.empty())
                on_path_param(seg.param, std::string(begin, end - begin));
            it = param_children[0];
        }
        else
            return {};
    }

    return it->second->route(method, end, on_path_param);
}


template <typename HandlerT>
bool router<HandlerT>::node::contains(std::string_view method, std::string_view path) const
{
    const char* begin = path.data();
    if (*begin == 0 || *begin == '?' || path == "/")
        return handlers.find(method.data()) != std::end(handlers);

    const char* end = find_first_of(++begin, "/?");
    if (begin >= end)
        return false;
    std::string seg(begin, end - begin);

    auto it = children.find(seg);
    if (it == std::end(children))
        return false;

    return it->second->contains(method, end);
}

template <typename HandlerT>
bool router<HandlerT>::node::mount(std::string_view path, router r)
{
    const char* begin = path.data();
    if (*begin == 0 || *begin == '?' || path == "/")
        return false;

    const char* end = find_first_of(++begin, "/?");
    if (begin >= end)
        return false;
    std::string seg(begin, end - begin);

    auto it = children.find(seg);
    if (it == std::end(children))
    {
        if (*end == 0 || *end == '?')
        {
            children[seg] = r._node;
            return true;
        }
        else
            children[seg] = std::make_shared<node>();
    }

    return it->second->mount(end, r);
}

template <typename HandlerT>
router<HandlerT> router<HandlerT>::node::unmount(std::string_view path)
{
    const char* begin = path.data();
    if (*begin == 0 || *begin == '?' || path == "/")
        return {};

    const char* end = find_first_of(++begin, "/?");
    if (begin >= end)
        return {};
    std::string seg(begin, end - begin);

    auto it = children.find(seg);
    if (it == std::end(children))
        return {};
    
    if (*end == 0 || *end == '?')
    {
        auto r = router(std::move(it->second));
        children.erase(it);
        return r;
    }

    return it->second->unmount(end);
}

template <typename HandlerT>
router<HandlerT>::router(std::shared_ptr<node> _node) : _node{_node} {}

template <typename HandlerT>
const char* router<HandlerT>::find_first_of(const char* data, const char* chars)
{
    for (; *data; ++data)
        for (const char* ch = chars; *ch; ++ch)
            if (*data == *ch)
                return data;
    return data;
}

template <typename HandlerT>
bool router<HandlerT>::add(std::string_view method, std::string_view path, const handler& h)
{
    if (path.empty() || path[0] != '/')
        return false;
    
    if (empty())
        _node = std::make_shared<node>();

    return _node->add(method, path, h);
}

template <typename HandlerT>
bool router<HandlerT>::remove(std::string_view method, std::string_view path)
{
    if (empty() || path.empty() || path[0] != '/')
        return false;
    
    return _node->remove(method, path);
}

template <typename HandlerT>
template <typename OnPathParamT>
typename router<HandlerT>::handler router<HandlerT>::route(std::string_view method, std::string_view path, const OnPathParamT& on_path_param) const
{
    if (empty() || path.empty() || path[0] != '/')
        return {};

    return _node->route(method, path, on_path_param);
}

template <typename HandlerT>
bool router<HandlerT>::contains(std::string_view method, std::string_view path) const
{
    if (empty() || path.empty() || path[0] != '/')
        return false;

    return _node->contains(method, path);
}

template <typename HandlerT>
bool router<HandlerT>::mount(std::string_view path, const router& r)
{
    if (path.empty() || path[0] != '/')
        return false;

    if (path == "/")
    {
        _node = r._node;
        return true;
    }

    if (empty())
        _node = std::make_shared<node>();
        
    return _node->mount(path, r);
}

template <typename HandlerT>
router<HandlerT> router<HandlerT>::unmount(std::string_view path)
{
    if (empty() || path.empty() || path[0] != '/')
        return {};

    if (path == "/")
        return router(std::move(_node));

    return _node->unmount(path);
}

template <typename HandlerT>
void router<HandlerT>::clear()
{
    _node = {};
}

template <typename HandlerT>
bool router<HandlerT>::empty() const 
{
    return !_node;
}

template <typename HandlerT>
router<HandlerT>::operator bool() const 
{
    return !empty();
}

template <typename HandlerT>
void router<HandlerT>::get(std::string_view path, const handler& h)
{
    add("GET", path, h);
}

template <typename HandlerT>
void router<HandlerT>::post(std::string_view path, const handler& h)
{
    add("POST", path, h);
}

template <typename HandlerT>
void router<HandlerT>::put(std::string_view path, const handler& h)
{
    add("PUT", path, h);
}

template <typename HandlerT>
void router<HandlerT>::patch(std::string_view path, const handler& h)
{
    add("PATCH", path, h);
}

template <typename HandlerT>
void router<HandlerT>::head(std::string_view path, const handler& h)
{
    add("HEAD", path, h);
}

}

#endif //MX_ROUTER_HPP