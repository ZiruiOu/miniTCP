#ifndef MINITCP_RSC_COMMON_BUFFER_WRITER_H_
#define MINITCP_RSC_COMMON_BUFFER_WRITER_H_

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>

#include "./logging.h"

// goal : achieve zero-copy frame writer

namespace minitcp {
class BufferWriterBase {
   public:
    BufferWriterBase(std::size_t max_size);
    virtual ~BufferWriterBase() = 0;

    void WriteUInt8(std::uint8_t number);
    void WriteUInt16(std::uint16_t number);
    void WriteUInt32(std::uint32_t number);

    void WriteBytes(const char *array_pointer, std::size_t length);

    std::shared_ptr<std::int8_t> GetBuffer() const { return buffer_; }

   protected:
    std::shared_ptr<std::int8_t> buffer_;
    std::size_t write_offset_;
    std::size_t max_size_;
}
}  // namespace minitcp

#endif  //! MINITCP_RSC_COMMON_BUFFER_WRITER_H_