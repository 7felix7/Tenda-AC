# $Id: Makefile.in,v 1.16 2010/01/13 17:20:39 armin Exp $

include $(ECOS_WORK_DIR)/.config

PREFIX?=$(notdir $(shell pwd))_

#------------------------------------------------------------------------------
# EXTRA_CFLAGS
#------------------------------------------------------------------------------
EXTRA_CFLAGS =
ifeq ($(CONFIG_MIPS16),y)
EXTRA_CFLAGS += -mips16
endif

#------------------------------------------------------------------------------
# CFLAGS/CXXFLAGS
#------------------------------------------------------------------------------
CFLAGS += $(EXTRA_CFLAGS) -I.
CXXFLAGS += $(EXTRA_CFLAGS) -I.

#------------------------------------------------------------------------------
.SUFFIXES: .o .c .cpp .cxx .s .S
SOURCES = dnsproxy.c hash.c internal.c log.c parse.c  
#------------------------------------------------------------------------------
DEPEND = ./.depend

OBJS = $(SOURCES:.c=.o)
PREFIX_OBJS=$(join $(dir $(OBJS)),$(addprefix $(PREFIX),$(notdir $(OBJS))))

all:depend $(OBJS) $(PREFIX_OBJS) Makefile
	$(AR) rcs $(APP_LIB) $(PREFIX_OBJS)
.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

$(PREFIX)%.o: %.o
	@cp $^ $@

clean:
	rm -f $(DEPEND)
	rm -f $(OBJS) $(PREFIX_OBJS) *.o *~

depend: $(SOURCES)
	if [ ! -e $(DEPEND) ]; then \
		$(CPP) $(CFLAGS) -MM $^ > $(DEPEND); \
	fi

-include $(DEPEND)

.PHONY: clean all depend
