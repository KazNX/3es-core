//
// author: Kazys Stepanas
//
#include <cstddef>

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
namespace tes
{
inline DataBuffer::DataBuffer() = default;


inline DataBuffer::DataBuffer(DataStreamType type, size_t component_count, size_t component_stride)
  : _component_count(int_cast<uint8_t>(component_count))
  , _element_stride(int_cast<uint8_t>(component_stride ? component_stride : component_count))
  , _type(type)
{
  switch (type)
  {
  case DctNone:
  case DctPackedFloat16:
  case DctPackedFloat32:
    _component_count = _element_stride = 0;
    break;
  case DctInt8:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<int8_t>::size());
    _affordances = detail::DataBufferAffordancesT<int8_t>::instance();
    break;
  case DctUInt8:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<uint8_t>::size());
    _affordances = detail::DataBufferAffordancesT<uint8_t>::instance();
    break;
  case DctInt16:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<int16_t>::size());
    _affordances = detail::DataBufferAffordancesT<int16_t>::instance();
    break;
  case DctUInt16:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<uint16_t>::size());
    _affordances = detail::DataBufferAffordancesT<uint16_t>::instance();
    break;
  case DctInt32:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<int32_t>::size());
    _affordances = detail::DataBufferAffordancesT<int32_t>::instance();
    break;
  case DctUInt32:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<uint32_t>::size());
    _affordances = detail::DataBufferAffordancesT<uint32_t>::instance();
    break;
  case DctInt64:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<int64_t>::size());
    _affordances = detail::DataBufferAffordancesT<int64_t>::instance();
    break;
  case DctUInt64:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<uint64_t>::size());
    _affordances = detail::DataBufferAffordancesT<uint64_t>::instance();
    break;
  case DctFloat32:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<float>::size());
    _affordances = detail::DataBufferAffordancesT<float>::instance();
    break;
  case DctFloat64:
    _primitive_type_size = int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<double>::size());
    _affordances = detail::DataBufferAffordancesT<double>::instance();
    break;
  }
}

template <typename T>
inline DataBuffer::DataBuffer(const T *v, size_t count, size_t component_count,
                              size_t component_stride)
  : _stream(v)
  , _count(int_cast<unsigned>(count))
  , _component_count(int_cast<uint8_t>(component_count))
  , _element_stride(int_cast<uint8_t>(component_stride ? component_stride : component_count))
  , _primitive_type_size(int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<T>::size()))
  , _type(DataBufferPrimitiveTypeInfo<T>::type())
  , _affordances(detail::DataBufferAffordancesT<T>::instance())
{}


template <typename T, typename std::enable_if_t<!std::is_const_v<T>, bool>>
inline DataBuffer::DataBuffer(bool own_pointer, T *v, size_t count, size_t component_count,
                              size_t component_stride)
  : _stream((own_pointer) ? v : const_cast<const T *>(v))  // Set const pointer if not owned.
  , _count(int_cast<unsigned>(count))
  , _component_count(int_cast<uint8_t>(component_count))
  , _element_stride(int_cast<uint8_t>(component_stride ? component_stride : component_count))
  , _primitive_type_size(int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<T>::size()))
  , _type(DataBufferPrimitiveTypeInfo<T>::type())
  , _flags((own_pointer && v) ? Flag::OwnPointer : Flag::Zero)
  , _affordances(detail::DataBufferAffordancesT<T>::instance())
{
  static_assert(typeid(std::remove_const_t<T>) != typeid(Vector3f),
                "Unsupported mutable stream type. Use const T * constructor and duplicate()");
  static_assert(typeid(std::remove_const_t<T>) != typeid(Vector3d),
                "Unsupported mutable stream type. Use const T * constructor and duplicate()");
  static_assert(typeid(std::remove_const_t<T>) != typeid(Colour),
                "Unsupported mutable stream type. Use const T * constructor and duplicate()");
}


