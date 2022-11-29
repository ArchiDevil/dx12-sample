set(SRC math.cpp
        math.h
        math/matrix.h
        math/matrixFuncs.h
        math/quaternion.h
        math/vector2.h
        math/vector3.h
        math/vector4.h)

add_library(math STATIC ${SRC})
