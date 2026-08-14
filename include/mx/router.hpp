#ifndef MX_ROUTER_HPP
#define MX_ROUTER_HPP

#include <string_view>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>
#include <algorithm>

namespace mx
{

template <typename T>
class router
{
public:
    using value_t = T;
    using on_path_param_t = std::function<void(std::string_view, std::string_view)>;

private:
    struct node
    {
        std::optional<value_t> _value;
        std::unordered_map<std::string, std::shared_ptr<node>> _children;

        std::shared_ptr<node> at(std::string_view path);
        std::shared_ptr<node> at(std::string_view path, const on_path_param_t& on_path_param) const;

        std::shared_ptr<node> mount(std::string_view path, std::shared_ptr<node> other);
        std::shared_ptr<node> unmount(std::string_view path);
    };

    static const char* find_first_of(const char* data, const char* chars);

private:
    router(std::shared_ptr<node> root);

public:
    router();

    std::optional<value_t>& at(std::string_view path);
    const std::optional<value_t>& at(std::string_view path, const on_path_param_t& on_path_param = {}) const;

    router mount(std::string_view path, router other);
    router unmount(std::string_view path);

    bool contains(std::string_view path) const;

    void clear();
    bool empty() const;
    explicit operator bool() const;

private:
    std::shared_ptr<node> _root;
};

template <typename T>
std::shared_ptr<typename router<T>::node> router<T>::node::at(std::string_view path)
{
    const char* begin = path.size() > 0 ? path.data() + 1 : path.data();
    const char* end = find_first_of(begin, "/?");
    std::string next_word(begin, end - begin);

    auto it = _children.find(next_word);

    if (it == std::end(_children))
        it = _children.emplace(next_word, std::make_shared<node>()).first;

    if (*end == 0 || *end == '?')
        return it->second;

    return it->second->at(end);
}

template <typename T>
std::shared_ptr<typename router<T>::node> router<T>::node::at(std::string_view path, const on_path_param_t& on_path_param) const
{
    const char* begin = path.size() > 0 ? path.data() + 1 : path.data();
    const char* end = find_first_of(begin, "/?");
    std::string next_word(begin, end - begin);

    auto it = _children.find(next_word);
    
    auto find_wildcard = [&]()
    {
        auto it = std::find_if(std::begin(_children), std::end(_children), [](const auto& kv) { return !kv.first.empty() && kv.first[0] == '*'; });
        if (it != std::end(_children))
        {
            const char* end = find_first_of(begin, "?");
            std::string_view param(it->first.data() + 1, it->first.size() - 1);
            if (!param.empty() && on_path_param)
                on_path_param(param, std::string_view(begin, end - begin));
        }
        return it;
    };

    if (it == std::end(_children))
    {
        it = std::begin(_children);
        for (it = std::find_if(it, std::end(_children), [](const auto& kv) { return !kv.first.empty() && kv.first[0] == ':'; }); it != std::end(_children); ++it)
        {
            if (*end == 0 || *end == '?')
            {
                if (!it->second->_value)
                    return {};
                std::string_view param(it->first.data() + 1, it->first.size() - 1);
                if (!param.empty() && on_path_param)
                    on_path_param(param, next_word);
                return it->second;
            }
  
            auto _node = it->second->at(end, on_path_param);
            if (_node)
            {
                std::string_view param(it->first.data() + 1, it->first.size() - 1);
                if (!param.empty() && on_path_param)
                    on_path_param(param, next_word);
                return _node;
            }
        }
    }

    if (it == std::end(_children))
        it = find_wildcard();
    if (it == std::end(_children))
        return {};

    if (*end == 0 || *end == '?')
    {
        if (!it->second->_value)
            return {};
        return it->second;
    }

    auto _node = it->second->at(end, on_path_param);
    if (_node)
        return _node;

    it = find_wildcard();
    if (it == std::end(_children))
        return {};

    return it->second;
}

template <typename T>
std::shared_ptr<typename router<T>::node> router<T>::node::mount(std::string_view path, std::shared_ptr<node> other)
{
    const char* begin = path.size() > 0 ? path.data() + 1 : path.data();
    const char* end = find_first_of(begin, "/?");
    std::string next_word(begin, end - begin);

    auto it = _children.find(next_word);

    if (it == std::end(_children))
        it = _children.emplace(next_word, std::make_shared<node>()).first;
    
    if (*end == 0 || *end == '?')
        return std::exchange(it->second, other);

    return it->second->mount(end, other);
}

template <typename T>
std::shared_ptr<typename router<T>::node> router<T>::node::unmount(std::string_view path)
{
    const char* begin = path.size() > 0 ? path.data() + 1 : path.data();
    const char* end = find_first_of(begin, "/?");
    std::string next_word(begin, end - begin);

    auto it = _children.find(next_word);

    if (it == std::end(_children))
        return {};

    if (*end == 0 || *end == '?')
    {
        auto tmp = it->second;
        _children.erase(it);
        return tmp;
    }

    return it->second->unmount(end);
}

template <typename T>
const char* router<T>::find_first_of(const char* data, const char* chars)
{
    for (; *data; ++data)
        for (const char* ch = chars; *ch; ++ch)
            if (*data == *ch)
                return data;
    return data;
}

template <typename T>
router<T>::router(std::shared_ptr<node> root) : _root{root} {}

template <typename T>
router<T>::router() = default;

template <typename T>
std::optional<typename router<T>::value_t>& router<T>::at(std::string_view path)
{
    if (empty())
        _root = std::make_shared<node>();
    if (path == "/")
        return _root->_value;
    return _root->at(path)->_value;
}

template <typename T>
const std::optional<typename router<T>::value_t>& router<T>::at(std::string_view path, const on_path_param_t& on_path_param) const
{
    // if (empty())
    //     return {};
    if (path == "/")
        return _root->_value;
    return _root->at(path, on_path_param)->_value;
}

template <typename T>
router<T> router<T>::mount(std::string_view path, router other)
{
    if (path == "/")
        return std::exchange(_root, other._root);
    if (empty())
        _root = std::make_shared<node>();
    return _root->mount(path, other._root);
}

template <typename T>
router<T> router<T>::unmount(std::string_view path)
{
    if (path == "/")
        return std::exchange(_root, {});
    if (empty())
        return {};
    return _root->unmount(path);
}

template <typename T>
bool router<T>::contains(std::string_view path) const
{
    if (empty())
        return false;
    if (path == "/")
        return _root && _root->_value;
    auto _node = _root->at(path);
    return _node && _node->_value;
}

template <typename T>
void router<T>::clear()
{
    _root = {};
}

template <typename T>
bool router<T>::empty() const 
{
    return !_root;
}

template <typename T>
router<T>::operator bool() const 
{
    return !empty();
}

}

#endif //MX_ROUTER_HPP