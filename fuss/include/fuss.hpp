/**
 * @file fuss/include/fuss.hpp
 * @brief Contains all necessary definitions for FUSS
 * @author André Medeiros
 * @date 26/06/23
 * @copyright 2023 (C) André Medeiros
**/

#ifndef FUSS_HPP
#define FUSS_HPP

#include <list>
#include <memory>

namespace fuss {

template<class, class...>
class shouter;

/**
 * @brief Defines a generic cancellable interface
 */
class cancellable {
public:

    /**
     * @brief Should cancel a derived construct
     */
    virtual void cancel() = 0;
    virtual ~cancellable() noexcept = default;
};

/**
 * @brief A message is a type-based contract
 * @tparam T_args The type signature of this message: any shouter must
 * be able to produce `T_args...` when calling `fuss::shouter::shout()` and
 * and any listener must be able to consume `T_args...` through the functor
 * that it provides
 */
template<class ...T_args>
class message {  
public:
    class handler;

    /**
     * @brief Each message type has a unique handler type; this aliases a list
     * of that unique handler type
     */
    using handler_list = std::list<std::shared_ptr<handler>>;

    /**
     * @brief The unique handler type for this specific message type
     */
    class handler : public cancellable {
        template<class, class...> friend class shouter;

    protected:
        /**
         * @brief Keeps a reference of the list where this handler is stored for
         * when the handler is unregistered
         */
        handler_list &source;

        /**
         * @brief An iterator that points to this element in a list
         */
        typename handler_list::iterator element;

        /**
         * @brief Creates a new message handler to be inserted in the provided list
         * @param source The list that will contain this new handler
         */
        handler(handler_list &source) :
            source { source },
            element { source.end() }
        {  }

        /**
         * @brief Message handler calling interface
         * @param args The arguments with which to invoke this handler
         */
        virtual void operator()(T_args...args) = 0;

    public:
        virtual ~handler() noexcept = default;

        /**
         * @brief Cancels this handler by removing it from the list where it is
         * stored
         */
        virtual void cancel() noexcept override {
            if(element != source.end()) {
                source.erase(element);
                element = source.end();
            }
        }
        
    private:
        /**
         * @brief Informs the handler of where it was stored
         * @param it The iterator that points to this handler
         */
        void attach(typename handler_list::iterator it) {
            element = it;
        }
    };

    /**
     * @brief Stores the concrete implementation of a handler functor
     * @tparam T The type of the functor wrapped by this object
     */
    template<class T>
    class functor : public handler {
        /**
         * @brief This is the actual callable object that gets executed
         */
        T t;

    public:
        /**
         * @brief Constructs a new wrapper around the supplied functor object
         * @tparam T_func The type of the functor being wrapped
         * @param source The list into which this functor will be placed
         * @param func The functor to be wrapped
         */
        template<class T_func>
        functor(handler_list &source, T_func &&func) : 
            handler { source },
            t { std::forward<T_func>(func) }
        {  }

        functor(const functor &)
            noexcept(std::is_nothrow_copy_constructible_v<T>) = default;

        functor(functor &&)
            noexcept(std::is_nothrow_move_constructible_v<T>) = default;

        functor &operator=(const functor &)
            noexcept(std::is_nothrow_copy_assignable_v<T>) = default;

        functor &operator=(functor &&)
            noexcept(std::is_nothrow_move_assignable_v<T>) = default;

        /**
         * @brief Invokes the wrapped functor with the provided arguments
         * @param args The arguments that will be forwarded to the functor
         */
        virtual void operator()(T_args ...args) override {
            t(std::move(args)...);
        }
    };
};

/**
 * @brief A message listener keeps a weak pointer to a message handler attached
 * to a particular message shouter, so the subscription can be cancelled
 * safely anytime
 */
class listener {
    /**
     * @brief Keeps a weak reference to the message handler
     */
    std::weak_ptr<cancellable> handler;

public:
    /**
     * @brief Constructs a new empty listener, that points to no handler
     */
    inline listener() noexcept = default;

    /**
     * @brief Creates a new listener out a shared pointer to a handler
     * @param handler A shared pointer to a handler object, cast to a
     * cancellable interface
     */
    inline listener(const std::shared_ptr<cancellable> &handler) noexcept :
        handler { handler }
    {  }


    listener(const listener &) noexcept = default;
    listener(listener &&) noexcept = default;
    virtual ~listener() noexcept = default;

    listener &operator=(const listener &) noexcept = default;
    listener &operator=(listener &&) noexcept = default;

    /**
     * @brief Attempts to lock the weak pointer and cancel the message
     * handler by removing it from the shouter's handler list
     */
    inline void cancel() const noexcept {
        if(auto h = handler.lock()) {
            h->cancel();
        }
    }
};

/**
 * @brief RAII-enabled container for a message listener; when it
 * goes out of scope, it cancels the handler pointer by the listener
 */
class message_guard : public listener {
public:
    using listener::operator=;

    message_guard() noexcept = default;

    /**
     * @brief Message guards are move-only, so its copy constructor is
     * deleted
     */
    message_guard(const message_guard &) = delete;
    message_guard(message_guard &&) noexcept = default;

