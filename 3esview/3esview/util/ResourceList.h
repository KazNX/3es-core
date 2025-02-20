#pragma once

#include <3esview/ViewConfig.h>

#include <3escore/Log.h>

#include <atomic>
#include <mutex>
#include <utility>
#include <vector>

namespace tes::view::util
{
using ResourceListId = size_t;
/// A @c ResourceList marker value for null items. Internally used to identify the end of the free
/// list or other linked list structures.
inline constexpr ResourceListId kNullResource = ~ResourceListId(0u);
/// A @c ResourceList marker value used for items which are currently allocated.
inline constexpr ResourceListId kAllocatedResource = ~ResourceListId(0u) - 1;

/// A resource list is a container which assigns items from it's internal buffer - resources - for
/// external usage.
///
/// Such resource items may be released back to the @c ResourceList where they are added to a free
/// item list and may be used in future resource assignments.
///
/// Resources are assigned by @c Id and such an id must be dereferenced every time a resource item
/// is to be accessed. This is because allocating new resource may reallocate the internal buffer
/// invalidating any resources currently held externally to this class.
///
/// A @c ResourceRef can be used as a kind of resource lock which ensures the @c ResourceList cannot
/// invalidate items. As such a @c ResourceRef must be short lived and no new resources can be
/// assigned while at least one @c ResourceRef is held.
///
/// @tparam T The resource type.
template <typename T>
class ResourceList
{
public:
  /// The type used to identify resources. This maps to indices in the items list.
  using Id = ResourceListId;

  class iterator;
  class const_iterator;

  /// Represents a transient reference to an item in the @c ResourceList .
  ///
  /// @c ResourceRefBase objects are obtained via @c allocate() and @c Id indexing functions ensures
  /// that the resource remains valid for the lifespan on the @c ResourceRefBase object. This
  /// includes locking the @c ResourceList for the current thread, thus only one thread at a time
  /// can hold any @c ResourceRefBase objects at a time.
  ///
  /// The resource should only be accessed using @c * and @c -> operators as these accessors remain
  /// valid even if
  /// @c allocate() causes the resource list to reallocate.
  ///
  /// @note A @c ResourceList must outlive all its @c ResourceRefBase objects.
  template <typename Item, typename List>
  class ResourceRefBase
  {
  public:
    /// Default constructor: the resulting object is not valid.
    ResourceRefBase() = default;
    /// Construct a resource for the given @p id and @p resource_list .
    /// @param id The resource Id.
    /// @param resource_list The resource list which we are referencing into.
    ResourceRefBase(Id id, List *resource_list)
      : _id(id)
      , _resource_list(resource_list)
    {
      // Only hold a resource list if the id is valid.
      if (_id != kNullResource)
      {
        _resource_list->lock();
      }
      else
      {
        _resource_list = nullptr;
      }
    }
    ResourceRefBase(const ResourceRefBase<Item, List> &) = delete;
    /// Move constructor.
    /// @param other Object to move.
    ResourceRefBase(ResourceRefBase<Item, List> &&other) noexcept
      : _id(std::exchange(other._id, kNullResource))
      , _resource_list(std::exchange(other._resource_list, nullptr))
    {}

    /// Releases the resource reference, releasing a @c ResourceList lock.
    ~ResourceRefBase() { release(); }

    ResourceRefBase &operator=(const ResourceRefBase<Item, List> &) = delete;
    /// Move assignment.
    /// @param other Object to move; can match @c this .
    /// @return @c *this
    ResourceRefBase &operator=(ResourceRefBase<Item, List> &&other) noexcept
    {
      if (this != &other)
      {
        std::swap(other._id, _id);
        std::swap(other._resource_list, _resource_list);
      }
      return *this;
    }

    /// Check if this resource reference is valid. A valid reference has a valid @c Id and addresses
    /// a @c ResourceList .
    /// @return
    [[nodiscard]] bool isValid() const
    {
      return _resource_list != nullptr &&
             _resource_list->_items[_id].next_free == kAllocatedResource;
    }

    /// Dereference the resource.
    /// @return The references resource entry.
    [[nodiscard]] const Item &operator*() const { return _resource_list->_items[_id].resource; }
    /// Dereference the resource.
    /// @return The references resource entry.
    [[nodiscard]] const Item *operator->() const { return &_resource_list->_items[_id].resource; }

    /// Get the resource entry @c Id . This can be stored in order to later access the resource via
    /// @c ResourceList indexing functions.
    /// @return The resource Id.
    [[nodiscard]] Id id() const { return _id; }

