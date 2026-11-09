# algorithms.strings

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/algorithms/strings.codon)

function cttz[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-cttz "Copy/link to this API element")

@pure @llvm

```python
cttz(n: UInt[N], N: Literal[int])
```

```llvm
declare i{=N} @llvm.cttz.i{=N}(i{=N}, i1)
%0 = call i{=N} @llvm.cttz.i{=N}(i{=N} %n, i1 true)
ret i{=N} %0
```

function ctlz[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-ctlz "Copy/link to this API element")

@pure @llvm

```python
ctlz(n: UInt[N], N: Literal[int])
```

```llvm
declare i{=N} @llvm.ctlz.i{=N}(i{=N}, i1)
%0 = call i{=N} @llvm.ctlz.i{=N}(i{=N} %n, i1 true)
ret i{=N} %0
```

function ctpop[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-ctpop "Copy/link to this API element")

@pure @llvm

```python
ctpop(n: UInt[N], N: Literal[int])
```

```llvm
declare i{=N} @llvm.ctpop.i{=N}(i{=N})
%0 = call i{=N} @llvm.ctpop.i{=N}(i{=N} %n)
ret i{=N} %0
```

function forward\_mask[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_mask "Copy/link to this API element")

@pure @llvm

```python
forward_mask(s: Ptr[u8], k: int, i: int, firstb: u8, lastb: u8)
```

```llvm
%first0 = insertelement <16 x i8> undef, i8 %firstb, i64 0
%first = shufflevector <16 x i8> %first0, <16 x i8> poison, <16 x i32> zeroinitializer
%last0 = insertelement <16 x i8> undef, i8 %lastb, i64 0
%last = shufflevector <16 x i8> %last0, <16 x i8> poison, <16 x i32> zeroinitializer
%offset0 = add i64 %i, %k
%offset = sub i64 %offset0, 1
%ptr_first = getelementptr inbounds i8, ptr %s, i64 %i
%ptr_last = getelementptr inbounds i8, ptr %s, i64 %offset
%block_first = load <16 x i8>, ptr %ptr_first, align 1
%block_last = load <16 x i8>, ptr %ptr_last, align 1
%eq_first = icmp eq <16 x i8> %first, %block_first
%eq_last = icmp eq <16 x i8> %last, %block_last
%mask0 = and <16 x i1> %eq_first, %eq_last
%mask = bitcast <16 x i1> %mask0 to i16
ret i16 %mask
```

function backward\_mask[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-backward_mask "Copy/link to this API element")

@pure @llvm

```python
backward_mask(s: Ptr[u8], k: int, i: int, firstb: u8, lastb: u8)
```

```llvm
%j0 = sub i64 %i, 16
%j = add i64 %j0, 1
%first0 = insertelement <16 x i8> undef, i8 %firstb, i64 0
%first = shufflevector <16 x i8> %first0, <16 x i8> poison, <16 x i32> zeroinitializer
%last0 = insertelement <16 x i8> undef, i8 %lastb, i64 0
%last = shufflevector <16 x i8> %last0, <16 x i8> poison, <16 x i32> zeroinitializer
%offset0 = sub i64 %j, %k
%offset = add i64 %offset0, 1
%ptr_first = getelementptr inbounds i8, ptr %s, i64 %offset
%ptr_last = getelementptr inbounds i8, ptr %s, i64 %j
%block_first = load <16 x i8>, ptr %ptr_first, align 1
%block_last = load <16 x i8>, ptr %ptr_last, align 1
%eq_first = icmp eq <16 x i8> %last, %block_last
%eq_last = icmp eq <16 x i8> %first, %block_first
%mask0 = and <16 x i1> %eq_first, %eq_last
%mask = bitcast <16 x i1> %mask0 to i16
ret i16 %mask
```

function forward\_mask\_u16[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_mask_u16 "Copy/link to this API element")

@pure @llvm

```python
forward_mask_u16(s: Ptr[u16], k: int, i: int, first: u16, last: u16)
```

```llvm
%first0 = insertelement <8 x i16> undef, i16 %first, i64 0
%firstv = shufflevector <8 x i16> %first0, <8 x i16> poison, <8 x i32> zeroinitializer
%last0 = insertelement <8 x i16> undef, i16 %last, i64 0
%lastv = shufflevector <8 x i16> %last0, <8 x i16> poison, <8 x i32> zeroinitializer
%offset0 = add i64 %i, %k
%offset = sub i64 %offset0, 1
%ptr_first = getelementptr inbounds i16, ptr %s, i64 %i
%ptr_last = getelementptr inbounds i16, ptr %s, i64 %offset
%block_first = load <8 x i16>, ptr %ptr_first, align 1
%block_last = load <8 x i16>, ptr %ptr_last, align 1
%eq_first = icmp eq <8 x i16> %firstv, %block_first
%eq_last = icmp eq <8 x i16> %lastv, %block_last
%mask0 = and <8 x i1> %eq_first, %eq_last
%mask = bitcast <8 x i1> %mask0 to i8
ret i8 %mask
```

