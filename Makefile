PROGRAM = bta_control
LDFLAGS = -lcrypt -lm -lsla
SRCS = bta_shdata.c bta_control.c cmdlnopts.c parceargs.c usefull_macros.c ch4run.c
SRCS += angle_functions.c bta_print.c
CC = gcc
DEFINES = -D_XOPEN_SOURCE=666 -DEBUG
#-DEMULATION
CXX = gcc
CFLAGS = -Wall -Werror -Wextra $(DEFINES) -pthread
OBJS = $(SRCS:.c=.o)
all : $(PROGRAM)
$(PROGRAM) : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(PROGRAM)

# some addition dependencies
# %.o: %.c
#        $(CC) $(LDFLAGS) $(CFLAGS) $< -o $@
#$(SRCS) : %.c : %.h $(INDEPENDENT_HEADERS)
#        @touch $@

clean:
	/bin/rm -f *.o *~
depend:
	$(CXX) -MM $(CXX.SRCS)