    /**
     * @brief Converting constructor; allows to seemingly cast a message
     * listener to a message guard via copy-construction
     * @param listener The message listener
     */
    inline message_guard(const fuss::listener &listener) : 
        fuss::listener { listener }
    {  }

    /**
     * @brief Converting constructor; allows to seemingly cast a message
     * listener to a message guard via move-construction
     * @param listener The message listener
     */
    inline message_guard(fuss::listener &&listener) noexcept :
        fuss::listener { std::move(listener) } 
    {  }

    /**
     * @brief Copy-assignment deleted because message guards are
     * move-only objects
     */
    message_guard &operator=(const message_guard &) = delete;
    message_guard &operator=(message_guard &&) noexcept = default;

    /**
     * @brief Releases the guard: attempt to cancel the message handler
     * apointer by the listener
     */
    inline void release() const noexcept { cancel(); }

    /**
     * @brief Upon destruction, release this guard
     */
    ~message_guard() noexcept {
        release();
    }
};

/**
 * @brief A shouter is an actor that can broadcast messages containing
 * arbitrary data to interested parties, who react to the messages through
 * attached message handlers
 * @attention this is a proxy type that can produce a single
 * shouter class for the message classes in <T_message, T_rest...>
 * @tparam T_message The first message in the pack
 * @tparam T_rest  The rest of the messages in the pack
 */
template<class T_message, class ... T_rest>
struct shouter : public shouter<T_message>, public shouter<T_rest ...> {
  using shouter<T_message>::listen;
  using shouter<T_message>::shout;
  using shouter<T_rest ...>::listen;
  using shouter<T_rest ...>::shout;
};

/**
 * @brief A shouter is an actor that can broadcast messages containing
 * arbitrary data to interested parties, who react to the messages through
 * attached message handlers
 * @tparam T_message The type of the message this object can shout
 */
template<class T_message>
class shouter<T_message> {
public:
    /**
     * @brief The type of the handler is provided by the message type
     */
    using handler = typename T_message::handler;

    /**
     * @brief The type of the functor object is provided by the message type
     */
    template<class T> using functor = typename T_message::template functor<T>;

    /**
     * @brief Represents a list of message handlers
     */
    using handler_list = typename T_message::handler_list;

    /**
     * @brief The list of handlers registered at this shouter; whenever `.shout()`
     * is called, each handler in this list will be invoked
     */
    handler_list handlers;

public:

    /**
     * @brief Attaches a new message handler to this shouter and returns the
     * message listener that represents this subscription
     * @tparam T_msg The type of the message that is being shouted; this parameter
     * is used to disambiguate between the multiple `.shout()` functions a single
     * shouter can have
     * @tparam T The type of the functor to be executed when the message handler
     * is called
     * @param t The handler functor
     * @return A message listener that can be used to cancel this subscription
     */
    template<class T_msg, class T>
    std::enable_if_t<std::is_same_v<T_message, T_msg>, listener>
    listen(T &&t) {
        auto func = 
            std::make_shared<functor<std::decay_t<T>>>(handlers, std::forward<T>(t));
        auto iterator = handlers.emplace(handlers.end(), std::move(func));
        (*iterator)->attach(iterator);

        return std::static_pointer_cast<cancellable>(*iterator);
    }

    /**
     * @brief Broadcasts a message, calling each message handler in this shouter's
     * list with the provided arguments
     * @tparam T_msg The type of the message to shout; this parameter is used to
     * disambiguate between the multiple `.listen()` functions a single shouter
     * can have
     * @tparam T_args The type of the parameters that will be handled to each handler
     * @param args The arguments used to call each handler
     */
    template<class T_msg, class ...T_args>
    std::enable_if_t<std::is_same_v<T_message, T_msg>>
    shout(T_args &&...args) {
        for(auto &handler : handlers) {
            (*handler)(args...);
        }
    }
};

/**
 * @brief Utility metatemplate that installs a trampoline function on a derived class
 * @details This class is used when implementing a class that inherits from both
 * `shouter` and a class derived from `shouter`. In this case, the compiler is unable
 * to disambiguate between their `listen()` and `shout()` functions unless explicitly
 * informed of whose shouter ancestor's function to call. This class installs a
 * trampoline version of `.listen()` and `.shout()` that does this disambiguation
 * @tparam T_shouters The type of the shouters that will compose this group
 */
template<class ...T_shouters>
struct group : public T_shouters... {

    /**
     * @brief Explicitly calls `shouter<T_message>::template shout<T_message>(T_args...)`
     * @tparam T_message The type of the message being shouted
     * @tparam T_args The type of the arguments being forwarded to the shouter
     * @param args The provided arguments
     */
    template<class T_message, class ...T_args>
    void shout(T_args &&...args) {
        shouter<T_message>::template shout<T_message>(std::forward<T_args>(args)...);
    }

    /**
     * @brief Explicitly calls `shouter<T_message>::template listen<T_message>(T_functor)`
     * @tparam T_message The type of the message for which to listen
     * @tparam T_functor The type of the functor to forward to the shouter
     * @param fun The functor object to be used in the handler constructor
     * @return The message listener returned by the shouter
     */
    template<class T_message, class T_functor>
    auto listen(T_functor &&fun) {
        return shouter<T_message>::
            template listen<T_message>(std::forward<T_functor>(fun));
    }
};

} /* namespace fuss */

#endif /* FUSS_HPP */