function forward\_mask\_u32[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_mask_u32 "Copy/link to this API element")

@pure @llvm

```python
forward_mask_u32(s: Ptr[u32], k: int, i: int, first: u32, last: u32)
```

```llvm
%first0 = insertelement <4 x i32> undef, i32 %first, i64 0
%firstv = shufflevector <4 x i32> %first0, <4 x i32> poison, <4 x i32> zeroinitializer
%last0 = insertelement <4 x i32> undef, i32 %last, i64 0
%lastv = shufflevector <4 x i32> %last0, <4 x i32> poison, <4 x i32> zeroinitializer
%offset0 = add i64 %i, %k
%offset = sub i64 %offset0, 1
%ptr_first = getelementptr inbounds i32, ptr %s, i64 %i
%ptr_last = getelementptr inbounds i32, ptr %s, i64 %offset
%block_first = load <4 x i32>, ptr %ptr_first, align 1
%block_last = load <4 x i32>, ptr %ptr_last, align 1
%eq_first = icmp eq <4 x i32> %firstv, %block_first
%eq_last = icmp eq <4 x i32> %lastv, %block_last
%mask0 = and <4 x i1> %eq_first, %eq_last
%mask = bitcast <4 x i1> %mask0 to i4
%result = zext i4 %mask to i8
ret i8 %result
```

function backward\_mask\_u16[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-backward_mask_u16 "Copy/link to this API element")

@pure @llvm

```python
backward_mask_u16(s: Ptr[u16], k: int, i: int, first: u16, last: u16)
```

```llvm
%j0 = sub i64 %i, 8
%j = add i64 %j0, 1
%first0 = insertelement <8 x i16> undef, i16 %first, i64 0
%firstv = shufflevector <8 x i16> %first0, <8 x i16> poison, <8 x i32> zeroinitializer
%last0 = insertelement <8 x i16> undef, i16 %last, i64 0
%lastv = shufflevector <8 x i16> %last0, <8 x i16> poison, <8 x i32> zeroinitializer
%offset0 = sub i64 %j, %k
%offset = add i64 %offset0, 1
%ptr_first = getelementptr inbounds i16, ptr %s, i64 %offset
%ptr_last = getelementptr inbounds i16, ptr %s, i64 %j
%block_first = load <8 x i16>, ptr %ptr_first, align 1
%block_last = load <8 x i16>, ptr %ptr_last, align 1
%eq_first = icmp eq <8 x i16> %lastv, %block_last
%eq_last = icmp eq <8 x i16> %firstv, %block_first
%mask0 = and <8 x i1> %eq_first, %eq_last
%mask = bitcast <8 x i1> %mask0 to i8
ret i8 %mask
```

function backward\_mask\_u32[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-backward_mask_u32 "Copy/link to this API element")

@pure @llvm

```python
backward_mask_u32(s: Ptr[u32], k: int, i: int, first: u32, last: u32)
```

```llvm
%j0 = sub i64 %i, 4
%j = add i64 %j0, 1
%first0 = insertelement <4 x i32> undef, i32 %first, i64 0
%firstv = shufflevector <4 x i32> %first0, <4 x i32> poison, <4 x i32> zeroinitializer
%last0 = insertelement <4 x i32> undef, i32 %last, i64 0
%lastv = shufflevector <4 x i32> %last0, <4 x i32> poison, <4 x i32> zeroinitializer
%offset0 = sub i64 %j, %k
%offset = add i64 %offset0, 1
%ptr_first = getelementptr inbounds i32, ptr %s, i64 %offset
%ptr_last = getelementptr inbounds i32, ptr %s, i64 %j
%block_first = load <4 x i32>, ptr %ptr_first, align 1
%block_last = load <4 x i32>, ptr %ptr_last, align 1
%eq_first = icmp eq <4 x i32> %lastv, %block_last
%eq_last = icmp eq <4 x i32> %firstv, %block_first
%mask0 = and <4 x i1> %eq_first, %eq_last
%mask = bitcast <4 x i1> %mask0 to i4
%result = zext i4 %mask to i8
ret i8 %result
```

function forward\_find\_u8\_u16[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_find_u8_u16 "Copy/link to this API element")

```python
forward_find_u8_u16(s: Ptr[u8], n: int, needle: Ptr[u16], k: int)
```

function forward\_find\_u8\_u32[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_find_u8_u32 "Copy/link to this API element")

```python
forward_find_u8_u32(s: Ptr[u8], n: int, needle: Ptr[u32], k: int)
```

function forward\_find\_u16[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_find_u16 "Copy/link to this API element")

```python
forward_find_u16(s: Ptr[u16], n: int, needle: Ptr[u16], k: int)
```

function forward\_find\_u16\_u8[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_find_u16_u8 "Copy/link to this API element")

```python
forward_find_u16_u8(s: Ptr[u16], n: int, needle: Ptr[u8], k: int)
```

function forward\_find\_u16\_u32[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_find_u16_u32 "Copy/link to this API element")

```python
forward_find_u16_u32(s: Ptr[u16], n: int, needle: Ptr[u32], k: int)
```

