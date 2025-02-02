# TODO

- Add `Span.h` which imports a span header from whereever available
  - C++ 20 stl
  - gsl
  - tcb-span
- Use `span<T>` instead of pointer/count
- Review `VectorHash` to avoid C style casting.
- Refactor
  - ShapePainter protected members to private
  - `tes::view::handler::MeshResource::readMessage` - complexity, pointer use
  - `tes::ObjectAttribute` - use `std::array` for position, etc
  - `tes::view::handler::MeshShape` protected members
- Use `cxxopts` in `3esrec`

## Protocol breaking changes

## Viewer

- Comment pass
- Clang-tidy pass
- Change shape maps using `Id` object as a key to just `uint32_t`. The category is not part of the key.
- Server API examples
  - Fixed ScopedShape to allow call chaining on the shape before `create()`` is called.
    - E.g., calling `ScopedShape->setColour()` takes effect after `create()` has been called.
  - Convert the examples to fully use the `ServerApi` objects as an API test.
- `DataBuffer` conversion examples for vertices

Advanced:

- Consider interpolating transforms over render frames for smoother animation.
  - This would lag the viewer so it may be better to leave as is.
- Don't render while catching up to a target frame.

## Bugs

- Default draw size for points is zero.
- Point draw size doesn't seem to dynamically update from settings
- OFReplace flag with mesh shape crashes?
