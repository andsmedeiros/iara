#ifndef OBJECT_BUFFER_HPP
#define OBJECT_BUFFER_HPP

template<class T>
struct object_buffer {
    alignas(T) unsigned char data[sizeof(T)];

    template<class ... T_args>
    T &construct(T_args &... args) {
        return *new(reinterpret_cast<T *>(&data)) T(args ...);
    }

    void destruct() {
        reinterpret_cast<T *>(&data)->~T();
    }

    operator T &() const {
        return *reinterpret_cast<const T *>(&data);
    }
    
    operator T &() {
        return *reinterpret_cast<T *>(&data);
    }
};

#endif /* OBJECT_BUFFER_HPP */