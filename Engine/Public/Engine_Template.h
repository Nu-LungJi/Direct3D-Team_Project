#pragma once

namespace Engine
{
    template <typename T>
    class Singleton
    {
    protected:
        Singleton() = default;
        virtual ~Singleton() = default;

    public:
        Singleton(const Singleton&) = delete;
        Singleton& operator=(const Singleton&) = delete;
        Singleton(Singleton&&) = delete;
        Singleton& operator=(Singleton&&) = delete;

        //static T& Get()
        //{
        //    // 내부 클래스를 통해 T를 상속받아 생성자를 노출시킴
        //    //struct T_Proxy : public T {
        //    //    using T::T;
        //    //};
        //    //static T_Proxy instance;
        //    static T instance;
        //    return instance;
        //}

    private:
        static T& Instance()
        {
            static T instance;
            return instance;
        }

    public:
        static T& Get() { return Instance(); }
        static const T& GetConst() { return Instance(); }
    };
}