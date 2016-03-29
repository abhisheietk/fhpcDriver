###############################################################
#
# Change History:
#
# $Log: not supported by cvs2svn $
# Revision 1.4  2006/11/17 18:56:44  marcus
# Added testCompatInterface.
#
# Revision 1.3  2006/10/30 19:40:32  marcus
# Added testPCIeBoard.
#
# Revision 1.2  2006/10/16 16:57:17  marcus
# Added testCinterface.
#
# Revision 1.1  2006/10/13 17:18:35  marcus
# Implemented and tested most of C++ interface.
#
###############################################################

# Makefile for the pciDriver test programs

include ../../common.mk

INCDIR += ../../include
LDINC += $(addprefix -L ,$(LIBDIR))
LDFLAGS += -lpthread -lrt
LIBS += -lpcidriver -lpthread

BINARIES = testCppInterface testCompatInterface testDMA

###############################################################
# Target definitions

.PHONY: all dirs depend clean

all: dirs depend $(BINARIES) $(BINDIR)/testPciDriver $(BINDIR)/testCinterface $(BINDIR)/test1 $(BINDIR)/testDMA_c $(BINDIR)/pcieSMA $(BINDIR)/pcieUSR $(BINDIR)/pcieDMA $(BINDIR)/pcieDESC

# Relate all exec names to it exec in the bin dir
$(BINARIES) : % : $(BINDIR)/% ;

# Target for each exec from the object file
$(BINDIR)/%: $(OBJDIR)/%.o
	@echo -e "LD \t$@"
	$(Q)$(CXX) $(LDINC) $(LDFLAGS) $(CXXFLAGS) -o $@ $< $(LIBS)

$(BINDIR)/testCinterface: $(OBJDIR)/testCinterface.o
	@echo -e "LD \t$@"
	$(Q)$(CC) $(LDINC) $(LDFLAGS) $(CFLAGS) -o $@ $< $(LIBS)

$(BINDIR)/testDMA_c: $(OBJDIR)/testDMA_c.o
	@echo -e "LD \t$@"
	$(Q)$(CC) $(LDINC) $(LDFLAGS) $(CFLAGS) -o $@ $< $(LIBS)

$(BINDIR)/pcieUSR: $(OBJDIR)/pcieUSR.o
	@echo -e "LD \t$@"
	$(Q)$(CC) $(LDINC) $(LDFLAGS) $(CFLAGS) -o $@ $< $(LIBS)

$(BINDIR)/pcieSMA: $(OBJDIR)/pcieSMA.o
	@echo -e "LD \t$@"
	$(Q)$(CC) $(LDINC) $(LDFLAGS) $(CFLAGS) -o $@ $< $(LIBS)

$(BINDIR)/pcieDESC: $(OBJDIR)/pcieDESC.o
	@echo -e "LD \t$@"
	$(Q)$(CC) $(LDINC) $(LDFLAGS) $(CFLAGS) -o $@ $< $(LIBS)

$(BINDIR)/pcieDMA: $(OBJDIR)/pcieDMA.o
	@echo -e "LD \t$@"
	$(Q)$(CC) $(LDINC) $(LDFLAGS) $(CFLAGS) -o $@ $< $(LIBS)

$(BINDIR)/test1: $(OBJDIR)/test1.o
	@echo -e "LD \t$@"
	$(Q)$(CC) $(LDINC) $(LDFLAGS) $(CFLAGS) -o $@ $< $(LIBS)

$(BINDIR)/testPciDriver: $(OBJDIR)/testPciDriver.o
	@echo -e "LD \t$@"
	$(Q)$(CC) $(LDINC) $(LDFLAGS) $(CFLAGS) -o $@ $< $(LIBS)
	    	
clean:
	@echo -e "CLEAN \t$(shell pwd)"
	-$(Q)rm -f $(addprefix $(BINDIR)/,$(BINARIES))
	-$(Q)rm -f $(BINDIR)/testCinterface
	-$(Q)rm -f $(BINDIR)/test1
	-$(Q)rm -f $(BINDIR)/testDMA_c
	-$(Q)rm -f $(BINDIR)/pcieSMA
	-$(Q)rm -f $(BINDIR)/pcieDESC
	-$(Q)rm -f $(BINDIR)/pcieUSR
	-$(Q)rm -f $(BINDIR)/pcieDMA
	-$(Q)rm -f $(BINDIR)/testPciDriver
	-$(Q)rm -f $(OBJ)
	-$(Q)rm -f $(OBJDIR)/testCinterface.o
	-$(Q)rm -f $(OBJDIR)/test1.o
	-$(Q)rm -f $(OBJDIR)/testDMA_c.o
	-$(Q)rm -f $(OBJDIR)/pcieSMA.o
	-$(Q)rm -f $(OBJDIR)/pcieDESC.o
	-$(Q)rm -f $(OBJDIR)/pcieUSR.o
	-$(Q)rm -f $(OBJDIR)/pcieDMA.o
	-$(Q)rm -f $(OBJDIR)/testPciDriver.o
	-$(Q)rm -f $(DEPEND)
