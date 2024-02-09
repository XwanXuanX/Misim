set(Main_SRC main.cpp)

# Form the full path to the source files...
PREPEND(Main_SRC)

# ... and pass the variable to the parent scope.
set(Main_SRC ${Main_SRC}  PARENT_SCOPE)
