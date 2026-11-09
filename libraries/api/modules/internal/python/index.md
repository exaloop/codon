# internal.python

Standard library module [View source](https://github.com/exaloop/codon/blob/master/stdlib/internal/python.codon)

variable Py\_DecRef[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-py_decref "Copy/link to this API element")

```python
Py_DecRef = Function[([cobj], NoneType)](cobj())
```

variable Py\_IncRef[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-py_incref "Copy/link to this API element")

```python
Py_IncRef = Function[([cobj], NoneType)](cobj())
```

variable Py\_Initialize[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-py_initialize "Copy/link to this API element")

```python
Py_Initialize = Function[([], NoneType)](cobj())
```

variable PyImport\_AddModule[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyimport_addmodule "Copy/link to this API element")

```python
PyImport_AddModule = Function[([cobj], cobj)](cobj())
```

variable PyImport\_AddModuleObject[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyimport_addmoduleobject "Copy/link to this API element")

```python
PyImport_AddModuleObject = Function[([cobj], cobj)](cobj())
```

variable PyImport\_ImportModule[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyimport_importmodule "Copy/link to this API element")

```python
PyImport_ImportModule = Function[([cobj], cobj)](cobj())
```

variable PyRun\_SimpleString[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyrun_simplestring "Copy/link to this API element")

```python
PyRun_SimpleString = Function[([cobj], NoneType)](cobj())
```

variable PyEval\_GetGlobals[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyeval_getglobals "Copy/link to this API element")

```python
PyEval_GetGlobals = Function[([], cobj)](cobj())
```

variable PyEval\_GetBuiltins[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyeval_getbuiltins "Copy/link to this API element")

```python
PyEval_GetBuiltins = Function[([], cobj)](cobj())
```

variable PyOS\_setsig[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyos_setsig "Copy/link to this API element")

```python
PyOS_setsig = Function[([i32, cobj], cobj)](cobj())
```

variable PyLong\_AsLong[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pylong_aslong "Copy/link to this API element")

```python
PyLong_AsLong = Function[([cobj], int)](cobj())
```

variable PyLong\_FromLong[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pylong_fromlong "Copy/link to this API element")

```python
PyLong_FromLong = Function[([int], cobj)](cobj())
```

variable PyFloat\_AsDouble[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyfloat_asdouble "Copy/link to this API element")

```python
PyFloat_AsDouble = Function[([cobj], float)](cobj())
```

variable PyFloat\_FromDouble[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyfloat_fromdouble "Copy/link to this API element")

```python
PyFloat_FromDouble = Function[([float], cobj)](cobj())
```

variable PyBool\_FromLong[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pybool_fromlong "Copy/link to this API element")

```python
PyBool_FromLong = Function[([int], cobj)](cobj())
```

variable PyBytes\_AsString[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pybytes_asstring "Copy/link to this API element")

```python
PyBytes_AsString = Function[([cobj], cobj)](cobj())
```

variable PyBytes\_Size[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pybytes_size "Copy/link to this API element")

```python
PyBytes_Size = Function[([cobj], int)](cobj())
```

variable PyBytes\_FromStringAndSize[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pybytes_fromstringandsize "Copy/link to this API element")

```python
PyBytes_FromStringAndSize = Function[([cobj, int], cobj)](cobj())
```

variable PyList\_New[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pylist_new "Copy/link to this API element")

```python
PyList_New = Function[([int], cobj)](cobj())
```

variable PyList\_Size[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pylist_size "Copy/link to this API element")

```python
PyList_Size = Function[([cobj], int)](cobj())
```

variable PyList\_GetItem[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pylist_getitem "Copy/link to this API element")

```python
PyList_GetItem = Function[([cobj, int], cobj)](cobj())
```

variable PyList\_SetItem[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pylist_setitem "Copy/link to this API element")

```python
PyList_SetItem = Function[([cobj, int, cobj], cobj)](cobj())
```

variable PyDict\_New[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pydict_new "Copy/link to this API element")

```python
PyDict_New = Function[([], cobj)](cobj())
```

variable PyDict\_Next[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pydict_next "Copy/link to this API element")

```python
PyDict_Next = Function[([cobj, Ptr[int], Ptr[cobj], Ptr[cobj]], int)](cobj())
```

variable PyDict\_GetItem[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pydict_getitem "Copy/link to this API element")

```python
PyDict_GetItem = Function[([cobj, cobj], cobj)](cobj())
```

variable PyDict\_GetItemString[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pydict_getitemstring "Copy/link to this API element")

```python
PyDict_GetItemString = Function[([cobj, cobj], cobj)](cobj())
```

variable PyDict\_SetItem[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pydict_setitem "Copy/link to this API element")

```python
PyDict_SetItem = Function[([cobj, cobj, cobj], cobj)](cobj())
```

variable PyDict\_Size[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pydict_size "Copy/link to this API element")

```python
PyDict_Size = Function[([cobj], int)](cobj())
```

variable PySet\_Add[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyset_add "Copy/link to this API element")

```python
PySet_Add = Function[([cobj, cobj], cobj)](cobj())
```

variable PySet\_New[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyset_new "Copy/link to this API element")

```python
PySet_New = Function[([cobj], cobj)](cobj())
```

variable PyTuple\_New[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pytuple_new "Copy/link to this API element")

```python
PyTuple_New = Function[([int], cobj)](cobj())
```

variable PyTuple\_Size[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pytuple_size "Copy/link to this API element")

```python
PyTuple_Size = Function[([cobj], int)](cobj())
```

variable PyTuple\_GetItem[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pytuple_getitem "Copy/link to this API element")

```python
PyTuple_GetItem = Function[([cobj, int], cobj)](cobj())
```

variable PyTuple\_SetItem[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pytuple_setitem "Copy/link to this API element")

```python
PyTuple_SetItem = Function[([cobj, int, cobj], i32)](cobj())
```

variable PyUnicode\_AsEncodedString[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyunicode_asencodedstring "Copy/link to this API element")

```python
PyUnicode_AsEncodedString = Function[([cobj, cobj, cobj], cobj)](cobj())
```

variable PyUnicode\_AsUTF8AndSize[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyunicode_asutf8andsize "Copy/link to this API element")

```python
PyUnicode_AsUTF8AndSize = Function[([cobj, Ptr[int]], cobj)](cobj())
```

variable PyUnicode\_FromKindAndData[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyunicode_fromkindanddata "Copy/link to this API element")

```python
PyUnicode_FromKindAndData = Function[([int, cobj, int], cobj)](cobj())
```

variable PyUnicode\_FromString[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyunicode_fromstring "Copy/link to this API element")

```python
PyUnicode_FromString = Function[([cobj], cobj)](cobj())
```

variable PyComplex\_FromDoubles[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pycomplex_fromdoubles "Copy/link to this API element")

```python
PyComplex_FromDoubles = Function[([float, float], cobj)](cobj())
```

variable PyComplex\_RealAsDouble[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pycomplex_realasdouble "Copy/link to this API element")

```python
PyComplex_RealAsDouble = Function[([cobj], float)](cobj())
```

variable PyComplex\_ImagAsDouble[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pycomplex_imagasdouble "Copy/link to this API element")

```python
PyComplex_ImagAsDouble = Function[([cobj], float)](cobj())
```

variable PyIter\_Next[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyiter_next "Copy/link to this API element")

```python
PyIter_Next = Function[([cobj], cobj)](cobj())
```

variable PySlice\_New[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyslice_new "Copy/link to this API element")

```python
PySlice_New = Function[([cobj, cobj, cobj], cobj)](cobj())
```

variable PySlice\_Unpack[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyslice_unpack "Copy/link to this API element")

```python
PySlice_Unpack = Function[([cobj, Ptr[int], Ptr[int], Ptr[int]], int)](cobj())
```

variable PyCapsule\_New[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pycapsule_new "Copy/link to this API element")

```python
PyCapsule_New = Function[([cobj, cobj, cobj], cobj)](cobj())
```

variable PyCapsule\_GetPointer[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pycapsule_getpointer "Copy/link to this API element")

```python
PyCapsule_GetPointer = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_Add[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_add "Copy/link to this API element")

```python
PyNumber_Add = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_Subtract[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_subtract "Copy/link to this API element")

```python
PyNumber_Subtract = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_Multiply[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_multiply "Copy/link to this API element")

```python
PyNumber_Multiply = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_MatrixMultiply[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_matrixmultiply "Copy/link to this API element")

```python
PyNumber_MatrixMultiply = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_FloorDivide[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_floordivide "Copy/link to this API element")

```python
PyNumber_FloorDivide = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_TrueDivide[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_truedivide "Copy/link to this API element")

```python
PyNumber_TrueDivide = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_Remainder[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_remainder "Copy/link to this API element")

```python
PyNumber_Remainder = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_Divmod[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_divmod "Copy/link to this API element")

```python
PyNumber_Divmod = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_Power[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_power "Copy/link to this API element")

```python
PyNumber_Power = Function[([cobj, cobj, cobj], cobj)](cobj())
```

variable PyNumber\_Negative[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_negative "Copy/link to this API element")

```python
PyNumber_Negative = Function[([cobj], cobj)](cobj())
```

variable PyNumber\_Positive[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_positive "Copy/link to this API element")

```python
PyNumber_Positive = Function[([cobj], cobj)](cobj())
```

variable PyNumber\_Absolute[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_absolute "Copy/link to this API element")

```python
PyNumber_Absolute = Function[([cobj], cobj)](cobj())
```

variable PyNumber\_Invert[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_invert "Copy/link to this API element")

```python
PyNumber_Invert = Function[([cobj], cobj)](cobj())
```

variable PyNumber\_Lshift[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_lshift "Copy/link to this API element")

```python
PyNumber_Lshift = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_Rshift[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_rshift "Copy/link to this API element")

```python
PyNumber_Rshift = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_And[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_and "Copy/link to this API element")

```python
PyNumber_And = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_Xor[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_xor "Copy/link to this API element")

```python
PyNumber_Xor = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_Or[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_or "Copy/link to this API element")

```python
PyNumber_Or = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_InPlaceAdd[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_inplaceadd "Copy/link to this API element")

```python
PyNumber_InPlaceAdd = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_InPlaceSubtract[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_inplacesubtract "Copy/link to this API element")

```python
PyNumber_InPlaceSubtract = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_InPlaceMultiply[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_inplacemultiply "Copy/link to this API element")

```python
PyNumber_InPlaceMultiply = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_InPlaceMatrixMultiply[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_inplacematrixmultiply "Copy/link to this API element")

```python
PyNumber_InPlaceMatrixMultiply = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_InPlaceFloorDivide[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_inplacefloordivide "Copy/link to this API element")

```python
PyNumber_InPlaceFloorDivide = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_InPlaceTrueDivide[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_inplacetruedivide "Copy/link to this API element")

```python
PyNumber_InPlaceTrueDivide = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_InPlaceRemainder[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_inplaceremainder "Copy/link to this API element")

```python
PyNumber_InPlaceRemainder = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_InPlacePower[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_inplacepower "Copy/link to this API element")

```python
PyNumber_InPlacePower = Function[([cobj, cobj, cobj], cobj)](cobj())
```

variable PyNumber\_InPlaceLshift[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_inplacelshift "Copy/link to this API element")

```python
PyNumber_InPlaceLshift = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_InPlaceRshift[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_inplacershift "Copy/link to this API element")

```python
PyNumber_InPlaceRshift = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_InPlaceAnd[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_inplaceand "Copy/link to this API element")

```python
PyNumber_InPlaceAnd = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_InPlaceXor[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_inplacexor "Copy/link to this API element")

```python
PyNumber_InPlaceXor = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_InPlaceOr[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_inplaceor "Copy/link to this API element")

```python
PyNumber_InPlaceOr = Function[([cobj, cobj], cobj)](cobj())
```

variable PyNumber\_Long[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_long "Copy/link to this API element")

```python
PyNumber_Long = Function[([cobj], cobj)](cobj())
```

variable PyNumber\_Float[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_float "Copy/link to this API element")

```python
PyNumber_Float = Function[([cobj], cobj)](cobj())
```

variable PyNumber\_Index[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pynumber_index "Copy/link to this API element")

```python
PyNumber_Index = Function[([cobj], cobj)](cobj())
```

variable PyObject\_Call[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_call "Copy/link to this API element")

```python
PyObject_Call = Function[([cobj, cobj, cobj], cobj)](cobj())
```

variable PyObject\_CallNoArgs[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_callnoargs "Copy/link to this API element")

```python
PyObject_CallNoArgs = Function[([cobj], cobj)](cobj())
```

variable PyObject\_CallOneArg[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_callonearg "Copy/link to this API element")

```python
PyObject_CallOneArg = Function[([cobj, cobj], cobj)](cobj())
```

variable PyObject\_GetAttr[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_getattr "Copy/link to this API element")

```python
PyObject_GetAttr = Function[([cobj, cobj], cobj)](cobj())
```

variable PyObject\_GetAttrString[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_getattrstring "Copy/link to this API element")

```python
PyObject_GetAttrString = Function[([cobj, cobj], cobj)](cobj())
```

variable PyObject\_GetIter[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_getiter "Copy/link to this API element")

```python
PyObject_GetIter = Function[([cobj], cobj)](cobj())
```

variable PyObject\_HasAttrString[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_hasattrstring "Copy/link to this API element")

```python
PyObject_HasAttrString = Function[([cobj, cobj], int)](cobj())
```

variable PyObject\_IsTrue[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_istrue "Copy/link to this API element")

```python
PyObject_IsTrue = Function[([cobj], i32)](cobj())
```

variable PyObject\_Length[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_length "Copy/link to this API element")

```python
PyObject_Length = Function[([cobj], int)](cobj())
```

variable PyObject\_LengthHint[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_lengthhint "Copy/link to this API element")

```python
PyObject_LengthHint = Function[([cobj, int], int)](cobj())
```

variable PyObject\_SetAttrString[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_setattrstring "Copy/link to this API element")

```python
PyObject_SetAttrString = Function[([cobj, cobj, cobj], i32)](cobj())
```

variable PyObject\_Str[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_str "Copy/link to this API element")

```python
PyObject_Str = Function[([cobj], cobj)](cobj())
```

variable PyObject\_Repr[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_repr "Copy/link to this API element")

```python
PyObject_Repr = Function[([cobj], cobj)](cobj())
```

variable PyObject\_Hash[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_hash "Copy/link to this API element")

```python
PyObject_Hash = Function[([cobj], int)](cobj())
```

variable PyObject\_GetItem[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_getitem "Copy/link to this API element")

```python
PyObject_GetItem = Function[([cobj, cobj], cobj)](cobj())
```

variable PyObject\_SetItem[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_setitem "Copy/link to this API element")

```python
PyObject_SetItem = Function[([cobj, cobj, cobj], int)](cobj())
```

variable PyObject\_DelItem[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_delitem "Copy/link to this API element")

```python
PyObject_DelItem = Function[([cobj, cobj], int)](cobj())
```

variable PyObject\_RichCompare[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_richcompare "Copy/link to this API element")

```python
PyObject_RichCompare = Function[([cobj, cobj, i32], cobj)](cobj())
```

variable PyObject\_IsInstance[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyobject_isinstance "Copy/link to this API element")

```python
PyObject_IsInstance = Function[([cobj, cobj], i32)](cobj())
```

variable PyErr\_Fetch[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyerr_fetch "Copy/link to this API element")

```python
PyErr_Fetch = Function[([Ptr[cobj], Ptr[cobj], Ptr[cobj]], NoneType)](cobj())
```

variable PyErr\_NormalizeException[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyerr_normalizeexception "Copy/link to this API element")

```python
PyErr_NormalizeException = Function[([Ptr[cobj], Ptr[cobj], Ptr[cobj]], NoneType)](cobj())
```

variable PyErr\_SetString[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyerr_setstring "Copy/link to this API element")

```python
PyErr_SetString = Function[([cobj, cobj], NoneType)](cobj())
```

variable Py\_None[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-py_none "Copy/link to this API element")

```python
Py_None = cobj()
```

variable Py\_True[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-py_true "Copy/link to this API element")

```python
Py_True = cobj()
```

variable Py\_False[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-py_false "Copy/link to this API element")

```python
Py_False = cobj()
```

variable Py\_Ellipsis[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-py_ellipsis "Copy/link to this API element")

```python
Py_Ellipsis = cobj()
```

variable Py\_NotImplemented[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-py_notimplemented "Copy/link to this API element")

```python
Py_NotImplemented = cobj()
```

variable Py\_LT[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-py_lt "Copy/link to this API element")

```python
Py_LT = 0
```

variable Py\_LE[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-py_le "Copy/link to this API element")

```python
Py_LE = 1
```

variable Py\_EQ[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-py_eq "Copy/link to this API element")

```python
Py_EQ = 2
```

variable Py\_NE[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-py_ne "Copy/link to this API element")

```python
Py_NE = 3
```

variable Py\_GT[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-py_gt "Copy/link to this API element")

```python
Py_GT = 4
```

variable Py\_GE[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-py_ge "Copy/link to this API element")

```python
Py_GE = 5
```

variable PyLong\_Type[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pylong_type "Copy/link to this API element")

```python
PyLong_Type = cobj()
```

variable PyFloat\_Type[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyfloat_type "Copy/link to this API element")

```python
PyFloat_Type = cobj()
```

variable PyBool\_Type[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pybool_type "Copy/link to this API element")

```python
PyBool_Type = cobj()
```

variable PyUnicode\_Type[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyunicode_type "Copy/link to this API element")

```python
PyUnicode_Type = cobj()
```

variable PyBytes\_Type[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pybytes_type "Copy/link to this API element")

```python
PyBytes_Type = cobj()
```

variable PyComplex\_Type[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pycomplex_type "Copy/link to this API element")

```python
PyComplex_Type = cobj()
```

variable PyList\_Type[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pylist_type "Copy/link to this API element")

```python
PyList_Type = cobj()
```

variable PyDict\_Type[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pydict_type "Copy/link to this API element")

```python
PyDict_Type = cobj()
```

variable PySet\_Type[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyset_type "Copy/link to this API element")

```python
PySet_Type = cobj()
```

variable PyTuple\_Type[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pytuple_type "Copy/link to this API element")

```python
PyTuple_Type = cobj()
```

variable PySlice\_Type[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyslice_type "Copy/link to this API element")

```python
PySlice_Type = cobj()
```

variable PyCapsule\_Type[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pycapsule_type "Copy/link to this API element")

```python
PyCapsule_Type = cobj()
```

variable PyExc\_BaseException[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_baseexception "Copy/link to this API element")

```python
PyExc_BaseException = cobj()
```

variable PyExc\_Exception[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_exception "Copy/link to this API element")

```python
PyExc_Exception = cobj()
```

variable PyExc\_NameError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_nameerror "Copy/link to this API element")

```python
PyExc_NameError = cobj()
```

variable PyExc\_OSError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_oserror "Copy/link to this API element")

```python
PyExc_OSError = cobj()
```

variable PyExc\_IOError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_ioerror "Copy/link to this API element")

```python
PyExc_IOError = cobj()
```

variable PyExc\_ValueError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_valueerror "Copy/link to this API element")

```python
PyExc_ValueError = cobj()
```

variable PyExc\_LookupError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_lookuperror "Copy/link to this API element")

```python
PyExc_LookupError = cobj()
```

variable PyExc\_IndexError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_indexerror "Copy/link to this API element")

```python
PyExc_IndexError = cobj()
```

variable PyExc\_KeyError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_keyerror "Copy/link to this API element")

```python
PyExc_KeyError = cobj()
```

variable PyExc\_TypeError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_typeerror "Copy/link to this API element")

```python
PyExc_TypeError = cobj()
```

variable PyExc\_ArithmeticError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_arithmeticerror "Copy/link to this API element")

```python
PyExc_ArithmeticError = cobj()
```

variable PyExc\_ZeroDivisionError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_zerodivisionerror "Copy/link to this API element")

```python
PyExc_ZeroDivisionError = cobj()
```

variable PyExc\_OverflowError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_overflowerror "Copy/link to this API element")

```python
PyExc_OverflowError = cobj()
```

variable PyExc\_AttributeError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_attributeerror "Copy/link to this API element")

```python
PyExc_AttributeError = cobj()
```

variable PyExc\_RuntimeError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_runtimeerror "Copy/link to this API element")

```python
PyExc_RuntimeError = cobj()
```

variable PyExc\_NotImplementedError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_notimplementederror "Copy/link to this API element")

```python
PyExc_NotImplementedError = cobj()
```

variable PyExc\_StopIteration[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_stopiteration "Copy/link to this API element")

```python
PyExc_StopIteration = cobj()
```

variable PyExc\_AssertionError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_assertionerror "Copy/link to this API element")

```python
PyExc_AssertionError = cobj()
```

variable PyExc\_EOFError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_eoferror "Copy/link to this API element")

```python
PyExc_EOFError = cobj()
```

variable PyExc\_SystemExit[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_systemexit "Copy/link to this API element")

```python
PyExc_SystemExit = cobj()
```

variable PyExc\_GeneratorExit[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_generatorexit "Copy/link to this API element")

```python
PyExc_GeneratorExit = cobj()
```

variable PyExc\_KeyboardInterrupt[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_keyboardinterrupt "Copy/link to this API element")

```python
PyExc_KeyboardInterrupt = cobj()
```

variable PyExc\_MemoryError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_memoryerror "Copy/link to this API element")

```python
PyExc_MemoryError = cobj()
```

variable PyExc\_ReferenceError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_referenceerror "Copy/link to this API element")

```python
PyExc_ReferenceError = cobj()
```

variable PyExc\_SystemError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_systemerror "Copy/link to this API element")

```python
PyExc_SystemError = cobj()
```

variable PyExc\_StopAsyncIteration[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_stopasynciteration "Copy/link to this API element")

```python
PyExc_StopAsyncIteration = cobj()
```

variable PyExc\_FloatingPointError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_floatingpointerror "Copy/link to this API element")

```python
PyExc_FloatingPointError = cobj()
```

variable PyExc\_RecursionError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_recursionerror "Copy/link to this API element")

```python
PyExc_RecursionError = cobj()
```

variable PyExc\_UnicodeError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_unicodeerror "Copy/link to this API element")

```python
PyExc_UnicodeError = cobj()
```

variable PyExc\_UnicodeDecodeError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_unicodedecodeerror "Copy/link to this API element")

```python
PyExc_UnicodeDecodeError = cobj()
```

variable PyExc\_UnicodeEncodeError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_unicodeencodeerror "Copy/link to this API element")

```python
PyExc_UnicodeEncodeError = cobj()
```

variable PyExc\_UnicodeTranslateError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_unicodetranslateerror "Copy/link to this API element")

```python
PyExc_UnicodeTranslateError = cobj()
```

variable PyExc\_BlockingIOError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_blockingioerror "Copy/link to this API element")

```python
PyExc_BlockingIOError = cobj()
```

variable PyExc\_ChildProcessError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_childprocesserror "Copy/link to this API element")

```python
PyExc_ChildProcessError = cobj()
```

variable PyExc\_ConnectionError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_connectionerror "Copy/link to this API element")

```python
PyExc_ConnectionError = cobj()
```

variable PyExc\_BrokenPipeError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_brokenpipeerror "Copy/link to this API element")

```python
PyExc_BrokenPipeError = cobj()
```

variable PyExc\_ConnectionAbortedError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_connectionabortederror "Copy/link to this API element")

```python
PyExc_ConnectionAbortedError = cobj()
```

variable PyExc\_ConnectionRefusedError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_connectionrefusederror "Copy/link to this API element")

```python
PyExc_ConnectionRefusedError = cobj()
```

variable PyExc\_ConnectionResetError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_connectionreseterror "Copy/link to this API element")

```python
PyExc_ConnectionResetError = cobj()
```

variable PyExc\_FileExistsError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_fileexistserror "Copy/link to this API element")

```python
PyExc_FileExistsError = cobj()
```

variable PyExc\_FileNotFoundError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_filenotfounderror "Copy/link to this API element")

```python
PyExc_FileNotFoundError = cobj()
```

variable PyExc\_InterruptedError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_interruptederror "Copy/link to this API element")

```python
PyExc_InterruptedError = cobj()
```

variable PyExc\_IsADirectoryError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_isadirectoryerror "Copy/link to this API element")

```python
PyExc_IsADirectoryError = cobj()
```

variable PyExc\_NotADirectoryError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_notadirectoryerror "Copy/link to this API element")

```python
PyExc_NotADirectoryError = cobj()
```

variable PyExc\_PermissionError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_permissionerror "Copy/link to this API element")

```python
PyExc_PermissionError = cobj()
```

variable PyExc\_ProcessLookupError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_processlookuperror "Copy/link to this API element")

```python
PyExc_ProcessLookupError = cobj()
```

variable PyExc\_TimeoutError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#variable-pyexc_timeouterror "Copy/link to this API element")

```python
PyExc_TimeoutError = cobj()
```

function init\_handles\_dlopen[](https://docs.exaloop.io/libraries/api/modules/internal/python/#function-init_handles_dlopen "Copy/link to this API element")

```python
init_handles_dlopen(py_handle: cobj)
```

function init\_handles\_static[](https://docs.exaloop.io/libraries/api/modules/internal/python/#function-init_handles_static "Copy/link to this API element")

```python
init_handles_static()
```

function init\_error\_py\_types[](https://docs.exaloop.io/libraries/api/modules/internal/python/#function-init_error_py_types "Copy/link to this API element")

```python
init_error_py_types()
```

function setup\_python[](https://docs.exaloop.io/libraries/api/modules/internal/python/#function-setup_python "Copy/link to this API element")

```python
setup_python(python_loaded: bool)
```

function ensure\_initialized[](https://docs.exaloop.io/libraries/api/modules/internal/python/#function-ensure_initialized "Copy/link to this API element")

```python
ensure_initialized(python_loaded: bool = False)
```

function setup\_decorator[](https://docs.exaloop.io/libraries/api/modules/internal/python/#function-setup_decorator "Copy/link to this API element")

```python
setup_decorator()
```

class PyError[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyerror "Copy/link to this API element")

```python
class PyError
```

[View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/python/PyError/)

Fields 1

`pytype` `pyobj`

Constructors 2

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyerror-__init__ "Copy/link to this API element")

```python
__init__(self: Self, message: str, Self: type)
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyerror-__init__-overload-2 "Copy/link to this API element")

