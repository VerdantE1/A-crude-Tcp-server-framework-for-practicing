#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <cassert>


class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    {}

    size_t readableBytes() const 
    {
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    
    char* peek()
    {
        return begin() + readerIndex_;
    }

    
    
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            readerIndex_ += len; 
        }
        else   
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    
    std::string retrieveAllAsString()
    {
        assert(readerIndex_ <= writerIndex_);
        return retrieveAsString(readableBytes()); 
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len); 
        return result;
    }

    

    //See makeSpace()
    void ensureWriteableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len); 
        }
    }

    

    //If the reall space is enough then do not need resize more space.
    //If not , resize the space
    //See ensureWriteableBytes()
    void append(const char *data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }


    /*void append(const char*  data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }
*/

 

    void prepend(const void* data, size_t len)
    {
        readerIndex_ -= len;
        const char* d = (const char*) (data);
        std::copy(d, d + len, begin() + readerIndex_);
    }

    const char* findCRLF() const
    {
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    const char* findCRLF(const char* start) const
    {
        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? NULL : crlf;
    }


    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    /*******manually Opration  Buffer ******/
    //should use ref receive
    size_t& Getreadidx() { return readerIndex_; }
    size_t& Getwriteidx() { return writerIndex_; }


    ssize_t readFd(int fd, int* saveErrno);  // Read IO(cfd) to Buffer
    ssize_t writeFd(int fd, int* saveErrno); // Write Buffer to IO(cfd)



    char& operator[](size_t idx)
    {
        return buffer_[idx];
    }



private:
    char* begin()     //The real chars's address
    {
        return &*buffer_.begin();  
    }
    const char* begin() const
    {
        return &*buffer_.begin();
    }
    void makeSpace(size_t len)
    {
        //if real space is not enough
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        //real space(After move data) is enough
        else
        {
            size_t readalbe = readableBytes();
            std::copy(begin() + readerIndex_, 
                    begin() + writerIndex_,
                    begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readalbe;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_; //Use index(size_t) insteadof iterator  is to prevent the Iterator failure After reszie the vector
    size_t writerIndex_;
    static const char kCRLF[];
};