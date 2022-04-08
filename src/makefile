FLAGS	= -Wall -g -pthread
CC	    = gcc

PROG1   = offload_simulator
OBJS1   = SystemManager.o auxiliar.o

PROG2	= mobile_node
OBJS2	= MobileNode.o auxiliar.o

# $@ nome do target (app)
# $< primeiro pre requesito (?)

all:	${PROG1} ${PROG2} clean

${PROG1}:	${OBJS1}
	${CC} ${FLAGS} ${OBJS1} -o $@ 

${PROG2}:	${OBJS2}
	${CC} ${FLAGS} ${OBJS2} -o $@ 

.PHONY : clean
clean:
	rm -f ${OBJS1} ${OBJS2}

#############################

# System Manager
SystemManager.o: SystemManager.c

auxiliar.o:	auxiliar.h auxiliar.c

offload_simulator: SystemManager.o auxiliar.o 

#Mobile Node

MobileNode.o: MobileNode.c

mobile_node: MobileNode.o auxiliar.o