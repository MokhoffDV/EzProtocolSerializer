# EzProtocolSerializer Reference
`EzProtocolSerializer` is a poweful C++14 serialization tool which is capable of `reading/writing` values to/from binary buffer in compliance with established protocol. Apart from reading and writing capabilities, `EzProtocolSerializer` provides a couple of powerful tools for `buffer visualization`, which can be very useful for debugging binary data.

## Key features
- Reading/writing of any arithmetic values (float, double, int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t)
- Possibility to set specific `byte order` for multi-byte which identifies how value should read to/written from buffer
- 