@overload

```python
__init__(self: Self, message: str, pytype: pyobj, Self: type)
```

class pyobj[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj "Copy/link to this API element")

@extend

```python
class pyobj
```

Extends [`pyobj`](https://docs.exaloop.io/libraries/api/classes/pyobj/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/pyobj/)

Constructors 2

\_\_new\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__new__ "Copy/link to this API element")

```python
__new__()
```

\_\_init\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__init__ "Copy/link to this API element")

```python
__init__(self: Self, p: Ptr[u8], steal: bool = False, Self: type)
```

Methods 5

exc\_wrap[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-exc_wrap "Copy/link to this API element")

```python
exc_wrap(retval: T, T: type)
```

to\_str[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-to_str "Copy/link to this API element")

```python
to_str(self: Self, errors: str, empty: str = "", Self: type)
```

```python
to_str(p: cobj, errors: str, empty: str = "")
```

exc\_check[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-exc_check "Copy/link to this API element")

```python
exc_check()
```

incref[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-incref "Copy/link to this API element")

```python
incref(self: Self, Self: type)
```

```python
incref(ptr: Ptr[u8])
```

decref[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-decref "Copy/link to this API element")

```python
decref(self: Self, Self: type)
```

```python
decref(ptr: Ptr[u8])
```

Special methods 68

