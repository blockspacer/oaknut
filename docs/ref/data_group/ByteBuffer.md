---
layout: default
---

# ByteBuffer

```
class ByteBuffer
    : public Object, 
public ISerializable
```


Simple wrapper for a refcounted contiguous array of bytes, analogous to NSData on iOS or byte[] in Java.     

    
` ByteBuffer()`<br>

` ByteBuffer(size_t cb)`<br>

` ByteBuffer(uint8_t* data, size_t cb, bool copy)`<br>

` ByteBuffer(const `[`ByteBuffer`](/ref/data_group/ByteBuffer)` & data)`<br>

` ByteBuffer(const `[`string`](/ref/base_group/string)` & str)`<br>

` ~ByteBuffer()`<br>

`void set(const void* rawData, size_t len)`<br>

`void attach(uint8_t* data, size_t cb)`<br>

`void detach()`<br>

`void saveToFile(const `[`string`](/ref/base_group/string)` & path)`<br>

[`string`](/ref/base_group/string)` toString(bool copy)`<br>

`bool readSelfFromStream(`[`Stream`](/ref/data_group/Stream)`* stream)`<br>

`bool writeSelfToStream(`[`Stream`](/ref/data_group/Stream)`* stream)`<br>


[`ByteBuffer`](/ref/data_group/ByteBuffer)`* createFromFile(const `[`string`](/ref/base_group/string)` & path)`<br>


