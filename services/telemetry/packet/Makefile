
CcsdsUtils.o:
	gcc -I. CcsdsUtils.c -c $%

Crc.o:
	gcc -I. Crc.c -c $%
	
Endian.o:
	gcc -I. Endian.c -c $%
	
Fillers.o:
	gcc -I. Fillers.c -c $%

FillersUser.o:
	gcc -I. FillersUser.c -c $%
	
PusUtils.o:
	gcc -I. PusUtils.c -c $%

Serializers.o:
	gcc -I. Serializers.c -c $%

SerializersUser.o:
	gcc -I. SerializersUser.c -c $%
	
OBJ=CcsdsUtils.o Endian.o PusUtils.o Crc.o Serializers.o SerializersUser.o Fillers.o FillersUser.o

compile: $(OBJ)
	gcc main.c $(OBJ) -I. -o main.exe
	
run: compile
	./main.exe