\_\_raw\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__raw__ "Copy/link to this API element")

```python
__raw__(self: Self, Self: type)
```

\_\_del\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__del__ "Copy/link to this API element")

```python
__del__(self: Self, Self: type)
```

\_\_add\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__add__ "Copy/link to this API element")

```python
__add__(self: Self, other, Self: type)
```

\_\_radd\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__radd__ "Copy/link to this API element")

```python
__radd__(self: Self, other, Self: type)
```

\_\_sub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__sub__ "Copy/link to this API element")

```python
__sub__(self: Self, other, Self: type)
```

\_\_rsub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__rsub__ "Copy/link to this API element")

```python
__rsub__(self: Self, other, Self: type)
```

\_\_mul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__mul__ "Copy/link to this API element")

```python
__mul__(self: Self, other, Self: type)
```

\_\_rmul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__rmul__ "Copy/link to this API element")

```python
__rmul__(self: Self, other, Self: type)
```

\_\_matmul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__matmul__ "Copy/link to this API element")

```python
__matmul__(self: Self, other, Self: type)
```

\_\_rmatmul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__rmatmul__ "Copy/link to this API element")

```python
__rmatmul__(self: Self, other, Self: type)
```

\_\_floordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__floordiv__ "Copy/link to this API element")

```python
__floordiv__(self: Self, other, Self: type)
```

