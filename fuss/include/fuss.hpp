#ifndef FUSS_HPP
#define FUSS_HPP

#include <list>
#include <memory>

namespace fuss {

template<class, class...>
class shouter;

class cancellable {
public:
    virtual void cancel() = 0;
    virtual ~cancellable() noexcept = default;
};

template<class ...T_args>
class message {  
public:

    class handler;
    using handler_list = std::list<std::shared_ptr<handler>>;

    class handler : public cancellable {
        template<class, class...> friend class shouter;

    protected:
        handler_list &source;
        typename handler_list::iterator element;

        handler(handler_list &source) :
            source { source },
            element { source.end() }
        {  }

        virtual void operator()(T_args...) = 0;

    public:
        virtual ~handler() = default;

        virtual void cancel() noexcept override {
            if(element != source.end()) {
                source.erase(element);
                element = source.end();
            }
        }
        
    private:
        void attach(typename handler_list::iterator it) {
            element = it;
        }
    };

    template<class T>
    class functor : public handler {
        T t;

    public:
        template<class T_func>
        functor(handler_list &source, T_func &&func) : 
            handler { source },
            t { std::forward<T_func>(func) }
        {  }

        functor(const functor &) = default;
        functor(functor &&) = default;

        functor &operator=(const functor &) = default;
        functor &operator=(functor &&) = default;

        virtual void operator()(T_args ...args) override {
            t(args...);
        }
    };

};

class listener {
    std::weak_ptr<cancellable> handler;

public:
    listener() = default;
    inline listener(const std::shared_ptr<cancellable> &handler) :
        handler { handler }
    {  }

    listener(const listener &) = default;
    listener(listener &&) noexcept = default;
    virtual ~listener() = default;

    listener &operator=(const listener &) = default;
    listener &operator=(listener &&) noexcept = default;

    inline void cancel() const noexcept {
        if(auto h = handler.lock()) {
            h->cancel();
        }
    }
};

class message_guard : public listener {
public:
    using listener::operator=;

    message_guard() = default;
    message_guard(const message_guard &) = delete;
    message_guard(message_guard &&) noexcept = default;
    inline message_guard(const fuss::listener &listener) : 
        fuss::listener { listener }
    {  }
    inline message_guard(fuss::listener &&listener) noexcept :
        fuss::listener { std::move(listener) } 
    {  }
    
    message_guard &operator=(const message_guard &) = delete;
    message_guard &operator=(message_guard &&) noexcept = default;

    inline void release() const noexcept { cancel(); }

    ~message_guard() {
        release();
    }
};

template<class T_message, class ... T_rest>
struct shouter : public shouter<T_message>, public shouter<T_rest ...> {
  using shouter<T_message>::listen;
  using shouter<T_message>::shout;
  using shouter<T_rest ...>::listen;
  using shouter<T_rest ...>::shout;
};

template<class T_message>
class shouter<T_message> {
public:
    using handler = typename T_message::handler;
    template<class T> using functor = typename T_message::template functor<T>;
    using handler_list = typename T_message::handler_list;

    handler_list handlers;

public:
    template<class T_msg, class T>
    std::enable_if_t<std::is_same_v<T_message, T_msg>, listener>
    listen(T &&t) {
        auto func = 
            std::make_shared<functor<std::decay_t<T>>>(handlers, std::forward<T>(t));
        auto iterator = handlers.emplace(handlers.end(), std::move(func));
        (*iterator)->attach(iterator);

        return std::static_pointer_cast<cancellable>(*iterator);
    }

    template<class T_msg, class ...T_args>
    std::enable_if_t<std::is_same_v<T_message, T_msg>>
    shout(T_args &&...args) {
        for(auto &handler : handlers) {
            (*handler)(args...);
        }
    }
};

template<class ...T_shouters>
struct group : public T_shouters... {
    template<class T_message, class ...T_args>
    void shout(T_args &&...args) {
        shouter<T_message>::template shout<T_message>(std::forward<T_args>(args)...);
    }

    template<class T_message, class T_functor>
    auto listen(T_functor &&fun) {
        return shouter<T_message>::
            template listen<T_message>(std::forward<T_functor>(fun));
    }
};

} /* namespace fuss */

#endif /* FUSS_HPP */
