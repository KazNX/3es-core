#include "Shape.h"

#include <3esview/painter/ShapePainter.h>

#include <3escore/Colour.h>
#include <3escore/Connection.h>
#include <3escore/Debug.h>
#include <3escore/Log.h>
#include <3escore/PacketReader.h>

#include <3escore/shapes/MultiShape.h>

namespace tes::view::handler
{
[[nodiscard]] bool readMultiShape(const Shape &shape, painter::ShapePainter &painter,
                                  const painter::ShapePainter::ParentId &parent_id,
                                  painter::ShapePainter::Type draw_type, unsigned shape_count,
                                  PacketReader &reader, bool double_precision)
{
  Shape::ObjectAttributes multi_attrs = {};
  for (unsigned i = 0; i < shape_count; ++i)
  {
    if (!multi_attrs.read(reader, double_precision))
    {
      log::error(shape.name(), " : failed to read multi shape part");
      return false;
    }
    auto transform = shape.composeTransform(multi_attrs);
    const Colour c(multi_attrs.colour);
    painter.addChild(parent_id, draw_type, transform,
                     Magnum::Color4(c.rf(), c.gf(), c.bf(), c.af()));
  }
  return true;
}


Shape::Shape(uint16_t routing_id, const std::string &name,
             std::shared_ptr<painter::ShapePainter> painter)
  : Message(routing_id, name)
  , _painter(std::exchange(painter, nullptr))
{}


void Shape::initialise()
{}


void Shape::reset()
{
  _painter->reset();
}


void Shape::prepareFrame(const FrameStamp &stamp)
{
  (void)stamp;
}


void Shape::endFrame(const FrameStamp &stamp)
{
  (void)stamp;
  _painter->commit();
}


void Shape::draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params,
                 const painter::CategoryState &categories)
{
  switch (pass)
  {
  case DrawPass::Opaque:
    _painter->drawOpaque(stamp, params.projection_matrix, params.view_matrix, categories);
    break;
  case DrawPass::Transparent:
    _painter->drawTransparent(stamp, params.projection_matrix, params.view_matrix, categories);
    break;
  default:
    break;
  }
}


void Shape::readMessage(PacketReader &reader)
{
  TES_ASSERT(reader.routingId() == routingId());
  ObjectAttributes attrs = {};
  bool ok = false;
  bool logged = false;
  switch (reader.messageId())
  {
  case OIdCreate: {
    CreateMessage msg = {};
    ok = msg.read(reader, attrs) && handleCreate(msg, attrs, reader);
    break;
  }
  case OIdDestroy: {
    DestroyMessage msg = {};
    ok = msg.read(reader);
    if (ok)
    {
      // Note: It's OK to destroy IDs which haven't been created.
      handleDestroy(msg, reader);
    }
    break;
  }
  case OIdUpdate: {
    UpdateMessage msg = {};
    ok = msg.read(reader, attrs) && handleUpdate(msg, attrs, reader);
    break;
  }
  case OIdData: {
    // We only expect data messages for multi-shape messages where the create message does not
    // contain all the shapes.
    DataMessage msg = {};
    ok = msg.read(reader) && handleData(msg, reader);
    break;
  }
  default:
    log::error(name(), " : unhandled shape message type: ", unsigned(reader.messageId()));
    logged = true;
    break;
  }

  if (!ok && !logged)
  {
    log::error(name(), " : failed to decode message type: ", unsigned(reader.messageId()));
  }
}