\_\_rfloordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__rfloordiv__ "Copy/link to this API element")

```python
__rfloordiv__(self: Self, other, Self: type)
```

\_\_truediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__truediv__ "Copy/link to this API element")

```python
__truediv__(self: Self, other, Self: type)
```

\_\_rtruediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__rtruediv__ "Copy/link to this API element")

```python
__rtruediv__(self: Self, other, Self: type)
```

\_\_mod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__mod__ "Copy/link to this API element")

```python
__mod__(self: Self, other, Self: type)
```

\_\_rmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__rmod__ "Copy/link to this API element")

```python
__rmod__(self: Self, other, Self: type)
```

\_\_divmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__divmod__ "Copy/link to this API element")

```python
__divmod__(self: Self, other, Self: type)
```

\_\_rdivmod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__rdivmod__ "Copy/link to this API element")

```python
__rdivmod__(self: Self, other, Self: type)
```

\_\_pow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__pow__ "Copy/link to this API element")

```python
__pow__(self: Self, other, Self: type)
```

\_\_rpow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__rpow__ "Copy/link to this API element")

```python
__rpow__(self: Self, other, Self: type)
```

\_\_neg\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__neg__ "Copy/link to this API element")

```python
__neg__(self: Self, Self: type)
```

\_\_pos\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__pos__ "Copy/link to this API element")

