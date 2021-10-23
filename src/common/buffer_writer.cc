
#include "buffer_writer.h"

namespace minitcp {

BufferWriterBase::BufferWriterBase(std::size_t max_size)
    : buffer_(new std::int8_t[max_size]()),
      write_offset_(0),
      max_size_(max_size) {
    std::cout << buffer_.use_count() << std::endl;
}

void BufferWriterBase::WriteUInt8(std::uint8_t number) {
    MINITCP_ASSERT_LT(write_offset_ + 1, max_size_)
        << "Write u8: buffer writer out of memory " << std::endl;
    memcpy(buffer_.get() + write_offset_, &number, sizeof(number));
    write_offset_ += 1;
}

void BufferWriterBase::WriteUInt16(std::unit16_t number) {
    MINITCP_ASSERT_LT(write_offset_ + sizeof(number), max_size_)
        << "Write u16: buffer writer out of memory " << std::endl;

    std::uint16_t hton_number = htons(number);
    memcpy(buffer_.get() + write_offset_, &hton_number, sizeof(hton_number));
    write_offset_ += sizeof(hton_number);
}

void BufferWriterBase::WriteUInt32(std::uint32_t number) {
    MINITCP_ASSERT_LT(write_offset_ + sizeof(number), max_size_)
        << "Write u32: buffer writer out of memory " << std::endl;

    std::uint32_t hton_number = htonl(number);
    memcpy(buffer_.get() + write_offset_, &hton_number, sizeof(hton_number));
    write_offset_ += sizeof(hton_number);
}

void BufferWriterBase::WriteBytes(const char* array_pointer,
                                  std::size_t length) {
    MINITCP_ASSERT_LT(write_offset_ + length, max_size_)
        << "WriteBytes: buffer writer out of memory " << std::endl;
    memcpy(buffer_.get() + write_offset_, array_pointer, length);
    write_offset_ += length;
}

}  // namespace minitcp