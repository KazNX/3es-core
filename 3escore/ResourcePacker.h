//
// author: Kazys Stepanas
//
#pragma once

#include "CoreConfig.h"

#include "Ptr.h"

#include <cstdint>
#include <memory>

namespace tes
{
class Resource;
struct TransferProgress;
class PacketWriter;

/// The @c ResourcePacker is a utility class to help create and transfer data associated with a @c
/// Resource. The
/// @c ResourcePacker keeps track of the progress for transferring a @c Resource and write data to a
/// @c PacketWriter so long as there are data packets remaining for the @c Resource.
///
/// Typical usage is as follows;
/// - Instantiate a @c ResourcePacker
/// - Call @c transfer() with the @c Resource to pack to initialise the @c ResourcePacker for this
/// @c Resource.
/// - While @c isNull() is @c false
///   - Call @c nextPacket() with an appropriately sized @c PacketWriter. The byte limit of the
///   writer is respected.
///   - Finalise and send the packet.
///
/// @c isNull() will return @c true once all packets have been generated and there is no more data
/// to pack for the
/// @c Resource. Note the @c Resource must outlive the packing process.
class TES_CORE_API ResourcePacker
{
public:
  using ResourcePtr = Ptr<const Resource>;

  /// Constructor.
  ResourcePacker();
  ResourcePacker(const ResourcePacker &other) = delete;
  /// Destructor.
  ~ResourcePacker();

  ResourcePacker &operator=(const ResourcePacker &other) = delete;

  /// Query the current resource being packed (if any).
  /// @return The current resource.
  [[nodiscard]] const ResourcePtr &resource() const { return _resource; }
  /// Is there a current @c Resource being packed?
  /// @return True if @c resource() is null.
  [[nodiscard]] bool isValid() const { return _resource != nullptr; }

  /// Initiate transfer/packing of @p resource.
  /// @param resource The resource to start packing.
  void transfer(ResourcePtr resource);

  /// Cancel packing of the current @c Resource. This releases the current resource and resets
  /// progress. Note that @c lastCompletedId() will not change.
  void cancel();

  /// Query the @c Resource::uniqueKey() of the last @c Resource packed. This is set after the past
  /// packet is generated in @c nextPacket() and the resource is released.
  /// @return The @c Resource::uniqueKey() of the last @c Resource completed.
  [[nodiscard]] uint64_t lastCompletedId() const { return _last_completed_id; }

  /// Populate the next packet for the current @c Resource.
  ///
  /// The first call to this function invokes @c Resource::create() on the current @c Resource.
  /// Subsequent calls invoke @c Resource::transfer() until the final packet is generated. Once
  /// there are no more packets to generate, the @c lastCompletedId() is set to the
  /// @c Resource::uniqueKey() and the @c Resource pointer is cleared
  /// (@c isValid() becomes false). The @c Resource pointer is also cleared on failure.
  ///
  /// @param packet The packet to write to.
  /// @param byte_limit The maximum number of bytes to write in a single @p packet.
  /// @return True if @p packet has been successfully populated (though not finalised) and @c
  /// nextPacket() should be called again. False if there is no current resource or @c Resource
  /// methods fail to populate the @p packet.
  [[nodiscard]] bool nextPacket(PacketWriter &packet, unsigned byte_limit);

private:
  ResourcePtr _resource;                        ///< Current resource pointer (borrowed or shared).
  std::unique_ptr<TransferProgress> _progress;  ///< Progress tracker for resource packing.
  uint64_t _last_completed_id = 0;              ///< Last completed resource key.
  bool _started = false;                        ///< Has the current resource been started?
};
}  // namespace tes