```python
__pos__(self: Self, Self: type)
```

\_\_invert\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__invert__ "Copy/link to this API element")

```python
__invert__(self: Self, Self: type)
```

\_\_lshift\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__lshift__ "Copy/link to this API element")

```python
__lshift__(self: Self, other, Self: type)
```

\_\_rlshift\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__rlshift__ "Copy/link to this API element")

```python
__rlshift__(self: Self, other, Self: type)
```

\_\_rshift\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__rshift__ "Copy/link to this API element")

```python
__rshift__(self: Self, other, Self: type)
```

\_\_rrshift\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__rrshift__ "Copy/link to this API element")

```python
__rrshift__(self: Self, other, Self: type)
```

\_\_and\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__and__ "Copy/link to this API element")

```python
__and__(self: Self, other, Self: type)
```

\_\_rand\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__rand__ "Copy/link to this API element")

```python
__rand__(self: Self, other, Self: type)
```

\_\_xor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__xor__ "Copy/link to this API element")

```python
__xor__(self: Self, other, Self: type)
```

\_\_rxor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__rxor__ "Copy/link to this API element")

```python
__rxor__(self: Self, other, Self: type)
```

\_\_or\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__or__ "Copy/link to this API element")

```python
__or__(self: Self, other, Self: type)
```

