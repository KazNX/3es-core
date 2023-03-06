//
// Author: Kazys Stepanas
//
#include "Category.h"

#include <3escore/Connection.h>
#include <3escore/Log.h>
#include <3escore/Messages.h>
#include <3escore/PacketWriter.h>

#include <array>

namespace tes::view::handler
{
Category::Category()
  : Message(MtCategory, "category")
{}


void Category::initialise()
{}

void Category::reset()
{
  std::lock_guard guard(_mutex);
  _categories.clear();
}


void Category::prepareFrame(const FrameStamp &stamp)
{
  (void)stamp;
}


void Category::endFrame(const FrameStamp &stamp)
{
  (void)stamp;
  std::lock_guard guard(_mutex);
  for (const auto &info : _pending)
  {
    _categories.updateCategory(info);
  }
  _pending.clear();
}


void Category::draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params,
                    const painter::CategoryState &categories)
{
  TES_UNUSED(pass);
  TES_UNUSED(stamp);
  TES_UNUSED(params);
  TES_UNUSED(categories);
}


void Category::readMessage(PacketReader &reader)
{
  bool ok = false;
  switch (reader.messageId())
  {
  case CategoryNameMessage::MessageId: {
    CategoryNameMessage msg;
    std::array<char, 8 * 1024u> name;
    ok = msg.read(reader, name.data(), name.size());
    if (ok)
    {
      painter::CategoryInfo info = {};
      info.name = msg.name;
      info.id = msg.category_id;
      info.parent_id = msg.parent_id;
      info.default_active = msg.default_active != 0;
      info.active = info.default_active;
      _pending.emplace_back(info);
    }

    if (!ok)
    {
      log::error("Failed to decode category message.");
    }
    break;
  }
  default:
    log::error("Unsupported category message ID: ", reader.messageId());
  }
}


void Category::serialise(Connection &out, ServerInfoMessage &info)
{
  (void)info;
  std::lock_guard guard(_mutex);
  CategoryNameMessage msg = {};
  const std::string error_str = "<error>";
  bool ok = true;

  const uint16_t buffer_size = 1024u;
  std::vector<uint8_t> packet_buffer(buffer_size, 0u);
  PacketWriter writer(packet_buffer.data(), buffer_size);
  for (auto &[id, info] : _categories.map())
  {
    msg.category_id = info.id;
    msg.parent_id = info.parent_id;
    if (info.name.length() < std::numeric_limits<decltype(msg.name_length)>::max())
    {
      msg.name = info.name.c_str();
      msg.name_length = uint16_t(info.name.size());
    }
    else
    {
      msg.name = error_str.c_str();
      msg.name_length = uint16_t(error_str.size());
    }
    msg.default_active = (info.default_active) ? 1 : 0;

    writer.reset(routingId(), CategoryNameMessage::MessageId);
    ok = msg.write(writer) && ok;
    ok = writer.finalise() && ok;
    ok = out.send(writer) >= 0 && ok;
  }

  if (!ok)
  {
    log::error("Category serialisation failed.");
  }
}
}  // namespace tes::view::handler