inline DataBuffer::DataBuffer(const Vector3f *v, size_t count)
  : _stream(v ? v->storage().data() : null())
  , _count(int_cast<unsigned>(count))
  , _component_count(3)
  , _element_stride(int_cast<uint8_t>(sizeof(Vector3f) / sizeof(float)))
  , _primitive_type_size(int_cast<uint8_t>(sizeof(Vector3f)))
  , _type(DataBufferPrimitiveTypeInfo<float>::type())
  , _flags(0)
  , _affordances(detail::DataBufferAffordancesT<float>::instance())
{}


inline DataBuffer::DataBuffer(const Vector3d *v, size_t count)
  : _stream(v ? v->storage().data() : null())
  , _count(int_cast<unsigned>(count))
  , _component_count(3)
  , _element_stride(int_cast<uint8_t>(sizeof(Vector3d) / sizeof(double)))
  , _primitive_type_size(int_cast<uint8_t>(sizeof(Vector3d)))
  , _type(DataBufferPrimitiveTypeInfo<double>::type())
  , _flags(0)
  , _affordances(detail::DataBufferAffordancesT<double>::instance())
{}


inline DataBuffer::DataBuffer(const Colour *c, size_t count)
  : _stream(c ? c->storage().data() : null())
  , _count(int_cast<unsigned>(count))
  , _primitive_type_size(int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<uint32_t>::size()))
  , _type(DctUInt32)
  , _flags(0)
  , _affordances(detail::DataBufferAffordancesT<uint32_t>::instance())
{
  static_assert(sizeof(Colour) == DataBufferPrimitiveTypeInfo<uint32_t>::size());
}


template <typename T>
inline DataBuffer::DataBuffer(const std::vector<T> &v, size_t component_count,
                              size_t component_stride)
  : _stream(v.data())
  , _count(int_cast<unsigned>(v.size() / (component_stride ? component_stride : component_count)))
  , _component_count(int_cast<uint8_t>(component_count))
  , _element_stride(int_cast<uint8_t>(component_stride ? component_stride : component_count))
  , _primitive_type_size(int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<T>::size()))
  , _type(DataBufferPrimitiveTypeInfo<T>::type())
  , _flags(0)
  , _affordances(detail::DataBufferAffordancesT<T>::instance())
{}

inline DataBuffer::DataBuffer(const std::vector<Vector3f> &v)
  : _stream(v.data()->storage().data())
  , _count(int_cast<unsigned>(v.size()))
  , _component_count(3)
  , _element_stride(int_cast<uint8_t>(sizeof(Vector3f) / sizeof(float)))
  , _primitive_type_size(int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<float>::size()))
  , _type(DctFloat32)
  , _flags(0)
  , _affordances(detail::DataBufferAffordancesT<float>::instance())
{}

inline DataBuffer::DataBuffer(const std::vector<Vector3d> &v)
  : _stream(v.data()->storage().data())
  , _count(int_cast<unsigned>(v.size()))
  , _component_count(3)
  , _element_stride(int_cast<uint8_t>(sizeof(Vector3d) / sizeof(double)))
  , _primitive_type_size(int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<double>::size()))
  , _type(DctFloat64)
  , _flags(0)
  , _affordances(detail::DataBufferAffordancesT<double>::instance())
{}

inline DataBuffer::DataBuffer(const std::vector<Colour> &colours)
  : _stream(colours.data()->storage().data())
  , _count(int_cast<unsigned>(colours.size()))
  , _primitive_type_size(int_cast<uint8_t>(DataBufferPrimitiveTypeInfo<uint32_t>::size()))
  , _type(DctUInt32)
  , _flags(0)
  , _affordances(detail::DataBufferAffordancesT<uint32_t>::instance())
{
  static_assert(sizeof(Colour) == DataBufferPrimitiveTypeInfo<uint32_t>::size());
}

inline DataBuffer::DataBuffer(DataBuffer &&other) noexcept
  : _stream(std::move(other._stream))  // NOLINT(performance-move-const-arg)
  , _count(std::exchange(other._count, 0))
  , _component_count(std::exchange(other._component_count, 0))
  , _element_stride(std::exchange(other._element_stride, 0))
  , _primitive_type_size(std::exchange(other._primitive_type_size, 0))
  , _type(std::exchange(other._type, DctNone))
  , _flags(std::exchange(other._flags, 0))
  , _affordances(std::exchange(other._affordances, nullptr))
{}

