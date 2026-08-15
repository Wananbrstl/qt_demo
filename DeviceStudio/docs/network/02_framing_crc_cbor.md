# Binary framing, CRC32, and CBOR payloads

## Envelope

All integer fields use network byte order:

```text
magic:u32 | version:u16 | type:u16 | flags:u16 | reserved:u16
sequence:u32 | payload-size:u32 | crc32:u32 | payload:N
```

The fixed 24-byte header allows the decoder to validate payload length before
allocating or waiting for a frame. `maximumFrameBytes` protects against a corrupt
or malicious length field.

## Recovery

When magic is not at offset zero, the decoder searches forward and reports the
discarded bytes. If no complete magic exists, it retains the final three bytes
because they may prefix a magic value split across TCP reads.

## CRC scope

CRC covers the first 20 header bytes and payload. CRC detects accidental damage
and aids resynchronization; it does not provide authenticity. TLS and message
authentication are separate security concerns.

## Payload

CBOR keeps the envelope stable while allowing message fields to evolve. The
decoder validates required types instead of assuming every map field exists.

## Exercises

- Add a protocol-version compatibility table.
- Add a test that prefixes garbage before a valid frame.
- Add a test for a payload length larger than the configured maximum.
- Implement a typed Command payload codec rather than constructing maps inside
  the session and simulator.