void Shape::serialise(Connection &out)
{
  const bool serialise_double_precision = sizeof(ObjectAttributes) == sizeof(ObjectAttributesd);
  const auto MultiShapeBlockCount = (serialise_double_precision) ?
                                      tes::MultiShape::BlockCountLimitDouble :
                                      tes::MultiShape::BlockCountLimitSingle;
  std::array<uint8_t, (1u << 16u) - 1> buffer = {};
  PacketWriter writer(buffer.data(), uint16_t(buffer.size()));
  CreateMessage create = {};
  ObjectAttributes attrs = {};

  const std::array<painter::ShapePainter::Type, 3> shape_types = {
    painter::ShapePainter::Type::Solid, painter::ShapePainter::Type::Wireframe,
    painter::ShapePainter::Type::Transparent
  };

  for (auto shape_type : shape_types)
  {
    const auto end = _painter->end(shape_type);
    for (auto shape = _painter->begin(shape_type); shape != end; ++shape)
    {
      if (shape->is_child)
      {
        continue;
      }

      auto transform = shape->attributes.transform;
      auto colour = shape->attributes.colour;

      const auto viewed_id = shape->id;
      create.id = viewed_id.id();
      create.category = shape->id.category();
      create.flags = 0;
      if (shape_type == painter::ShapePainter::Type::Transparent)
      {
        create.flags |= OFTransparent;
      }
      if (shape_type == painter::ShapePainter::Type::Wireframe)
      {
        create.flags |= OFWire;
      }

      if (serialise_double_precision)
      {
        create.flags |= OFDoublePrecision;
      }

      if (shape->child_count)
      {
        create.flags |= OFMultiShape;
      }

      decomposeTransform(transform, attrs);
      attrs.colour = Colour(colour.x(), colour.y(), colour.z(), colour.w()).colour32();

      bool ok = true;
      writer.reset(routingId(), CreateMessage::MessageId);
      ok = create.write(writer, attrs) && ok;

      if (shape->child_count)
      {
        // Handle multi shape
        // Write total number of items.
        const auto child_count = static_cast<uint32_t>(shape->child_count);
        ok = writer.writeElement(child_count) == sizeof(child_count) && ok;
        for (unsigned block_index = 0; block_index < shape->child_count;
             block_index += MultiShapeBlockCount)
        {
          const auto write_child_count =
            std::min<uint16_t>(child_count - block_index, MultiShapeBlockCount);
          // Write block count
          ok = writer.writeElement(write_child_count) == sizeof(write_child_count) && ok;

          for (uint32_t i = 0; ok && i < write_child_count; ++i)
          {
            const auto child_index = block_index + i;
            const auto child_view = shape.getChild(child_index);
            transform = child_view.attributes.transform;
            colour = child_view.attributes.colour;
            decomposeTransform(transform, attrs);
            attrs.colour = Colour(colour.x(), colour.y(), colour.z(), colour.w()).colour32();
            ok = attrs.write(writer, serialise_double_precision) && ok;
          }

          // Write the block and start a new block if more blocks are needed.
          if (block_index + write_child_count < child_count)
          {
            // Not the last block. Write the current message.
            ok = writer.finalise() && ok;
            if (ok)
            {
              out.send(writer.data(), writer.packetSize());
              writer.reset(routingId(), DataMessage::MessageId);
              DataMessage data = {};
              data.id = shape->id.id();
              ok = data.write(writer) && ok;
            }
            else
            {
              log::error("Failed to serialise shapes: ", name());
              break;
            }
          }
        }
      }

      ok = writer.finalise() && ok;
      if (ok)
      {
        out.send(writer.data(), writer.packetSize());
      }
      else
      {
        log::error("Failed to serialise shapes: ", name());
        break;
      }
    }
  }
}


Magnum::Matrix4 Shape::composeTransform(const ObjectAttributes &attrs) const
{
  return Message::composeTransform(attrs);
}


void Shape::decomposeTransform(const Magnum::Matrix4 &transform, ObjectAttributes &attrs) const
{
  Message::decomposeTransform(transform, attrs);
}


