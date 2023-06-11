# TODO

- Add `Span.h` which imports a span header from where ever available
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

## Protocol breaking changes

## Viewer

- Comment pass
- Clang-tidy pass
- Change shape maps using `Id` object as a key to just `uint32_t`. The category is not part of the key.

Advanced:

- Consider interpolating transforms over render frames for smoother animation.
  - This would lag the viewer so it may be better to leave as is.
- Don't render while catching up to a target frame.