function forward\_find\_u32[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_find_u32 "Copy/link to this API element")

```python
forward_find_u32(s: Ptr[u32], n: int, needle: Ptr[u32], k: int)
```

function forward\_find\_u32\_u8[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_find_u32_u8 "Copy/link to this API element")

```python
forward_find_u32_u8(s: Ptr[u32], n: int, needle: Ptr[u8], k: int)
```

function forward\_find\_u32\_u16[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_find_u32_u16 "Copy/link to this API element")

```python
forward_find_u32_u16(s: Ptr[u32], n: int, needle: Ptr[u16], k: int)
```

function forward\_count[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_count "Copy/link to this API element")

```python
forward_count(s: Ptr[u8], n: int, needle: Ptr[u8], k: int)
```

function forward\_count\_u8\_u16[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_count_u8_u16 "Copy/link to this API element")

```python
forward_count_u8_u16(s: Ptr[u8], n: int, needle: Ptr[u16], k: int)
```

function forward\_count\_u8\_u32[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_count_u8_u32 "Copy/link to this API element")

```python
forward_count_u8_u32(s: Ptr[u8], n: int, needle: Ptr[u32], k: int)
```

function forward\_count\_u16[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_count_u16 "Copy/link to this API element")

```python
forward_count_u16(s: Ptr[u16], n: int, needle: Ptr[u16], k: int)
```

function forward\_count\_u16\_u8[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_count_u16_u8 "Copy/link to this API element")

```python
forward_count_u16_u8(s: Ptr[u16], n: int, needle: Ptr[u8], k: int)
```

function forward\_count\_u16\_u32[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_count_u16_u32 "Copy/link to this API element")

```python
forward_count_u16_u32(s: Ptr[u16], n: int, needle: Ptr[u32], k: int)
```

function forward\_count\_u32[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_count_u32 "Copy/link to this API element")

```python
forward_count_u32(s: Ptr[u32], n: int, needle: Ptr[u32], k: int)
```

function forward\_count\_u32\_u8[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_count_u32_u8 "Copy/link to this API element")

```python
forward_count_u32_u8(s: Ptr[u32], n: int, needle: Ptr[u8], k: int)
```

function forward\_count\_u32\_u16[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_count_u32_u16 "Copy/link to this API element")

```python
forward_count_u32_u16(s: Ptr[u32], n: int, needle: Ptr[u16], k: int)
```

function backward\_find\_u8\_u16[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-backward_find_u8_u16 "Copy/link to this API element")

```python
backward_find_u8_u16(s: Ptr[u8], n: int, needle: Ptr[u16], k: int)
```

function backward\_find\_u8\_u32[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-backward_find_u8_u32 "Copy/link to this API element")

```python
backward_find_u8_u32(s: Ptr[u8], n: int, needle: Ptr[u32], k: int)
```

function backward\_find\_u16[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-backward_find_u16 "Copy/link to this API element")

```python
backward_find_u16(s: Ptr[u16], n: int, needle: Ptr[u16], k: int)
```

function backward\_find\_u16\_u8[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-backward_find_u16_u8 "Copy/link to this API element")

```python
backward_find_u16_u8(s: Ptr[u16], n: int, needle: Ptr[u8], k: int)
```

function backward\_find\_u16\_u32[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-backward_find_u16_u32 "Copy/link to this API element")

```python
backward_find_u16_u32(s: Ptr[u16], n: int, needle: Ptr[u32], k: int)
```

function backward\_find\_u32[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-backward_find_u32 "Copy/link to this API element")

```python
backward_find_u32(s: Ptr[u32], n: int, needle: Ptr[u32], k: int)
```

function backward\_find\_u32\_u8[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-backward_find_u32_u8 "Copy/link to this API element")

```python
backward_find_u32_u8(s: Ptr[u32], n: int, needle: Ptr[u8], k: int)
```

function backward\_find\_u32\_u16[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-backward_find_u32_u16 "Copy/link to this API element")

```python
backward_find_u32_u16(s: Ptr[u32], n: int, needle: Ptr[u16], k: int)
```

function forward\_find[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-forward_find "Copy/link to this API element")

```python
forward_find(s: Ptr[u8], n: int, needle: Ptr[u8], k: int)
```

function backward\_find[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-backward_find "Copy/link to this API element")

```python
backward_find(s: Ptr[u8], n: int, needle: Ptr[u8], k: int)
```

function find[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-find "Copy/link to this API element")

```python
find(haystack: bytes, needle: bytes)
```

function rfind[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-rfind "Copy/link to this API element")

```python
rfind(haystack: bytes, needle: bytes)
```

function count[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-count "Copy/link to this API element")

```python
count(haystack: bytes, needle: bytes)
```

function count\_with\_max[](https://docs.exaloop.io/libraries/api/modules/algorithms/strings/#function-count_with_max "Copy/link to this API element")

```python
count_with_max(haystack: bytes, needle: bytes, maxcount: int)
```

Source: https://docs.exaloop.io/libraries/api/modules/algorithms/strings/