\_\_ror\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__ror__ "Copy/link to this API element")

```python
__ror__(self: Self, other, Self: type)
```

\_\_iadd\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__iadd__ "Copy/link to this API element")

```python
__iadd__(self: Self, other, Self: type)
```

\_\_isub\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__isub__ "Copy/link to this API element")

```python
__isub__(self: Self, other, Self: type)
```

\_\_imul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__imul__ "Copy/link to this API element")

```python
__imul__(self: Self, other, Self: type)
```

\_\_imatmul\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__imatmul__ "Copy/link to this API element")

```python
__imatmul__(self: Self, other, Self: type)
```

\_\_ifloordiv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__ifloordiv__ "Copy/link to this API element")

```python
__ifloordiv__(self: Self, other, Self: type)
```

\_\_itruediv\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__itruediv__ "Copy/link to this API element")

```python
__itruediv__(self: Self, other, Self: type)
```

\_\_imod\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__imod__ "Copy/link to this API element")

```python
__imod__(self: Self, other, Self: type)
```

\_\_ipow\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__ipow__ "Copy/link to this API element")

```python
__ipow__(self: Self, other, Self: type)
```

\_\_ilshift\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__ilshift__ "Copy/link to this API element")

```python
__ilshift__(self: Self, other, Self: type)
```

