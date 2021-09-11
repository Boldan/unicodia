#pragma once

#include "QByteArray"

///
/// \brief The Mems class
///   Simple memory stream
///
class Mems
{
public:
    // Stream-like bhv (Motorola data here)
    const QByteArray& data() const { return d; }

    /// @return  ptr to beginning
    char* beg() { return d.begin(); }
    const char* beg() const { return d.begin(); }

    /// @return  ptr to end
    char* end() { return d.end(); }
    const char* end() const { return d.end(); }

    /// @return  ptr to carriage
    char* ptr() { return p; }
    const char* ptr() const { return p; }

    /// @return  data size
    size_t size() const { return d.size(); }

    /// @return  carriage position
    size_t pos() const { return p - beg(); }

    /// @return  how many bytes remains
    size_t remainder() const { return end() - p; }

    /// Seeks carriage to position x
    /// @post  pos() == x
    /// @throw  too big x
    void seek(size_t x);

    /// Reads dsize bytes, native byte order
    /// @throw too big dsize
    void read(char* data, size_t dsize);

    /// Writes dsize bytes, native byte order
    /// @throw too big dsize
    void write(const char* data, size_t dsize);

    /// Helper function: reads struc of known length, native byte order
    template <size_t Len, class T>
    void readStruc(T& data)
    {
        static constexpr auto actualLen = sizeof(std::remove_reference_t<T>);
        static_assert(Len == actualLen, "Wrong Len, maybe mistake?");
        read(reinterpret_cast<char*>(&data), Len);
    }

    /// Helper function: writes struc of known length, native byte order
    template <size_t Len, class T>
    void writeStruc(const T& data)
    {
        static constexpr auto actualLen = sizeof(std::remove_reference_t<T>);
        static_assert(Len == actualLen, "Wrong Len, maybe mistake?");
        write(reinterpret_cast<const char*>(&data), Len);
    }

    /// reads one byte
    uint8_t readB() {
        uint8_t r;
        readStruc<1>(r);
        return r;
    }

    /// read Intel word (2 bytes)
    uint16_t readIW();

    /// read Motorola word (2 bytes)
    uint16_t readMW();

    /// read Intel dword (4 bytes)
    uint32_t readID();

    /// read Motorola dword (4 bytes)
    uint32_t readMD();

    /// write Motorola dword (4 bytes)
    void writeMD(uint32_t x);

    /// Skip dword (4 bytes)
    inline void skipD() {
        uint32_t noMatter;
        readStruc<4>(noMatter);
    }

protected:
    QByteArray d {};
    char* p = nullptr;
};
