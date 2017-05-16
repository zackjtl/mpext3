LIB=
CC=g++
CPFLAGS=-std=c++11
BIN_NAME=mpext3

#-stdlib=libstdc++
#-O2 -Wunknown-pragmas
#-stdlib=libc++ #-Wall -Wunknown-pragmas

root_dir  = ./
INCDIR    = ./ 
SRCDIR    = ./

SRC_DIR   = $(addprefix ./, $@)
SRC_FILE  = $(wildcard $(SRC_DIR)/*.cpp)
OBJ       = $(notdir $(patsubst %.cpp, %.o,$(1)))
OBJDIR		= $(root_dir)/objects
OBJFILES	= $(wildcard $(OBJDIR)/*.o)
INCLUDES  = $(addprefix -I, $(shell pwd) ) $(addprefix -I$(shell pwd)/, $(INCDIR) )
CntFile   = 0
define ADD_RULE_TEMPLATE
	$(eval ofilepath = $(wildcard $(OBJDIR)/$(notdir $(patsubst %.cpp, %.o, $(1) ) ) ) )
	$(eval ofile = $(wildcard $(notdir $(patsubst %.cpp, %.o, $(1) ) ) ) )
  $(if $(ofilepath), @echo "1 file has compiled ==> $(ofilepath)", @echo "  New compile => $(notdir $(1))"; $(CC) -g $(CPFLAGS) $(INCLUDES) -c -fpic $(1); mv $(OBJ) $(OBJDIR);)
endef

.PHONY: all $(SRCDIR) $(OBJDIR)

all: $(SRCDIR) $(OBJDIR)

$(SRCDIR):
ifeq (,$(findstring $(OBJDIR), $(wildcard $(OBJDIR) )))
	mkdir -p $(OBJDIR)
endif
	@echo  "---------------------------- Build Start : Folder $@ ----------------------------"
	@$(foreach file,  $(SRC_FILE), $(call ADD_RULE_TEMPLATE, $(file)))


$(OBJDIR):
	@echo "Linking.. "
	@$(CC) $(CPFLAGS) -o $(BIN_NAME) $(OBJFILES)
	chmod +x $(BIN_NAME)
	@echo "Done"

.PHONY: remake $(1)
remake: $(1)

#$(foo):
	$(foreach file, $(1), rm -rf objects/$(file).o;)
	sudo make
	
.PHONY: clean
clean:
ifeq "$(wildcard $(OBJDIR))" ""
	@echo "objects directory not exists"
else
	@echo "remove whole objects directory.."
	rm -rf $(OBJDIR)/*.o
	rmdir $(OBJDIR)	
endif
	@echo "remove execution binary file.."
	rm $(BIN_NAME)

#$(if $(OBJDIR), rmdir $(OBJDIR), @echo "objects directory not exists")
#rm -rf $(OBJDIR)/*.o
#rmdir $(OBJDIR)	
