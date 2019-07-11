# კომპილატორი და მისი საჭირო ოფციები;
# ამ ოფციების გარეშე gcc ვერ მიაგნებს fuse3-ის ფაილებს
CC=gcc
FLAGS=`pkg-config fuse3 --cflags --libs`
rebuild: clean all

# default target რომელსაც make ასრულებს;
# მისი სინტაქსი ასეთია:
# სახელი : მოდულების სახელების რაზეც დამოკიდებულია
# 		შესასრულებელი ბრძანება
all : main.o memcached_client.o free-map.o inode.o list.o directory.o filesys.o
	$(CC) -o cachefs main.o memcached_client.o free-map.o inode.o list.o directory.o filesys.o  $(FLAGS)

# რიგითი მოდულის კონფიგურაცია:
# სახელი : დამოკიდებულებების სია (აქ შეიძლება იყოს .h ჰედერ ფაილებიც)
# 	შესასრულებელი ბრძანება
main.o : main.c 
	$(CC) -c main.c $(FLAGS)
	
memcached_client.o : memcached_client.c memcached_client.h
	$(CC) -c memcached_client.c

free-map.o : free-map.c free-map.h
	$(CC) -c free-map.c

inode.o : inode.c inode.h
	$(CC) -c inode.c

list.o : list.c list.h
	$(CC) -c list.c

directory.o : directory.c directory.h
	$(CC) -c directory.c

filesys.o : filesys.c 
	$(CC) -c filesys.c $(FLAGS)

# დაგენერირებული არტიფაქტების წაშლა
clean :
	rm -f cachefs main.o memcached_client.o free-map.o inode.o list.o directory.o filesys.o


# თუ პროექტს დაამატებთ .c ფაილებს, მაშინ აქ უნდა დაამატოთ ახალი მოდული, main.o-ს მსგავსად. ასევე ახალი_ფაილი.o უნდა დაუმაროთ all-ს, და clean-ს. მაგალითად:
# all : main.o new_file.o
# 	$(CC) -o cachefs main.o new_file.o $(FLAGS)
# 	
# new_file.o : new_file.c new_file.h
# 	$(CC) -c main.c $(FLAGS)
#
# clean :
# 	rm cachefs main.o new_file.o
