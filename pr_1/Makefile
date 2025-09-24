LIBS = -L./build -lgenstr

all: genstr strcheck

genstr: static_libs
	gcc genstr.c $(LIBS) -o genstr

strcheck: dynamic_libs
	gcc strcheck.c -ldl -o strcheck

static_libs: folders
	gcc -c StaticLibs/libgenstr.c -o ./build/libgenstr.o 
	ar rcs ./build/libgenstr.a ./build/libgenstr.o 

dynamic_libs: folders
	gcc DynamicLibs/small_lib.c -fPIC -shared -o ./build/lib_small/libcounter.so
	gcc DynamicLibs/big_lib.c -fPIC -shared -o ./build/lib_big/libcounter.so

folders:
	mkdir -p build build/lib_small build/lib_big 

clean:
	rm -rf build genstr strcheck