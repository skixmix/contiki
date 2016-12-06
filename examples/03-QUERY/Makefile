all: coap-server

SMALL=1

CONTIKI=../..
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"


APPS += er-coap
APPS += rest-engine


#ADDED
CONTIKI_WITH_SDN = 1
#ADDED
CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include
