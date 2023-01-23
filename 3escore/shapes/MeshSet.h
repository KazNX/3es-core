//
// author: Kazys Stepanas
//
#ifndef _3ESMESH_H_
#define _3ESMESH_H_

#include <3escore/CoreConfig.h>

#include "IntArg.h"
#include "Matrix4.h"
#include "Shape.h"

#include <cstdint>

namespace tes
{
class MeshResource;

/// Represents a mesh shape. Requires a @c MeshResource parts to get represent mesh topology.
/// The shape never owns the @c MeshResource parts and they must outlive the shape.
class TES_CORE_API MeshSet : public Shape
{
public:
  /// Create a shape with a @c partCount parts. Use @c setPart() to populate.
  /// @param partCount The number of parts to the mesh.
  /// @param id The unique mesh shape ID, zero for transient (not recommended for mesh shapes).
  /// @param category The mesh shape category.
  MeshSet(const Id &id = Id(), const UIntArg &partCount = 0);
  /// Create a shape with a single @p part with transform matching the shape transform.
  /// @param part The mesh part.
  /// @param id The unique mesh shape ID, zero for transient (not recommended for mesh shapes).
  /// @param category The mesh shape category.
  MeshSet(const MeshResource *part, const Id &id = Id());

  /// Copy constructor
  /// @param other Object to copy.
  MeshSet(const MeshSet &other);

  /// Move constructor.
  /// @param other Object to move.
  MeshSet(MeshSet &&other);

  /// Destructor.
  ~MeshSet();

  inline const char *type() const override { return "meshSet"; }

  /// Get the number of parts to this shape.
  /// @return The number of parts this shape has.
  unsigned partCount() const;
  /// Set the part at the given index.
  /// @param index The part index to set. Must be in the range <tt>[0, partCount())</tt>.
  /// @param part The mesh data to set at @p index.
  /// @param transform The transform for this part, relative to this shape's transform.
  ///     This transform may not be updated after the shape is sent to a client.
  /// @param colour Tint to apply just to this part.
  void setPart(const UIntArg &index, const MeshResource *part, const Transform &transform,
               const Colour &colour = Colour(255, 255, 255));
  /// Fetch the part resource at the given @p index.
  /// @param index The part index to fetch. Must be in the range <tt>[0, partCount())</tt>.
  /// @return The mesh at the given index.
  const MeshResource *partResource(const UIntArg &index) const;
  /// Fetch the transform for the part at the given @p index.
  /// @param index The part transform to fetch. Must be in the range <tt>[0, partCount())</tt>.
  /// @return The transform for the mesh at the given index.
  const Transform &partTransform(const UIntArg &index) const;
  /// Fetch the colour tint for the part at the given @p index.
  /// @param index The part transform to fetch. Must be in the range <tt>[0, partCount())</tt>.
  /// @return The colour tint of mesh at the given index.
  const Colour &partColour(const UIntArg &index) const;

  /// Overridden to include the number of mesh parts, their IDs and transforms.
  bool writeCreate(PacketWriter &stream) const override;

  /// Reads the @c CreateMessage and details about the mesh parts.
  ///
  /// Successfully reading the message modifies the data in this shape such
  /// that the parts (@c partResource()) are only dummy resources
  /// (@c MeshPlaceholder). This identifies the resource IDs, but the data
  /// must be resolved separately.
  ///
  /// @param stream The stream to read from.
  /// @return @c true on success.
  bool readCreate(PacketReader &stream) override;

  /// Enumerate the mesh resources for this shape.
  /// @todo Add material resources.
  unsigned enumerateResources(const Resource **resources, unsigned capacity, unsigned fetchOffset = 0) const override;

  /// Clone the mesh shape. @c MeshResource objects are shared.
  /// @return The cloned shape.
  Shape *clone() const override;

protected:
  void onClone(MeshSet *copy) const;

private:
  void cleanupParts();

  struct Part
  {
    const MeshResource *resource = nullptr;
    Transform transform = Transform::identity();
    Colour colour = Colour(255, 255, 255);
  };

  Part *_parts;
  unsigned _partCount;
  bool _ownPartResources;
};

inline unsigned MeshSet::partCount() const
{
  return _partCount;
}

inline void MeshSet::setPart(const UIntArg &index, const MeshResource *part, const Transform &transform,
                             const Colour &colour)
{
  _parts[index.i].resource = part;
  _parts[index.i].transform = transform;
  _parts[index.i].colour = colour;
}

inline const MeshResource *MeshSet::partResource(const UIntArg &index) const
{
  return _parts[index.i].resource;
}

inline const Transform &MeshSet::partTransform(const UIntArg &index) const
{
  return _parts[index.i].transform;
}

inline const Colour &MeshSet::partColour(const UIntArg &index) const
{
  return _parts[index.i].colour;
}
}  // namespace tes

#endif  // _3ESMESH_H_
