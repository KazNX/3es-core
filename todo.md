# TODO

## Protocol breaking changes

## Client

## Viewer

- Change shape maps using `Id` object as a key to just `uint32_t`. The category is not part of the key.

Advanced:

- Consider interpolating transforms over render frames for smoother animation.
  - This would lag the viewer so it may be better to leave as is.
- Don't render while catching up to a target frame.

### Viewer bugs

- Mouse camera control is broken since IMGui integration
- Command shortcuts are broken since IMGui integration
- Enabling the EDL shader breaks text rendering (since IMGui integration?)