\_\_irshift\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__irshift__ "Copy/link to this API element")

```python
__irshift__(self: Self, other, Self: type)
```

\_\_iand\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__iand__ "Copy/link to this API element")

```python
__iand__(self: Self, other, Self: type)
```

\_\_ixor\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__ixor__ "Copy/link to this API element")

```python
__ixor__(self: Self, other, Self: type)
```

\_\_ior\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__ior__ "Copy/link to this API element")

```python
__ior__(self: Self, other, Self: type)
```

\_\_int\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__int__ "Copy/link to this API element")

```python
__int__(self: Self, Self: type)
```

\_\_float\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__float__ "Copy/link to this API element")

```python
__float__(self: Self, Self: type)
```

\_\_index\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__index__ "Copy/link to this API element")

```python
__index__(self: Self, Self: type)
```

\_\_len\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__len__ "Copy/link to this API element")

```python
__len__(self: Self, Self: type)
```

\_\_length\_hint\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__length_hint__ "Copy/link to this API element")

```python
__length_hint__(self: Self, Self: type)
```

\_\_getitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__getitem__ "Copy/link to this API element")

```python
__getitem__(self: Self, key, Self: type)
```

\_\_setitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__setitem__ "Copy/link to this API element")

```python
__setitem__(self: Self, key, v, Self: type)
```

\_\_delitem\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__delitem__ "Copy/link to this API element")

```python
__delitem__(self: Self, key, Self: type)
```

\_\_lt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__lt__ "Copy/link to this API element")

```python
__lt__(self: Self, other, Self: type)
```

\_\_le\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__le__ "Copy/link to this API element")

```python
__le__(self: Self, other, Self: type)
```

\_\_eq\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__eq__ "Copy/link to this API element")

```python
__eq__(self: Self, other, Self: type)
```

\_\_ne\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__ne__ "Copy/link to this API element")

```python
__ne__(self: Self, other, Self: type)
```

\_\_gt\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__gt__ "Copy/link to this API element")

```python
__gt__(self: Self, other, Self: type)
```

\_\_ge\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__ge__ "Copy/link to this API element")

```python
__ge__(self: Self, other, Self: type)
```

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__from_py__ "Copy/link to this API element")

```python
__from_py__(p: cobj)
```

\_\_str\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__str__ "Copy/link to this API element")

```python
__str__(self: Self, Self: type)
```

\_\_repr\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__repr__ "Copy/link to this API element")

```python
__repr__(self: Self, Self: type)
```

\_\_hash\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__hash__ "Copy/link to this API element")

```python
__hash__(self: Self, Self: type)
```

\_\_iter\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__iter__ "Copy/link to this API element")

```python
__iter__(self: Self, Self: type)
```

\_\_call\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__call__ "Copy/link to this API element")

