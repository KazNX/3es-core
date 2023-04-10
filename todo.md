# TODO

## Protocol breaking changes

## Client

## Viewer

- Comment pass
- Clang-tidy pass
- Change shape maps using `Id` object as a key to just `uint32_t`. The category is not part of the key.

Advanced:

- Consider interpolating transforms over render frames for smoother animation.
  - This would lag the viewer so it may be better to leave as is.
- Don't render while catching up to a target frame.
