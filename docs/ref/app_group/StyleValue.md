---
layout: default
---

# StyleValue

```
class StyleValue

Represents a value in the global style namespace.     

Not instantiated directly, use `App::getStyleValue()` and friends to get one, then use one of the accessors `intVal()`, `stringVal()` to get to the raw value.Is alarmingly similar to the 'variant' type with which it should probably be unified.     
` StyleValue()`<br>
` StyleValue(const `[`StyleValue`](/oaknut/ref/app_group/StyleValue)` & )`<br>
` StyleValue(`[`StyleValue`](/oaknut/ref/app_group/StyleValue)` && )`<br>
` ~StyleValue()`<br>
[`StyleValue`](/oaknut/ref/app_group/StyleValue)` & operator=(const `[`StyleValue`](/oaknut/ref/app_group/StyleValue)` & other)`<br>
`bool isEmpty()`<br>
`bool isNumeric()`<br>
`bool isString()`<br>
`bool isMeasurement()`<br>
`bool isArray()`<br>
`int intVal()`<br>
`bool boolVal()`<br>
`float floatVal()`<br>
[`string`](/oaknut/ref/base_group/string)` stringVal()`<br>
`measurement measurementVal()`<br>
[`COLOR`](/oaknut/ref/graphics_group/COLOR)` colorVal()`<br>
`const vector< `[`StyleValue`](/oaknut/ref/app_group/StyleValue)` > & arrayVal()`<br>
`const map< `[`string`](/oaknut/ref/base_group/string)`, `[`StyleValue`](/oaknut/ref/app_group/StyleValue)` > & compoundVal()`<br>
`VECTOR4 cornerRadiiVal()`<br>
`EDGEINSETS edgeInsetsVal()`<br>
`float fontWeightVal()`<br>
`const `[`StyleValue`](/oaknut/ref/app_group/StyleValue)`* get(const `[`string`](/oaknut/ref/base_group/string)` & keypath)`<br>
`int intVal(const `[`string`](/oaknut/ref/base_group/string)` & name)`<br>
`float floatVal(const `[`string`](/oaknut/ref/base_group/string)` & name)`<br>
[`string`](/oaknut/ref/base_group/string)` stringVal(const `[`string`](/oaknut/ref/base_group/string)` & name)`<br>
`const vector< `[`StyleValue`](/oaknut/ref/app_group/StyleValue)` > & arrayVal(const `[`string`](/oaknut/ref/base_group/string)` & name)`<br>
`void importValues(const map< `[`string`](/oaknut/ref/base_group/string)`, `[`StyleValue`](/oaknut/ref/app_group/StyleValue)` > & values)`<br>
`bool parse(class StringProcessor & it, int flags)`<br>

`void setType(Type newType)`<br>
`void copyFrom(const `[`StyleValue`](/oaknut/ref/app_group/StyleValue)`* other)`<br>
`const `[`StyleValue`](/oaknut/ref/app_group/StyleValue)`* select()`<br>
`bool parseNumberOrMeasurement(StringProcessor & it)`<br>
