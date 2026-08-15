#ifndef MX_ROUTER_HPP
#define MX_ROUTER_HPP

#include <string_view>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <optional>
#include <utility>

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
        std::optional<value_t> value;
        std::unordered_map<std::string, std::shared_ptr<node>> children;

        std::shared_ptr<node> insert(std::string_view path);

        std::shared_ptr<node> find(std::string_view path, const on_path_param_t& on_path_param = {});
        std::shared_ptr<const node> find(std::string_view path, const on_path_param_t& on_path_param = {}) const;

        std::shared_ptr<node> insert(std::string_view path, std::shared_ptr<node> other);
        std::shared_ptr<node> remove(std::string_view path);
    };

    router(std::shared_ptr<node> _node);

    static const char* find_first_of(const char* data, const char* chars);

private:
    template <bool IsConst>
    class basic_iterator
    {
    private:
        using node_t = std::conditional_t<IsConst, const node, node>;
        using ref_t = std::conditional_t<IsConst, const std::optional<value_t>&, std::optional<value_t>&>;
        using ptr_t = std::conditional_t<IsConst, const std::optional<value_t>*, std::optional<value_t>*>;

    public:
        basic_iterator();
        basic_iterator(std::nullptr_t);

        ref_t operator*() const;
        ptr_t operator->() const;

        explicit operator bool() const;

        bool operator==(const basic_iterator& other) const;
        bool operator!=(const basic_iterator& other) const;

        operator basic_iterator<true>() const;

    private:
        friend class router;
        friend class basic_iterator<!IsConst>;
        basic_iterator(std::shared_ptr<node_t> _node);

    private:
        std::shared_ptr<node_t> _node;
    };

public:
    using iterator = basic_iterator<false>;
    using const_iterator = basic_iterator<true>;

public:
    router();
    router(iterator it);

    iterator insert(std::string_view path);

    iterator find(std::string_view path, const on_path_param_t& on_path_param = {});
    const_iterator find(std::string_view path, const on_path_param_t& on_path_param = {}) const;

    iterator insert(std::string_view path, iterator other);
    iterator remove(std::string_view path);

    bool contains(std::string_view path) const;

    iterator root();
    const_iterator root() const;

    void clear();
    bool empty() const;
    operator bool() const;

private:
    std::shared_ptr<node> _root;
};

template <typename T>
std::shared_ptr<typename router<T>::node> router<T>::node::insert(std::string_view path)
{
    const char* begin = path.size() > 0 ? path.data() + 1 : path.data();
    const char* end = find_first_of(begin, "/?");
    std::string next_word(begin, end - begin);

    auto it = children.find(next_word);

    if (it == std::end(children))
        it = children.emplace(next_word, std::make_shared<node>()).first;

    if (*end == 0 || *end == '?')
        return it->second;

    return it->second->insert(end);
}

template <typename T>
std::shared_ptr<typename router<T>::node> router<T>::node::find(std::string_view path, const on_path_param_t& on_path_param)
{
    const char* begin = path.size() > 0 ? path.data() + 1 : path.data();
    const char* end = find_first_of(begin, "/?");
    std::string next_word(begin, end - begin);

    auto it = children.find(next_word);

    if (it != std::end(children))
    {
        if (*end == 0 || *end == '?')
        {
            if (!it->second->value)
                return {};
            return it->second;
        }
        auto _node = it->second->find(end, on_path_param);
        if (_node)
            return _node;
    }

    it = std::begin(children);
    for (it = std::find_if(it, std::end(children), [](const auto& kv) { return !kv.first.empty() && kv.first[0] == ':'; }); it != std::end(children); ++it)
    {
        if (*end == 0 || *end == '?')
        {
            if (!it->second->value)
                return {};
            std::string_view param(it->first.data() + 1, it->first.size() - 1);
            if (!param.empty() && on_path_param)
                on_path_param(param, next_word);
            return it->second;
        }

        auto _node = it->second->find(end, on_path_param);
        if (_node)
        {
            std::string_view param(it->first.data() + 1, it->first.size() - 1);
            if (!param.empty() && on_path_param)
                on_path_param(param, next_word);
            return _node;
        }
    }

    it = std::find_if(std::begin(children), std::end(children), [](const auto& kv) { return !kv.first.empty() && kv.first[0] == '*'; });
    if (it == std::end(children) || !it->second->value)
        return {};
    end = find_first_of(begin, "?");
    std::string_view param(it->first.data() + 1, it->first.size() - 1);
    if (!param.empty() && on_path_param)
        on_path_param(param, std::string_view(begin, end - begin));
    return it->second;
}

template <typename T>
std::shared_ptr<const typename router<T>::node> router<T>::node::find(std::string_view path, const on_path_param_t& on_path_param) const
{
    return const_cast<node*>(this)->find(path, on_path_param);
}

