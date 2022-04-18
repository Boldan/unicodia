#pragma once

#include <unordered_map>
#include <filesystem>

namespace loc {

    class Text
    {
    public:
        Text() noexcept = default;

        template <class T>
        Text(T&& x) : fSrc(std::forward<T>(x)) {}
    private:
        std::u8string fSrc;
    };

    class Dic
    {
    public:
        void clear() { fMap.clear(); }
        void add(std::string_view id, std::u8string translation);
    private:
        std::unordered_map<std::string, Text> fMap;;
    };

    extern Dic dic;

    void loadIni(Dic& d, const std::filesystem::path& path);

}   // namespace loc