```python
__call__(self: Self, *args, **kwargs, Self: type)
```

\_\_bool\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-pyobj-__bool__ "Copy/link to this API element")

```python
__bool__(self: Self, Self: type)
```

class NoneType[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-nonetype "Copy/link to this API element")

@extend

```python
class NoneType
```

Extends [`NoneType`](https://docs.exaloop.io/libraries/api/classes/NoneType/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/NoneType/)

Special methods 2

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-nonetype-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-nonetype-__from_py__ "Copy/link to this API element")

```python
__from_py__(x: cobj)
```

class int[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-int "Copy/link to this API element")

@extend

```python
class int
```

Special methods 2

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-int-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-int-__from_py__ "Copy/link to this API element")

```python
__from_py__(i: cobj)
```

class float[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-float "Copy/link to this API element")

@extend

```python
class float
```

Extends [`float`](https://docs.exaloop.io/libraries/api/classes/float/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/float/)

Special methods 2

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-float-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-float-__from_py__ "Copy/link to this API element")

```python
__from_py__(d: cobj)
```

class bool[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-bool "Copy/link to this API element")

@extend

```python
class bool
```

Extends [`bool`](https://docs.exaloop.io/libraries/api/classes/bool/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/bool/)

Special methods 2

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-bool-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-bool-__from_py__ "Copy/link to this API element")

```python
__from_py__(b: cobj)
```

class str[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-str "Copy/link to this API element")

@extend

```python
class str
```

Extends [`str`](https://docs.exaloop.io/libraries/api/classes/str/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/str/)

Special methods 2

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-str-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-str-__from_py__ "Copy/link to this API element")

```python
__from_py__(s: cobj)
```

class bytes[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-bytes "Copy/link to this API element")

@extend

```python
class bytes
```

Extends [`bytes`](https://docs.exaloop.io/libraries/api/classes/bytes/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/bytes/)

Special methods 2

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-bytes-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-bytes-__from_py__ "Copy/link to this API element")

```python
__from_py__(b: cobj)
```

class complex[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-complex "Copy/link to this API element")

@extend

```python
class complex
```

Extends [`internal.types.complex.complex`](https://docs.exaloop.io/libraries/api/classes/internal/types/complex/complex/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/complex/complex/)

Special methods 2

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-complex-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-complex-__from_py__ "Copy/link to this API element")

```python
__from_py__(c: cobj)
```

class List[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-list "Copy/link to this API element")

@extend

```python
class List[T]
```

Extends [`List`](https://docs.exaloop.io/libraries/api/classes/List/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/List/)

Special methods 2

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-list-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-list-__from_py__ "Copy/link to this API element")

```python
__from_py__(v: cobj)
```

class OrderedDict[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-ordereddict "Copy/link to this API element")

@extend

```python
class OrderedDict[K, V]
```

Extends [`OrderedDict`](https://docs.exaloop.io/libraries/api/classes/OrderedDict/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/OrderedDict/)

Special methods 2

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-ordereddict-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-ordereddict-__from_py__ "Copy/link to this API element")

```python
__from_py__(d: cobj)
```

class Set[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-set "Copy/link to this API element")

@extend

```python
class Set[K]
```

Extends [`Set`](https://docs.exaloop.io/libraries/api/classes/Set/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/Set/)

Special methods 2

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-set-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-set-__from_py__ "Copy/link to this API element")

```python
__from_py__(s: cobj)
```

class DynamicTuple[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-dynamictuple "Copy/link to this API element")

@extend

```python
class DynamicTuple[T]
```

Extends [`DynamicTuple`](https://docs.exaloop.io/libraries/api/classes/DynamicTuple/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/DynamicTuple/)

Special methods 2

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-dynamictuple-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-dynamictuple-__from_py__ "Copy/link to this API element")

```python
__from_py__(t: cobj)
```

class Slice[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-slice "Copy/link to this API element")

@extend

```python
class Slice[T, U, V]
```

Extends [`internal.types.slice.Slice`](https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/internal/types/slice/Slice/)

Special methods 2

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-slice-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-slice-__from_py__ "Copy/link to this API element")

```python
__from_py__(s: cobj)
```

class Optional[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-optional "Copy/link to this API element")

@extend

```python
class Optional[T]
```

Extends [`Optional`](https://docs.exaloop.io/libraries/api/classes/Optional/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/Optional/)

Special methods 2

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-optional-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-optional-__from_py__ "Copy/link to this API element")

```python
__from_py__(o: cobj)
```

class ellipsis[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-ellipsis "Copy/link to this API element")

@extend

```python
class ellipsis
```

Extends [`ellipsis`](https://docs.exaloop.io/libraries/api/classes/ellipsis/) · [View complete class API →](https://docs.exaloop.io/libraries/api/classes/ellipsis/)

Special methods 2

\_\_to\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-ellipsis-__to_py__ "Copy/link to this API element")

```python
__to_py__(self: Self, Self: type)
```

\_\_from\_py\_\_[](https://docs.exaloop.io/libraries/api/modules/internal/python/#class-ellipsis-__from_py__ "Copy/link to this API element")

```python
__from_py__(e: cobj)
```

Source: https://docs.exaloop.io/libraries/api/modules/internal/python/
