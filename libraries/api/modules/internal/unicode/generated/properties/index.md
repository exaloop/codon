# internal.unicode.generated.properties

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/unicode/generated/properties.codon)

variable UNICODE\_MAX[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#variable-unicode_max "Copy/link to this API element")

```python
UNICODE_MAX: Literal[int] = 0x10FFFF
```

variable UNICODE\_PAGE\_SHIFT[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#variable-unicode_page_shift "Copy/link to this API element")

```python
UNICODE_PAGE_SHIFT: Literal[int] = 8
```

variable UNICODE\_PAGE\_WORDS[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#variable-unicode_page_words "Copy/link to this API element")

```python
UNICODE_PAGE_WORDS: Literal[int] = 4
```

function unicode\_is\_alpha[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_is_alpha "Copy/link to this API element")

```python
unicode_is_alpha(cp: int)
```

function unicode\_is\_decimal[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_is_decimal "Copy/link to this API element")

```python
unicode_is_decimal(cp: int)
```

function unicode\_is\_digit[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_is_digit "Copy/link to this API element")

```python
unicode_is_digit(cp: int)
```

function unicode\_is\_numeric[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_is_numeric "Copy/link to this API element")

```python
unicode_is_numeric(cp: int)
```

function unicode\_is\_space[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_is_space "Copy/link to this API element")

```python
unicode_is_space(cp: int)
```

function unicode\_is\_printable[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_is_printable "Copy/link to this API element")

```python
unicode_is_printable(cp: int)
```

function unicode\_is\_lower[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_is_lower "Copy/link to this API element")

```python
unicode_is_lower(cp: int)
```

function unicode\_is\_upper[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_is_upper "Copy/link to this API element")

```python
unicode_is_upper(cp: int)
```

function unicode\_is\_title[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_is_title "Copy/link to this API element")

```python
unicode_is_title(cp: int)
```

function unicode\_is\_cased[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_is_cased "Copy/link to this API element")

```python
unicode_is_cased(cp: int)
```

function unicode\_is\_xid\_start[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_is_xid_start "Copy/link to this API element")

```python
unicode_is_xid_start(cp: int)
```

function unicode\_is\_xid\_continue[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_is_xid_continue "Copy/link to this API element")

```python
unicode_is_xid_continue(cp: int)
```

function unicode\_is\_case\_ignorable[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_is_case_ignorable "Copy/link to this API element")

```python
unicode_is_case_ignorable(cp: int)
```

function unicode\_lower\_mapping\_record[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_lower_mapping_record "Copy/link to this API element")

```python
unicode_lower_mapping_record(cp: int)
```

function unicode\_lower\_mapping\_record\_length[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_lower_mapping_record_length "Copy/link to this API element")

```python
unicode_lower_mapping_record_length(record: int)
```

function unicode\_lower\_mapping\_record\_codepoint[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_lower_mapping_record_codepoint "Copy/link to this API element")

```python
unicode_lower_mapping_record_codepoint(cp: int, record: int, index: int)
```

function unicode\_lower\_mapping\_length[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_lower_mapping_length "Copy/link to this API element")

```python
unicode_lower_mapping_length(cp: int)
```

function unicode\_lower\_mapping\_codepoint[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_lower_mapping_codepoint "Copy/link to this API element")

```python
unicode_lower_mapping_codepoint(cp: int, index: int)
```

function unicode\_upper\_mapping\_record[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_upper_mapping_record "Copy/link to this API element")

```python
unicode_upper_mapping_record(cp: int)
```

function unicode\_upper\_mapping\_record\_length[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_upper_mapping_record_length "Copy/link to this API element")

```python
unicode_upper_mapping_record_length(record: int)
```

function unicode\_upper\_mapping\_record\_codepoint[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_upper_mapping_record_codepoint "Copy/link to this API element")

```python
unicode_upper_mapping_record_codepoint(cp: int, record: int, index: int)
```

function unicode\_upper\_mapping\_length[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_upper_mapping_length "Copy/link to this API element")

```python
unicode_upper_mapping_length(cp: int)
```

function unicode\_upper\_mapping\_codepoint[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_upper_mapping_codepoint "Copy/link to this API element")

```python
unicode_upper_mapping_codepoint(cp: int, index: int)
```

function unicode\_title\_mapping\_record[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_title_mapping_record "Copy/link to this API element")

```python
unicode_title_mapping_record(cp: int)
```

function unicode\_title\_mapping\_record\_length[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_title_mapping_record_length "Copy/link to this API element")

```python
unicode_title_mapping_record_length(record: int)
```

function unicode\_title\_mapping\_record\_codepoint[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_title_mapping_record_codepoint "Copy/link to this API element")

```python
unicode_title_mapping_record_codepoint(cp: int, record: int, index: int)
```

function unicode\_title\_mapping\_length[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_title_mapping_length "Copy/link to this API element")

```python
unicode_title_mapping_length(cp: int)
```

function unicode\_title\_mapping\_codepoint[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_title_mapping_codepoint "Copy/link to this API element")

```python
unicode_title_mapping_codepoint(cp: int, index: int)
```

function unicode\_casefold\_mapping\_record[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_casefold_mapping_record "Copy/link to this API element")

```python
unicode_casefold_mapping_record(cp: int)
```

function unicode\_casefold\_mapping\_record\_length[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_casefold_mapping_record_length "Copy/link to this API element")

```python
unicode_casefold_mapping_record_length(record: int)
```

function unicode\_casefold\_mapping\_record\_codepoint[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_casefold_mapping_record_codepoint "Copy/link to this API element")

```python
unicode_casefold_mapping_record_codepoint(cp: int, record: int, index: int)
```

function unicode\_casefold\_mapping\_length[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_casefold_mapping_length "Copy/link to this API element")

```python
unicode_casefold_mapping_length(cp: int)
```

function unicode\_casefold\_mapping\_codepoint[](https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/#function-unicode_casefold_mapping_codepoint "Copy/link to this API element")

```python
unicode_casefold_mapping_codepoint(cp: int, index: int)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/unicode/generated/properties/