inline DataBuffer::DataBuffer(const DataBuffer &other)
  : _stream(other._stream)
  , _count(other._count)
  , _component_count(other._component_count)
  , _element_stride(other._element_stride)
  , _primitive_type_size(other._primitive_type_size)
  , _type(other._type)
  , _flags(0)  // Copy assignment. We do not own the pointer.
  , _affordances(other._affordances)
{}

inline DataBuffer::~DataBuffer()
{
  reset();
}

template <typename T>
inline void DataBuffer::set(const T *v, size_t count, size_t component_count,
                            size_t component_stride)
{
  *this = DataBuffer(v, count, component_count, component_stride);
}

template <typename T, typename std::enable_if_t<!std::is_const_v<T>, bool>>
inline void DataBuffer::set(bool own_pointer, T *v, size_t count, size_t component_count,
                            size_t component_stride)
{
  *this = DataBuffer(v, count, own_pointer, component_count, component_stride);
}

template <typename T>
inline void DataBuffer::set(const std::vector<T> &v, size_t component_count,
                            size_t component_stride)
{
  *this = DataBuffer(v, component_count, component_stride);
}

inline void DataBuffer::set(const std::vector<Vector3f> &v)
{
  *this = DataBuffer(v);
}

inline void DataBuffer::set(const std::vector<Vector3d> &v)
{
  *this = DataBuffer(v);
}

inline void DataBuffer::set(const std::vector<Colour> &colours)
{
  *this = DataBuffer(colours);
}

template <typename T>
inline T DataBuffer::get(size_t element_index, size_t component_index) const
{
  T datum{ 0 };
  _affordances->get(DataBufferPrimitiveTypeInfo<T>::type(), element_index, component_index, 1,
                    readPtr(), _count, _component_count, _element_stride, &datum, 1);
  return datum;
}

template <typename T>
inline size_t DataBuffer::get(size_t element_index, size_t element_count, T *dst,
                              size_t capacity) const
{
  const size_t components_read = _affordances->get(
    DataBufferPrimitiveTypeInfo<T>::type(), element_index, 0, element_count * _component_count,
    readPtr(), _count, _component_count, _element_stride, dst, capacity);
  return components_read / componentCount();
}

inline void DataBuffer::swap(DataBuffer &other) noexcept
{
  using std::swap;
  swap(_stream, other._stream);
  swap(_count, other._count);
  swap(_component_count, other._component_count);
  swap(_element_stride, other._element_stride);
  swap(_primitive_type_size, other._primitive_type_size);
  swap(_type, other._type);
  swap(_flags, other._flags);
  swap(_affordances, other._affordances);
}

inline DataBuffer &DataBuffer::operator=(DataBuffer &&other) noexcept
{
  swap(other);
  return *this;
}

inline DataBuffer &DataBuffer::operator=(const DataBuffer &other)
{
  if (this != &other)
  {
    _stream = other._stream;
    _count = other._count;
    _component_count = other._component_count;
    _element_stride = other._element_stride;
    _primitive_type_size = other._primitive_type_size;
    _type = other._type;
    _flags = 0;  // Copy assignment. We do not own the pointer.
    _affordances = other._affordances;
  }
  return *this;
}

template <typename T>
inline const T *DataBuffer::ptr(size_t element_index) const
{
  TES_ASSERT2(DataBufferPrimitiveTypeInfo<T>::type() == _type, "Element type mismatch");
  return &static_cast<const T *>(readPtr())[element_index];
}

template <typename T>
inline const T *DataBuffer::ptrAt(size_t element_index) const
{
  if (DataBufferPrimitiveTypeInfo<T>::type() == _type)
  {
    return &static_cast<const T *>(readPtr())[element_index];
  }
  return nullptr;
}

