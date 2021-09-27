x, y = 1, 2
print x, y # EXPECT: 1 2
(x, y) = (3, 4)
print x, y # EXPECT: 3 4
x, y = (1, 2)
print x, y # EXPECT: 1 2
(x, y) = 3, 4
print x, y # EXPECT: 3 4
(x, y) = [3, 4]
print x, y # EXPECT: 3 4
[x, y] = [1, 2]
print x, y # EXPECT: 1 2

# TODO generator/range slices?
# a, b, *tx, c, d = range(10)
# print a, b, tx, c, d # 0 1 [2, 3, 4, 5, 6, 7] 8 9

l = list(iter(range(10)))
[a, b, *lx, c, d] = l
print a, b, lx, c, d # EXPECT: 0 1 [2, 3, 4, 5, 6, 7] 8 9
a, b, *lx = l
print a, b, lx # EXPECT: 0 1 [2, 3, 4, 5, 6, 7, 8, 9]
*lx, a, b = l
print lx, a, b # EXPECT: [0, 1, 2, 3, 4, 5, 6, 7] 8 9

*xz, a, b = (1, 2, 3, 4, 5)
print xz, a, b # EXPECT: (1, 2, 3) 4 5


# *x = range(5) # ERR
(*ex,) = [1, 2, 3]
print ex # EXPECT: [1, 2, 3]

# https://stackoverflow.com/questions/6967632/unpacking-extended-unpacking-and-nested-extended-unpacking

sa, sb = 'XY'
print sa, sb # EXPECT: X Y
(sa, sb), sc = 'XY', 'Z'
print sa, sb, sc # EXPECT: X Y Z
# (sa, sb), sc = 'XYZ' # ERROR:

sa, *la = 'X'
print sa, la, 1 # EXPECT: X  1

sa, *la = 'XYZ'
print sa, la # EXPECT: X YZ

# a, *b, c, *d = 1,2,3,4,5             # ERROR -- two starred expressions in assignment

(xa,xb), *xc, xd = [1,2],'this'
print xa, xb, xc, xd # EXPECT: 1 2 () this

(a, b), (sc, *sl) = [1,2], 'this'
print a, b, sc, sl # EXPECT: 1 2 t his



# // a, b, *x, c, d = y
#   // (^) = y
#   // [^] = y
#   // *a = y NO ; *a, = y YES
#   // (a, b), c = d, e
#   // *(a, *b), c = this
#   // a = *iterable


# # Issue #116
# def foo():
#     a = 42
#     def bar():
#         print a  # <-- this should be a parser error; 'a' has to be global
#     bar()
# foo() # ERROR: identifier 'a' not found