bool Shape::handleCreate(const CreateMessage &msg, const ObjectAttributes &attrs,
                         PacketReader &reader)
{
  painter::ShapePainter::Type draw_type = painter::ShapePainter::Type::Solid;

  if (msg.flags & OFTransparent)
  {
    draw_type = painter::ShapePainter::Type::Transparent;
  }
  if (msg.flags & OFWire)
  {
    draw_type = painter::ShapePainter::Type::Wireframe;
  }

  const Id id(msg.id, msg.category);

  if (msg.flags & OFReplace)
  {
    _painter->remove(id);
  }

  auto transform = composeTransform(attrs);
  auto c = Colour(attrs.colour);
  const auto multi_shape = (msg.flags & OFMultiShape) != 0;
  const auto parent_id = _painter->add(id, draw_type, transform,
                                       Magnum::Color4(c.rf(), c.gf(), c.bf(), c.af()), multi_shape);

  bool ok = true;
  if (multi_shape)
  {
    // Multi shape message.
    uint32_t shape_count = 0;   // Total number of expected items.
    uint16_t create_count = 0;  // Current packet items.
    reader.readElement(shape_count);
    reader.readElement(create_count);
    ok = readMultiShape(*this, *_painter, parent_id, draw_type, create_count, reader,
                        (msg.flags & OFDoublePrecision) != 0) &&
         ok;

    const MultiShapeInfo info = { shape_count, (msg.flags & OFDoublePrecision) != 0 };
    if (msg.id)
    {
      _multi_shapes[msg.id] = info;
    }
    else
    {
      _last_transient_multi_shape = info;
    }
  }

  return ok;
}


bool Shape::handleUpdate(const UpdateMessage &msg, const ObjectAttributes &attrs,
                         PacketReader &reader)
{
  (void)reader;
  const Id id(msg.id);
  Magnum::Matrix4 transform = {};
  Magnum::Color4 colour = {};
  auto c = Colour(attrs.colour);
  // Handle partial update.
  if (msg.flags & UFUpdateMode)
  {
    ObjectAttributes cur_attrs = {};
    if (!_painter->readShape(id, transform, colour))
    {
      return false;
    }

    if (msg.flags & (UFPosition | UFRotation | UFScale))
    {
      decomposeTransform(transform, cur_attrs);
      // Decompose the transform.
      if (msg.flags & UFPosition)
      {
        cur_attrs.position[0] = attrs.position[0];
        cur_attrs.position[1] = attrs.position[1];
        cur_attrs.position[2] = attrs.position[2];
      }
      if (msg.flags & UFRotation)
      {
        cur_attrs.rotation[0] = attrs.rotation[0];
        cur_attrs.rotation[1] = attrs.rotation[1];
        cur_attrs.rotation[2] = attrs.rotation[2];
        cur_attrs.rotation[3] = attrs.rotation[3];
      }
      if (msg.flags & UFScale)
      {
        cur_attrs.scale[0] = attrs.scale[0];
        cur_attrs.scale[1] = attrs.scale[1];
        cur_attrs.scale[2] = attrs.scale[2];
      }
      transform = composeTransform(cur_attrs);
    }
    if ((msg.flags & UFColour) == 0)
    {
      c = Colour(cur_attrs.colour);
    }
  }
  else
  {
    transform = composeTransform(attrs);
    colour = Magnum::Color4(c.rf(), c.gf(), c.bf(), c.af());
  }
  _painter->update(id, transform, colour);
  return true;
}


bool Shape::handleDestroy(const DestroyMessage &msg, PacketReader &reader)
{
  (void)reader;
  const Id id(msg.id);

  // Remove multi-shape tracking data.
  auto multi_shape_info = _multi_shapes.find(msg.id);
  if (multi_shape_info != _multi_shapes.end())
  {
    _multi_shapes.erase(multi_shape_info);
  }

  // Remove shape.
  return _painter->remove(id);
}


bool Shape::handleData(const DataMessage &msg, PacketReader &reader)
{
  // Handle multi-shape data messages. Assumed to be the only valid data message.
  bool ok = true;

  MultiShapeInfo info = {};
  if (!Id(msg.id).isTransient())
  {
    auto search = _multi_shapes.find(msg.id);
    if (search == _multi_shapes.end())
    {
      log::error("Received data message for unknown multi-shape id: ", msg.id);
      return false;
    }
    info = search->second;
  }
  else
  {
    info = _last_transient_multi_shape;
  }

  // Things to resolve.
  auto draw_type = painter::ShapePainter::Type::Solid;
  const painter::ShapePainter::ParentId parent_id = _painter->lookup(Id(msg.id), draw_type);

  uint16_t block_count = 0;
  ok = reader.readElement(block_count) == sizeof(block_count) && ok;
  ok = ok && readMultiShape(*this, *_painter, parent_id, draw_type, block_count, reader,
                            info.double_precision);
  return ok;
}
}  // namespace tes::view::handler