inline uint16_t DataBuffer::estimateTransferCount(size_t element_size, unsigned overhead,
                                                  unsigned byte_limit)
{
  // FIXME: Without additional overhead padding I was getting missing messages at the client with
  // no obvious error path. For this reason, we use 0xff00u, instead of 0xffffu
  //           packet header           message                 crc
  const size_t max_transfer =
    (0xff00u - (sizeof(PacketHeader) + overhead + sizeof(PacketWriter::CrcType))) / element_size;
  size_t count = byte_limit ? byte_limit / element_size : max_transfer;
  if (count > max_transfer)
  {
    count = max_transfer;
  }

  return static_cast<uint16_t>(count);
}

namespace detail
{
template <typename T>
DataBufferAffordances *DataBufferAffordancesT<T>::instance()
{
  static DataBufferAffordancesT<T> obj;
  return &obj;
}

template <typename T>
void DataBufferAffordancesT<T>::release(const void *stream_ptr) const
{
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  delete static_cast<const T *>(stream_ptr);
}

template <typename T>
void *DataBufferAffordancesT<T>::copyStream(const void *stream, const DataBuffer &buffer) const
{
  if (stream == nullptr)
  {
    // Nothing to copy.
    return nullptr;
  }

  // Allocate a new array.
  const T *src_array = static_cast<const T *>(stream);
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  T *new_array = new T[static_cast<size_t>(buffer.count()) * buffer.elementStride()];
  std::copy(src_array, src_array + buffer.count() * buffer.elementStride(), new_array);
  return new_array;
}

template <typename T>
uint32_t DataBufferAffordancesT<T>::write(PacketWriter &packet, uint32_t offset,
                                          DataStreamType write_as_type, unsigned byte_limit,
                                          uint32_t receive_offset, const DataBuffer &buffer,
                                          double quantisation_unit) const
{
  switch (write_as_type)
  {
  case DctInt8:
    return writeAs<int8_t>(packet, offset, write_as_type, byte_limit, receive_offset, buffer);
  case DctUInt8:
    return writeAs<uint8_t>(packet, offset, write_as_type, byte_limit, receive_offset, buffer);
  case DctInt16:
    return writeAs<int16_t>(packet, offset, write_as_type, byte_limit, receive_offset, buffer);
  case DctUInt16:
    return writeAs<uint16_t>(packet, offset, write_as_type, byte_limit, receive_offset, buffer);
  case DctInt32:
    return writeAs<int32_t>(packet, offset, write_as_type, byte_limit, receive_offset, buffer);
  case DctUInt32:
    return writeAs<uint32_t>(packet, offset, write_as_type, byte_limit, receive_offset, buffer);
  case DctInt64:
    return writeAs<int64_t>(packet, offset, write_as_type, byte_limit, receive_offset, buffer);
  case DctUInt64:
    return writeAs<uint64_t>(packet, offset, write_as_type, byte_limit, receive_offset, buffer);
  case DctFloat32:
    return writeAs<float>(packet, offset, write_as_type, byte_limit, receive_offset, buffer);
  case DctFloat64:
    return writeAs<double>(packet, offset, write_as_type, byte_limit, receive_offset, buffer);
  case DctPackedFloat16:
    return writeAsPacked<float, int16_t>(packet, offset, write_as_type, byte_limit, receive_offset,
                                         nullptr, static_cast<float>(quantisation_unit), buffer);
  case DctPackedFloat32:
    return writeAsPacked<double, int32_t>(packet, offset, write_as_type, byte_limit, receive_offset,
                                          nullptr, quantisation_unit, buffer);
  default:
    // Throw?
    return 0;
  }
}

template <typename T>
template <typename WriteType>
uint32_t DataBufferAffordancesT<T>::writeAs(PacketWriter &packet, uint32_t offset,
                                            DataStreamType write_as_type, unsigned byte_limit,
                                            uint32_t receive_offset, const DataBuffer &buffer) const
{
  const unsigned item_size = static_cast<unsigned>(sizeof(WriteType)) * buffer.componentCount();

  // Overhead: account for:
  // - uint32_t offset
  // - uint16_t count
  // - uint8_t component count
  // - uint8_t data type
  const unsigned overhead = sizeof(uint32_t) +  // offset
                            sizeof(uint16_t) +  // count
                            sizeof(uint8_t) +   // element stride
                            sizeof(uint8_t);    // data type;

  byte_limit =
    (byte_limit) ? (byte_limit > overhead ? byte_limit - overhead : 0) : packet.bytesRemaining();
  uint16_t transfer_count = DataBuffer::estimateTransferCount(item_size, overhead, byte_limit);
  if (transfer_count > buffer.count() - offset)
  {
    transfer_count = static_cast<uint16_t>(buffer.count() - offset);
  }

  // Write header
  bool ok = true;
  ok =
    packet.writeElement(static_cast<uint32_t>(offset + receive_offset)) == sizeof(uint32_t) && ok;
  ok = packet.writeElement(static_cast<uint16_t>(transfer_count)) == sizeof(uint16_t) && ok;
  ok = packet.writeElement(static_cast<uint8_t>(buffer.componentCount())) == sizeof(uint8_t) && ok;
  ok = packet.writeElement(static_cast<uint8_t>(write_as_type)) == sizeof(uint8_t) && ok;

  if (!ok)
  {
    return 0;
  }

  const T *src = buffer.ptr<T>(static_cast<size_t>(offset) * buffer.elementStride());
  unsigned write_count = 0;
  if (DataBufferPrimitiveTypeInfo<T>::type() == DataBufferPrimitiveTypeInfo<WriteType>::type() &&
      buffer.elementStride() == buffer.componentCount())
  {
    // We can write the array directly if the T/WriteType types match and the source array is
    // densely packed (element stride matches component count).
    write_count += static_cast<unsigned>(packet.writeArray(
                     // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                     reinterpret_cast<const WriteType *>(src),
                     static_cast<size_t>(transfer_count) * buffer.componentCount())) /
                   buffer.componentCount();
  }
  else
  {
    // We have either a striding mismatch or a type mismatch. Componentwise write
    for (unsigned i = 0; i < transfer_count; ++i)
    {
      unsigned component_write_count = 0;
      for (unsigned j = 0; j < buffer.componentCount(); ++j)
      {
        // NOLINTNEXTLINE(bugprone-signed-char-misuse)
        const auto dst_value = static_cast<WriteType>(src[j]);
        component_write_count +=
          static_cast<unsigned>(packet.writeElement(dst_value) / sizeof(dst_value));
      }
      write_count += component_write_count / buffer.componentCount();
      src += buffer.elementStride();
    }
  }

  if (write_count == transfer_count)
  {
    return write_count;
  }

  // Failed to write the expected number of items.
  return 0;
}

template <typename T>
template <typename FloatType, typename PackedType>
uint32_t DataBufferAffordancesT<T>::writeAsPacked(PacketWriter &packet, uint32_t offset,
                                                  DataStreamType write_as_type, unsigned byte_limit,
                                                  uint32_t receive_offset,
                                                  const FloatType *packet_origin,
                                                  const FloatType quantisation_unit,
                                                  const DataBuffer &buffer) const
{
  // packet_origin is used to define the packing origin. That is, items are packed releative to
  // this. quantisation_unit is the divisor used to quantise data. packedType must be either
  // DctPackedFloat16 or DctPackedFloat32 Each component is packed as:
  //    PackedType((vertex[componentIndex] - packedOrigin[componentIndex]) / quantisation_unit)
  const unsigned item_size = static_cast<unsigned>(sizeof(PackedType)) * buffer.componentCount();

  // Overhead: account for:
  // - uint32_t offset
  // - uint16_t count
  // - uint8_t element stride
  // - uint8_t data type
  // - FloatType quantisation_unit
  // - FloatType[buffer.componentCount()] packet_origin
  const auto overhead =
    int_cast<unsigned>(sizeof(uint32_t) +                             // offset
                       sizeof(uint16_t) +                             // count
                       sizeof(uint8_t) +                              // element stride
                       sizeof(uint8_t) +                              // data type
                       sizeof(quantisation_unit) +                    // quantisation_unit
                       sizeof(FloatType) * buffer.componentCount());  // packet_origin

  byte_limit = (byte_limit) ? byte_limit : packet.bytesRemaining();
  uint16_t transfer_count = DataBuffer::estimateTransferCount(item_size, overhead, byte_limit);
  if (transfer_count > buffer.count() - offset)
  {
    transfer_count = static_cast<uint16_t>(buffer.count() - offset);
  }

  if (transfer_count == 0)
  {
    return 0;
  }

  // Write header
  bool ok = true;
  ok =
    packet.writeElement(static_cast<uint32_t>(offset + receive_offset)) == sizeof(uint32_t) && ok;
  ok = packet.writeElement(static_cast<uint16_t>(transfer_count)) == sizeof(uint16_t) && ok;
  ok = packet.writeElement(static_cast<uint8_t>(buffer.componentCount())) == sizeof(uint8_t) && ok;
  ok = packet.writeElement(static_cast<uint8_t>(write_as_type)) == sizeof(uint8_t) && ok;
  const FloatType q_unit{ quantisation_unit };
  ok = packet.writeElement(q_unit) == sizeof(q_unit) && ok;

  if (packet_origin)
  {
    ok = packet.writeArray(packet_origin, buffer.componentCount()) == buffer.componentCount() && ok;
  }
  else
  {
    const FloatType zero{ 0 };
    for (unsigned i = 0; i < buffer.componentCount(); ++i)
    {
      ok = packet.writeElement(zero) == sizeof(FloatType) && ok;
    }
  }

  const T *src = buffer.ptr<T>(static_cast<size_t>(offset) * buffer.elementStride());
  unsigned write_count = 0;

  const FloatType quantisation_factor = FloatType{ 1 } / FloatType{ quantisation_unit };
  bool item_ok = true;
  for (unsigned i = 0; i < transfer_count; ++i)
  {
    for (unsigned j = 0; j < buffer.componentCount(); ++j)
    {
      auto dst_value = static_cast<FloatType>(src[j]);
      if (packet_origin)
      {
        dst_value -= packet_origin[j];
      }
      dst_value *= quantisation_factor;
      const auto packed = static_cast<PackedType>(std::round(dst_value));
      if (std::abs(static_cast<FloatType>(packed) - dst_value) > 1)
      {
        // Failed: quantisation limit reached.
        return 0;
      }
      item_ok = item_ok && packet.writeElement(packed) == sizeof(packed);
    }
    write_count += !!item_ok;
    src += buffer.elementStride();
  }

  if (write_count == transfer_count)
  {
    return write_count;
  }

  // Failed to write the expected number of items.
  return 0;
}

template <typename T>
uint32_t DataBufferAffordancesT<T>::read(PacketReader &packet, void **stream_ptr,
                                         unsigned *stream_size, bool *has_ownership,
                                         const DataBuffer &buffer) const
{
  uint32_t offset = 0u;
  uint16_t count = 0u;

  bool ok = true;
  ok = packet.readElement(offset) == sizeof(offset) && ok;
  ok = packet.readElement(count) == sizeof(count) && ok;

  if (!ok)
  {
    return 0;
  }

  return read(packet, stream_ptr, stream_size, has_ownership, buffer, offset, count);
}

template <typename T>
uint32_t DataBufferAffordancesT<T>::read(PacketReader &packet, void **stream_ptr,
                                         unsigned *stream_size, bool *has_ownership,
                                         const DataBuffer &buffer, unsigned offset,
                                         unsigned count) const
{
  bool ok = true;
  uint8_t component_count = 0;  // buffer.componentCount();;
  uint8_t packet_type = 0;      // DataBufferPrimitiveTypeInfo<T>::type();
  ok = packet.readElement(component_count) == sizeof(component_count) && ok;
  ok = packet.readElement(packet_type) == sizeof(packet_type) && ok;

  if (!ok)
  {
    return 0;
  }

  T *new_ptr = nullptr;
  if (*stream_ptr == nullptr || !*has_ownership || *stream_size < (offset + count))
  {
    // Current stream too small. Reallocate. Note we allocate with the buffer's component count.
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    new_ptr = new T[static_cast<size_t>(offset + count) * buffer.componentCount()];
  }

  if (new_ptr)
  {
    if (*stream_ptr)
    {
      std::copy(static_cast<const T *>(*stream_ptr),
                static_cast<const T *>(*stream_ptr) + (*stream_size) * component_count, new_ptr);
      if (*has_ownership)
      {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        delete[] static_cast<const T *>(*stream_ptr);
      }
    }
    *stream_ptr = new_ptr;
    *stream_size = offset + count;
    *has_ownership = true;
  }

  // We can only read what's available and what we have capacity for. Minimise the component count
  component_count = std::min(component_count, static_cast<uint8_t>(buffer.componentCount()));
  switch (packet_type)
  {
  case DctInt8:
    return readAs<int8_t>(packet, offset, count, component_count, stream_ptr);
  case DctUInt8:
    return readAs<uint8_t>(packet, offset, count, component_count, stream_ptr);
  case DctInt16:
    return readAs<int16_t>(packet, offset, count, component_count, stream_ptr);
  case DctUInt16:
    return readAs<uint16_t>(packet, offset, count, component_count, stream_ptr);
  case DctInt32:
    return readAs<int32_t>(packet, offset, count, component_count, stream_ptr);
  case DctUInt32:
    return readAs<uint32_t>(packet, offset, count, component_count, stream_ptr);
  case DctInt64:
    return readAs<int64_t>(packet, offset, count, component_count, stream_ptr);
  case DctUInt64:
    return readAs<uint64_t>(packet, offset, count, component_count, stream_ptr);
  case DctFloat32:
    return readAs<float>(packet, offset, count, component_count, stream_ptr);
  case DctFloat64:
    return readAs<double>(packet, offset, count, component_count, stream_ptr);
  case DctPackedFloat16:
    return readAsPacked<float, int16_t>(packet, offset, count, component_count, stream_ptr);
  case DctPackedFloat32:
    return readAsPacked<double, int32_t>(packet, offset, count, component_count, stream_ptr);
  default:
    // Throw?
    return 0;
  }
}

template <typename DST, typename SRC>
inline size_t affordanceCopy(DST *dst, size_t dst_capacity, const SRC *src,
                             size_t src_component_count, size_t src_element_stride,
                             size_t src_element_count, size_t component_read_count,
                             size_t src_elemment_index, size_t src_component_start)
{
  size_t wrote = 0;
  component_read_count = std::min(dst_capacity, component_read_count);
  for (size_t e = 0; e + src_elemment_index < src_element_count && wrote < component_read_count;
       ++e)
  {
    for (size_t c = src_component_start; c < src_component_count && wrote < component_read_count;
         ++c)
    {
      // NOLINTNEXTLINE(bugprone-signed-char-misuse)
      dst[wrote++] = static_cast<DST>(src[(src_elemment_index + e) * src_element_stride + c]);
    }
    src_component_start = 0;
  }

  return wrote;
}

template <typename T>
size_t DataBufferAffordancesT<T>::get(DataStreamType as_type, size_t element_index,
                                      size_t component_index, size_t component_read_count,
                                      const void *stream, size_t stream_element_count,
                                      size_t stream_component_count, size_t stream_element_stride,
                                      void *dst, size_t dst_capacity) const
{
  if (element_index >= stream_element_count ||
      element_index == stream_element_count && component_index >= stream_component_count ||
      component_read_count == 0)
  {
    return 0;
  }

  // Clamp the read count.
  const size_t element_read_count =
    std::max<size_t>(component_read_count / stream_component_count, 1u);
  component_read_count =
    std::min(component_read_count, element_read_count * stream_component_count);

  const T *src = static_cast<const T *>(stream);
  size_t read_component_count = 0;
  switch (as_type)
  {
  case DctInt8:
    read_component_count = affordanceCopy(
      static_cast<int8_t *>(dst), dst_capacity, src, stream_component_count, stream_element_stride,
      stream_element_count, component_read_count, element_index, component_index);
    break;
  case DctUInt8:
    read_component_count = affordanceCopy(
      static_cast<uint8_t *>(dst), dst_capacity, src, stream_component_count, stream_element_stride,
      stream_element_count, component_read_count, element_index, component_index);
    break;
  case DctInt16:
    read_component_count = affordanceCopy(
      static_cast<int16_t *>(dst), dst_capacity, src, stream_component_count, stream_element_stride,
      stream_element_count, component_read_count, element_index, component_index);
    break;
  case DctUInt16:
    read_component_count =
      affordanceCopy(static_cast<uint16_t *>(dst), dst_capacity, src, stream_component_count,
                     stream_element_stride, stream_element_count, component_read_count,
                     element_index, component_index);
    break;
  case DctInt32:
    read_component_count = affordanceCopy(
      static_cast<int32_t *>(dst), dst_capacity, src, stream_component_count, stream_element_stride,
      stream_element_count, component_read_count, element_index, component_index);
    break;
  case DctUInt32:
    read_component_count =
      affordanceCopy(static_cast<uint32_t *>(dst), dst_capacity, src, stream_component_count,
                     stream_element_stride, stream_element_count, component_read_count,
                     element_index, component_index);
    break;
  case DctInt64:
    read_component_count = affordanceCopy(
      static_cast<int64_t *>(dst), dst_capacity, src, stream_component_count, stream_element_stride,
      stream_element_count, component_read_count, element_index, component_index);
    break;
  case DctUInt64:
    read_component_count =
      affordanceCopy(static_cast<uint64_t *>(dst), dst_capacity, src, stream_component_count,
                     stream_element_stride, stream_element_count, component_read_count,
                     element_index, component_index);
    break;
  case DctFloat32:
    read_component_count = affordanceCopy(
      static_cast<float *>(dst), dst_capacity, src, stream_component_count, stream_element_stride,
      stream_element_count, component_read_count, element_index, component_index);
    break;
  case DctFloat64:
    read_component_count = affordanceCopy(
      static_cast<double *>(dst), dst_capacity, src, stream_component_count, stream_element_stride,
      stream_element_count, component_read_count, element_index, component_index);
    break;
  default:
    TES_THROW(Exception("Unsupported vertex stream read type"), false);
  }

  return read_component_count;
}

template <typename T>
template <typename ReadType>
uint32_t DataBufferAffordancesT<T>::readAs(PacketReader &packet, unsigned offset, unsigned count,
                                           unsigned component_count, void **stream_ptr) const
{
  T *dst = static_cast<T *>(*stream_ptr);
  dst += offset * component_count;
  ReadType read_value = {};

  for (unsigned i = 0; i < count; ++i)
  {
    for (unsigned j = 0; j < component_count; ++j)
    {
      if (packet.readElement(read_value) != sizeof(read_value))
      {
        return 0;
      }
      // NOLINTNEXTLINE(bugprone-signed-char-misuse)
      dst[j] = static_cast<T>(read_value);
    }
    dst += component_count;
  }

  return count;
}

template <typename T>
template <typename FloatType, typename ReadType>
uint32_t DataBufferAffordancesT<T>::readAsPacked(PacketReader &packet, unsigned offset,
                                                 unsigned count, unsigned component_count,
                                                 void **stream_ptr) const
{
  // First read the packing origin.
  std::vector<FloatType> origin(component_count);

  bool ok = true;
  FloatType quantisation_unit = 1;
  ok = packet.readElement(quantisation_unit) == sizeof(quantisation_unit) && ok;
  ok = packet.readArray(origin) == component_count && ok;

  if (!ok)
  {
    return 0;
  }

  T *dst = static_cast<T *>(*stream_ptr);
  dst += offset * component_count;

  for (unsigned i = 0; i < count; ++i)
  {
    for (unsigned j = 0; j < component_count; ++j)
    {
      ReadType read_value;
      if (packet.readElement(read_value) != sizeof(read_value))
      {
        return 0;
      }
      dst[j] = static_cast<T>(read_value * quantisation_unit + origin[j]);
    }
    dst += component_count;
  }

  return count;
}
}  // namespace detail
}  // namespace tes

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
