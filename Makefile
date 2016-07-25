#for PC test, if android please write android.mk
all: touchwho

touchwho: touchwho.c
	gcc -Wall --static -o touchwho $<

clean: touchwho
	rm -rf $<