    /// Explicitly release the current resource (if any). Safe to call if not valid.
    void release()
    {
      if (_resource_list)
      {
        _id = kNullResource;
        _resource_list->unlock();
        _resource_list = nullptr;
      }
    }

  protected:
    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    Id _id = kNullResource;
    List *_resource_list = nullptr;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
  };

  class ResourceRef final : public ResourceRefBase<T, ResourceList<T>>
  {
  public:
    using Super = ResourceRefBase<T, ResourceList<T>>;

    /// Default constructor: the resulting object is not valid.
    ResourceRef() = default;
    /// Construct a resource for the given @p id and @p resource_list .
    /// @param id The resource Id.
    /// @param resource_list The resource list which we are referencing into.
    ResourceRef(Id id, ResourceList<T> *resource_list)
      : Super(id, resource_list)
    {}

    ResourceRef(const ResourceRef &other) = delete;
    /// Move constructor.
    /// @param other Object to move.
    ResourceRef(ResourceRef &&other) noexcept
      : Super(std::move(other))
    {}

    ~ResourceRef() = default;

    ResourceRef &operator=(const ResourceRef &other) = delete;
    ResourceRef &operator=(ResourceRef &&other) noexcept
    {
      if (this != &other)
      {
        std::swap(other._id, Super::_id);
        std::swap(other._resource_list, Super::_resource_list);
      }
      return *this;
    }

    /// Dereference the resource.
    /// @return The references resource entry.
    [[nodiscard]] T &operator*() { return Super::_resource_list->_items[Super::_id].resource; }
    [[nodiscard]] T *operator->() { return &Super::_resource_list->_items[Super::_id].resource; }
  };

  using ResourceConstRef = ResourceRefBase<const T, const ResourceList<T>>;

  /// Construct a resource list optionally specifying the initial capacity.
  /// @param capacity The initial resource capacity.
  ResourceList(size_t capacity = 0);
  ResourceList(const ResourceList &other) = delete;
  /// Destructor
  ~ResourceList() noexcept;

  ResourceList &operator=(const ResourceList &other) = delete;

  [[nodiscard]] iterator begin() { return iterator(this, firstValid()); }
  [[nodiscard]] iterator end() { return iterator(this, kNullResource); }
  [[nodiscard]] const_iterator begin() const { return const_iterator(this, firstValid()); }
  [[nodiscard]] const_iterator end() const { return const_iterator(this, kNullResource); }

  /// Allocate a new resource.
  ///
  /// The @c Id from the @c ResourceRef return value should be stored for later release.
  ///
  /// @return A resource reference to the allocated item.
  ResourceRef allocate();

  /// Access the item at the given @p id .
  ///
  /// Raises a @c std::runtime_error if @p id does not reference a valid item.
  ///
  /// @param id The @c Id of the item to reference.
  /// @return The reference item.
  ResourceRef at(Id id);
  ResourceConstRef at(Id id) const;

  /// Release the item at the given @p id .
  /// @param id The @c Id of the item to release.
  void release(Id id);

  /// Access the item at the given @p id with undefined behaviour if @p id is invalid.
  /// @param id The @c Id of the item to reference.
  /// @return The reference item.
  ResourceRef operator[](Id id);
  /// @overload
  ResourceConstRef operator[](Id id) const;

  /// Return the number of allocated items.
  /// @return
  size_t size() const { return _item_count; }

  /// Release all resources. Raises a @c std::runtime_error if there are outstanding references.
  void clear();

  template <typename R>
  class BaseIterator
  {
  public:
    using ResourceListT = R;
    BaseIterator() = default;
    BaseIterator(ResourceListT *owner, ResourceListId id)
      : _owner(owner)
      , _id(id)
    {
      if (owner)
      {
        owner->lock();
      }
    }

    ~BaseIterator()
    {
      if (_owner)
      {
        _owner->unlock();
      }
    }

    BaseIterator(const BaseIterator &other)
      : BaseIterator(other._owner, other._id)
    {}
    BaseIterator(BaseIterator &&other) noexcept
      : _owner(std::exchange(other._owner, nullptr))
      , _id(std::exchange(other._id, kNullResource))
    {}

    BaseIterator &operator=(  // NOLINT(bugprone-unhandled-self-assignment)
      const BaseIterator &other)
    {
      // Why does clang-tidy think self assignment isn't properly handled?
      BaseIterator(other).swap(*this);
      return *this;
    }
    BaseIterator &operator=(BaseIterator &&other) noexcept
    {
      _owner = std::exchange(other._owner, nullptr);
      _id = std::exchange(other._id, kNullResource);
      return *this;
    }

