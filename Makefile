obj-m += test2_roulette_driver.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
        $(MAKE) -C $(KDIR) M=$(PWD) modules
        gcc -o test2_user_app test2_user_app.c -lpthread

clean:
        $(MAKE) -C $(KDIR) M=$(PWD) clean