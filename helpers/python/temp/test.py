from time import sleep

sleep(2)

print "THIS IS STDOUT23"
x = [1, 2, 3]
print "Second line of text"
y = [4, 5, 6]
z = y
y = x
x = z
print "Second line of text"
print "Second line of text"
print "Second line of text"
print "Second line of text"
print "Second line of text"
print "Second line of text"
print "Second line of text"




x = [1, 2, 3] # a different [1, 2, 3] list! test etst
y = x
x.append(4)
y.append(5)
z = [1, 2, 3, 4, 5] # a different list!
x.append(6)
y.append(7)
y = "hello"


def foo(lst):
    lst.append("hello")
    bar(lst)

def bar(myLst):
    print(myLst)

foo(x)
foo(z)