    [[nodiscard]] ResourceListT *owner() const { return _owner; }
    [[nodiscard]] Id id() const { return _id; }

  protected:
    void swap(BaseIterator &other)
    {
      using std::swap;
      swap(_owner, other._owner);
      swap(_id, other._id);
    }

    void next()
    {
      // Not happy with how clunky the next/prev implementations are. The compiler will probably
      // sort it out, but it should be nicer.
      if (_owner && _id != kNullResource && _id + 1 < _owner->_items.size())
      {
        do
        {
          ++_id;
        } while (_id < _owner->_items.size() && _id != kNullResource &&
                 _owner->_items[_id].next_free != kAllocatedResource);
        _id = (_id < _owner->_items.size()) ? _id : kNullResource;
      }
      else
      {
        _id = kNullResource;
      }
    }

    void prev()
    {
      if (_owner && _id > 0 && _id != kNullResource)
      {
        do
        {
          --_id;
        } while (_id < _owner->_items.size() && _id != kNullResource &&
                 _owner->_items[_id].next_free != kAllocatedResource);
        // Note: we let underflow deal with setting id to kNullResource as we reach the end of
        // iteration. Let's assert that's valid though.
        static_assert(decltype(_id)(0) - decltype(_id)(1) == kNullResource);
      }
      else
      {
        _id = kNullResource;
      }
    }

    // NOLINTBEGIN(cppcoreguidelines-non-private-member-variables-in-classes)
    ResourceListT *_owner = nullptr;
    Id _id = kNullResource;
    // NOLINTEND(cppcoreguidelines-non-private-member-variables-in-classes)
  };

  class iterator : public BaseIterator<ResourceList<T>>  // NOLINT(readability-identifier-naming)
  {
  public:
    using Super = BaseIterator<ResourceList<T>>;

    iterator() {}
    iterator(ResourceList<T> *owner, Id id)
      : BaseIterator<ResourceList<T>>(owner, id)
    {}
    iterator(const iterator &other) = default;
    iterator(iterator &&other) noexcept = default;

    ~iterator() = default;

    /// Dereference the resource.
    /// @return The references resource entry.
    [[nodiscard]] T &operator*() { return Super::_owner->_items[Super::_id].resource; }
    /// @overload
    [[nodiscard]] const T &operator*() const { return Super::_owner->_items[Super::_id].resource; }
    /// Dereference the resource.
    /// @return The references resource entry.
    [[nodiscard]] T *operator->() { return &Super::_owner->_items[Super::_id].resource; }
    /// @overload
    [[nodiscard]] const T *operator->() const
    {
      return &Super::_owner->_items[Super::_id].resource;
    }

    iterator &operator=(const iterator &other) = default;
    iterator &operator=(iterator &&other) noexcept = default;

    template <typename R>
    [[nodiscard]] bool operator==(const BaseIterator<R> &other) const
    {
      return Super::_owner == other.owner() && Super::_id == other.id();
    }

    template <typename R>
    [[nodiscard]] bool operator!=(const BaseIterator<R> &other) const
    {
      return !operator==(other);
    }

    iterator &operator++()
    {
      Super::next();
      return *this;
    }

    iterator operator++(int)
    {
      iterator current = *this;
      Super::next();
      return current;
    }

    iterator &operator--()
    {
      Super::prev();
      return *this;
    }

    iterator operator--(int)
    {
      iterator current = *this;
      Super::prev();
      return current;
    }

    friend class const_iterator;
  };

  class const_iterator  // NOLINT(readability-identifier-naming)
    : public BaseIterator<const ResourceList<T>>
  {
  public:
    using Super = BaseIterator<const ResourceList<T>>;

    const_iterator() = default;
    const_iterator(const ResourceList<T> *owner, Id id)
      : BaseIterator<const ResourceList<T>>(owner, id)
    {}
    const_iterator(const iterator &other)
      : BaseIterator<const ResourceList<T>>(other._owner, other._id)
    {}
    const_iterator(const const_iterator &other) = default;
    const_iterator(const_iterator &&other) noexcept = default;

    ~const_iterator() = default;

    /// Dereference the resource.
    /// @return The references resource entry.
    [[nodiscard]] const T &operator*() const { return Super::_owner->_items[Super::_id].resource; }
    /// Dereference the resource.
    /// @return The references resource entry.
    [[nodiscard]] const T *operator->() const
    {
      return &Super::_owner->_items[Super::_id].resource;
    }

    const_iterator &operator=(const iterator &other)
    {
      Super::_owner = other._owner;
      Super::_id = other._id;
      return *this;
    }
    const_iterator &operator=(const const_iterator &other) = default;
    const_iterator &operator=(const_iterator &&other) noexcept = default;

    template <typename R>
    bool operator==(const BaseIterator<R> &other) const
    {
      return Super::_owner == other.owner() && Super::_id == other.id();
    }

    template <typename R>
    bool operator!=(const BaseIterator<R> &other) const
    {
      return !operator==(other);
    }

    const_iterator &operator++()
    {
      Super::next();
      return *this;
    }

    const_iterator operator++(int)
    {
      const_iterator current = *this;
      Super::next();
      return current;
    }

    const_iterator &operator--()
    {
      Super::prev();
      return *this;
    }

    const_iterator operator--(int)
    {
      const_iterator current = *this;
      Super::prev();
      return current;
    }
  };

private:
  friend ResourceRef;
  friend iterator;
  friend const_iterator;

