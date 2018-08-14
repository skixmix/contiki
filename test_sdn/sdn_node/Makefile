all: sdn_node
CONTIKI=../../

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

#linker optimizations
SMALL=1

ifeq ($(MAKE_WITH_NON_STORING),1)
CFLAGS += -DWITH_NON_STORING=1
endif

CONTIKI_WITH_SDN = 1
MODULES += core/net/ipv6/multicast
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