template <typename T>
std::shared_ptr<typename router<T>::node> router<T>::node::insert(std::string_view path, std::shared_ptr<node> other)
{
    const char* begin = path.size() > 0 ? path.data() + 1 : path.data();
    const char* end = find_first_of(begin, "/?");
    std::string next_word(begin, end - begin);

    auto it = children.find(next_word);

    if (it == std::end(children))
        it = children.emplace(next_word, std::make_shared<node>()).first;
    
    if (*end == 0 || *end == '?')
        return std::exchange(it->second, other);

    return it->second->insert(end, other);
}

template <typename T>
std::shared_ptr<typename router<T>::node> router<T>::node::remove(std::string_view path)
{
    const char* begin = path.size() > 0 ? path.data() + 1 : path.data();
    const char* end = find_first_of(begin, "/?");
    std::string next_word(begin, end - begin);

    auto it = children.find(next_word);

    if (it == std::end(children))
        return {};
        
    if (*end == 0 || *end == '?')
    {
        auto _node = it->second;
        children.erase(it);
        return _node;
    }

    return it->second->remove(end);
}

template <typename T>
template <bool IsConst>
router<T>::basic_iterator<IsConst>::basic_iterator() = default;

template <typename T>
template <bool IsConst>
router<T>::basic_iterator<IsConst>::basic_iterator(std::nullptr_t) : basic_iterator() {}

template <typename T>
template <bool IsConst>
typename router<T>::basic_iterator<IsConst>::ref_t router<T>::basic_iterator<IsConst>::operator*() const
{
    return _node->value;
}

template <typename T>
template <bool IsConst>
typename router<T>::basic_iterator<IsConst>::ptr_t router<T>::basic_iterator<IsConst>::operator->() const
{
    return &_node->value;
}

template <typename T>
template <bool IsConst>
router<T>::basic_iterator<IsConst>::operator bool() const
{
    return _node != nullptr;
}

template <typename T>
template <bool IsConst>
bool router<T>::basic_iterator<IsConst>::operator==(const basic_iterator& other) const
{
    return _node == other._node;
}

template <typename T>
template <bool IsConst>
bool router<T>::basic_iterator<IsConst>::operator!=(const basic_iterator& other) const
{
    return !(*this == other);
}

template <typename T>
template <bool IsConst>
router<T>::basic_iterator<IsConst>::operator basic_iterator<true>() const
{
    return std::shared_ptr<const node>(_node);
}

template <typename T>
template <bool IsConst>
router<T>::basic_iterator<IsConst>::basic_iterator(std::shared_ptr<node_t> _node) : _node{_node} {}

template <typename HandlerT>
const char* router<HandlerT>::find_first_of(const char* data, const char* chars)
{
    for (; *data; ++data)
        for (const char* ch = chars; *ch; ++ch)
            if (*data == *ch)
                return data;
    return data;
}

template <typename T>
router<T>::router() = default;

template <typename T>
router<T>::router(iterator it) : _root{it._node} {}

template <typename T>
typename router<T>::iterator router<T>::insert(std::string_view path)
{
    if (empty())
        _root = std::make_shared<node>();
    if (path == "/")
        return _root;
    return _root->insert(path);
}

template <typename T>
typename router<T>::iterator router<T>::find(std::string_view path, const on_path_param_t& on_path_param)
{
    if (empty())
        return {};
    if (path == "/" && _root->value)
        return _root;
    return _root->find(path, on_path_param);
}

template <typename T>
typename router<T>::const_iterator router<T>::find(std::string_view path, const on_path_param_t& on_path_param) const
{
    return const_cast<router*>(this)->find(path, on_path_param);
}

template <typename T>
typename router<T>::iterator router<T>::insert(std::string_view path, iterator other)
{
    if (path == "/")
        return std::exchange(_root, other._node);
    if (empty())
        _root = std::make_shared<node>();
    return _root->insert(path, other._node);
}

template <typename T>
typename router<T>::iterator router<T>::remove(std::string_view path)
{
    if (path == "/")
        return std::exchange(_root, {});
    if (empty())
        return {};
    return _root->remove(path);
}

template <typename T>
bool router<T>::contains(std::string_view path) const
{
    if (empty())
        return false;
    if (path == "/")
        return _root->value.has_value();
    return _root->find(path) != nullptr;
}

template <typename T>
typename router<T>::iterator router<T>::root()
{
    return _root;
}

template <typename T>
typename router<T>::const_iterator router<T>::root() const
{
    return _root;
}

template <typename T>
void router<T>::clear()
{
    _root = {};
}

template <typename T>
bool router<T>::empty() const
{
    return _root == nullptr;
}

template <typename T>
router<T>::operator bool() const
{
    return !empty();
}

}

#endif //MX_ROUTER_HPP