  [[nodiscard]] Id firstValid() const
  {
    const std::scoped_lock<decltype(_lock)> guard(_lock);
    for (Id id = 0; id < _items.size(); ++id)
    {
      const auto &item = _items[id];
      if (item.next_free == kAllocatedResource)
      {
        return id;
      }
    }

    return kNullResource;
  }

  void lock() const
  {
    _lock.lock();
    ++_lock_count;
  }
  void unlock() const
  {
    --_lock_count;
    _lock.unlock();
  }

  struct Item
  {
    T resource;
    Id next_free;
  };

  std::vector<Item> _items = {};
  mutable std::recursive_mutex _lock = {};
  mutable std::atomic_uint32_t _lock_count = 0;
  std::atomic_size_t _item_count = 0;
  Id _free_head = kNullResource;
  Id _free_tail = kNullResource;
};


template <typename T>
ResourceList<T>::ResourceList(size_t capacity)
{
  if (capacity)
  {
    _items.reserve(capacity);
  }
}


template <typename T>
ResourceList<T>::~ResourceList() noexcept
{
  if (_lock_count > 0)
  {
    log::fatal("Deleting resource list with outstanding resource references");
  }
}


template <typename T>
typename ResourceList<T>::ResourceRef ResourceList<T>::allocate()
{
  const std::unique_lock<decltype(_lock)> guard(_lock);
  // Try free list first.
  if (_free_head != kNullResource)
  {
    ResourceRef resource(_free_head, this);
    if (_free_head != _free_tail)
    {
      _free_head = _items[_free_head].next_free;
    }
    else
    {
      _free_tail = _free_head = kNullResource;
    }
    _items[resource.id()].next_free = kAllocatedResource;
    ++_item_count;
    return resource;
  }

  if (_items.size() == kAllocatedResource)
  {
    throw std::runtime_error("Out of resources");
  }

  // Grow the container.
  _items.emplace_back(Item{ T{}, kAllocatedResource });
  ++_item_count;
  return ResourceRef(_items.size() - 1u, this);
}


template <typename T>
typename ResourceList<T>::ResourceRef ResourceList<T>::at(Id id)
{
  if (id < _items.size() && _items[id].next_free == kAllocatedResource)
  {
    return ResourceRef(id, this);
  }
  return ResourceRef(kNullResource, this);
}


template <typename T>
typename ResourceList<T>::ResourceConstRef ResourceList<T>::at(Id id) const
{
  if (id < _items.size() && _items[id].next_free == kAllocatedResource)
  {
    return ResourceConstRef(id, this);
  }
  return ResourceConstRef(kNullResource, this);
}


template <typename T>
void ResourceList<T>::release(Id id)
{
  const std::unique_lock<decltype(_lock)> guard(_lock);
  if (_free_head != kNullResource)
  {
    // Append to the free list tail.
    _items[_free_tail].next_free = id;
    _free_tail = id;
  }
  else
  {
    // First free item.
    _free_head = _free_tail = id;
  }
  --_item_count;
  _items[id].next_free = kNullResource;
}


template <typename T>
typename ResourceList<T>::ResourceRef ResourceList<T>::operator[](Id id)
{
  return ResourceRef(id, this);
}


template <typename T>
typename ResourceList<T>::ResourceConstRef ResourceList<T>::operator[](Id id) const
{
  return ResourceConstRef(id, this);
}


template <typename T>
void ResourceList<T>::clear()
{
  std::unique_lock<decltype(_lock)> guard(_lock);
  if (_lock_count > 0)
  {
    throw std::runtime_error("Deleting resource list with outstanding resource references");
  }
  _items.clear();
  _free_head = _free_tail = kNullResource;
  _item_count = 0;
}
}  // namespace tes::view::util
