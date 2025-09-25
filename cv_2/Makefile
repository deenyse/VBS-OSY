BUILD_TARGET = build
LIBS = -L./$(BUILD_TARGET) -lgenstr

all: genstr 

genstr: static_libs encrypt
	gcc genstr.c $(LIBS) -o genstr

encrypt: dynamic_libs
	gcc encrypt.c -ldl -o encrypt

static_libs: folders
	gcc -c StaticLibs/libgenstr.c -o ./$(BUILD_TARGET)/libgenstr.o 
	ar rcs ./$(BUILD_TARGET)/libgenstr.a ./$(BUILD_TARGET)/libgenstr.o 

dynamic_libs: folders
	gcc DynamicLibs/xor_lib.c -fPIC -shared -o ./$(BUILD_TARGET)/xor_lib/libencrypt.so
	gcc DynamicLibs/caesar_lib.c -fPIC -shared -o ./$(BUILD_TARGET)/caesar_lib/libencrypt.so
	gcc DynamicLibs/rot_lib.c -fPIC -shared -o ./$(BUILD_TARGET)/rot_lib/libencrypt.so

folders:
	mkdir -p $(BUILD_TARGET) $(BUILD_TARGET)/caesar_lib $(BUILD_TARGET)/rot_lib $(BUILD_TARGET)/xor_lib  

clean:
	rm -rf $(BUILD_TARGET) genstr strcheck

setlib:
	export LD_LIBRARY_PATH=build/xor_lib