CXX=g++

#Compilation flags for C++ (-Wall -Werror)  -D_WIN32_WINNT=0x0501 is XP
CXXFLAGS= -std=c++17 -Wall -O3 -D_WIN32_WINNT=0x0501 -DNDEBUG -DUNICODE -ffunction-sections -fdata-sections

#Directories
SOURCE_DIR=sources
INC_DIR=headers
RES_DIR=resources

BASE_DIR=_Shared
CLIENT_DIR=Client
SERVER_DIR=Server

SOURCES_EXT=cpp

OUTPUT_DIR=objects

#Application Name
CLIENT=$(CLIENT_DIR)/Chess.exe
SERVER=$(SERVER_DIR)/ChessServer.exe

SOURCE_DIR_BASE=$(BASE_DIR)/$(SOURCE_DIR)
SOURCE_DIR_CLIENT=$(CLIENT_DIR)/$(SOURCE_DIR)
SOURCE_DIR_SERVER=$(SERVER_DIR)/$(SOURCE_DIR)

OUTPUT_DIR_BASE=$(BASE_DIR)/$(OUTPUT_DIR)
OUTPUT_DIR_CLIENT=$(CLIENT_DIR)/$(OUTPUT_DIR)
OUTPUT_DIR_SERVER=$(SERVER_DIR)/$(OUTPUT_DIR)

#Libs To Use
#-lgdi32 -lcomctl32 : windows GUI
#-Wl,--subsystem,windows : no console for windows GUI
#-lpthreadGCE2
LDLIBS_CLIENT=-lgdi32 -lcomctl32 -lgdiplus -lws2_32 -s -Wl,--gc-sections,--subsystem,windows
LDLIBS_SERVER=-lgdi32 -lcomctl32 -lws2_32 -llibmariadb -lmariadbclient -s -Wl,--gc-sections,--subsystem,windows

#Preprocessor options for C or C++
CPPFLAGS_BASE=-I $(BASE_DIR)/$(INC_DIR)
CPPFLAGS_CLIENT=$(CPPFLAGS_BASE) -I $(CLIENT_DIR)/$(INC_DIR) -I $(CLIENT_DIR)/$(RES_DIR)
CPPFLAGS_SERVER=$(CPPFLAGS_BASE) -I $(SERVER_DIR)/$(INC_DIR) -I $(SERVER_DIR)/$(RES_DIR)
WINDRESFLAGS_CLIENT=-O coff -I $(CLIENT_DIR)/$(RES_DIR)
WINDRESFLAGS_SERVER=-O coff -I $(SERVER_DIR)/$(RES_DIR)

##########################################################

#source files
SOURCES_BASE=$(wildcard $(BASE_DIR)/$(SOURCE_DIR)/*.$(SOURCES_EXT)) \
						 $(wildcard $(BASE_DIR)/$(SOURCE_DIR)/**/*.$(SOURCES_EXT))
SOURCES_CLIENT=$(wildcard $(CLIENT_DIR)/$(SOURCE_DIR)/*.$(SOURCES_EXT)) \
						 $(wildcard $(CLIENT_DIR)/$(SOURCE_DIR)/**/*.$(SOURCES_EXT))
SOURCES_SERVER=$(wildcard $(SERVER_DIR)/$(SOURCE_DIR)/*.$(SOURCES_EXT)) \
						 $(wildcard $(SERVER_DIR)/$(SOURCE_DIR)/**/*.$(SOURCES_EXT))

#Object Files
_OBJS_BASE=$(SOURCES_BASE:.$(SOURCES_EXT)=.o)
_OBJS_CLIENT=$(SOURCES_CLIENT:.$(SOURCES_EXT)=.o)
_OBJS_SERVER=$(SOURCES_SERVER:.$(SOURCES_EXT)=.o)
OBJS_BASE=$(subst $(SOURCE_DIR)/,$(OUTPUT_DIR)/, $(_OBJS_BASE))
OBJS_CLIENT=$(subst $(SOURCE_DIR)/,$(OUTPUT_DIR)/, $(_OBJS_CLIENT))
OBJS_SERVER=$(subst $(SOURCE_DIR)/,$(OUTPUT_DIR)/, $(_OBJS_SERVER))

#res file
RES_CLIENT=$(wildcard $(CLIENT_DIR)/$(RES_DIR)/*.rc)
RES_SERVER=$(wildcard $(SERVER_DIR)/$(RES_DIR)/*.rc)
OBJ_RES_CLIENT=$(RES_CLIENT:.rc=.res)
OBJ_RES_SERVER=$(RES_SERVER:.rc=.res)

.PHONY: all directories directories_base directories_client directories_server client server cleanres clean cleanall

all: directories client server

client: directories_client $(OBJS_CLIENT) $(OBJ_RES_CLIENT) $(CLIENT)

server: directories_server $(OBJS_SERVER) $(OBJ_RES_SERVER) $(SERVER)

#BASE
$(OUTPUT_DIR_BASE): $(SOURCE_DIR_BASE)
	robocopy $(SOURCE_DIR_BASE)/ $(OUTPUT_DIR_BASE)/ /e /xf *

$(OUTPUT_DIR_CLIENT): $(SOURCE_DIR_CLIENT)
	robocopy $(SOURCE_DIR_CLIENT)/ $(OUTPUT_DIR_CLIENT)/ /e /xf *

$(OUTPUT_DIR_SERVER): $(SOURCE_DIR_SERVER)
	robocopy $(SOURCE_DIR_SERVER)/ $(OUTPUT_DIR_SERVER)/ /e /xf *

directories: directories_base directories_client directories_server

directories_base: $(OUTPUT_DIR_BASE)

directories_client: directories_base $(OUTPUT_DIR_CLIENT)

directories_server: directories_base $(OUTPUT_DIR_SERVER)

$(OBJ_RES_CLIENT): $(RES_CLIENT)
	windres $(RES_CLIENT) $(WINDRESFLAGS_CLIENT) -o $(OBJ_RES_CLIENT)

$(OBJ_RES_SERVER): $(RES_SERVER)
	windres $(RES_SERVER) $(WINDRESFLAGS_SERVER) -o $(OBJ_RES_SERVER)

$(CLIENT): $(OBJS_BASE) $(OBJS_CLIENT)
	$(CXX) -o $@ $^ $(OBJ_RES_CLIENT) $(LDLIBS_CLIENT)

$(SERVER): $(OBJS_BASE) $(OBJS_SERVER)
	$(CXX) -o $@ $^ $(OBJ_RES_SERVER) $(LDLIBS_SERVER)

$(OBJS_BASE): $(OUTPUT_DIR_BASE)/%.o : $(SOURCE_DIR_BASE)/%.$(SOURCES_EXT)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS_BASE) -o $@ -c $<

$(OBJS_CLIENT): $(OUTPUT_DIR_CLIENT)/%.o : $(SOURCE_DIR_CLIENT)/%.$(SOURCES_EXT)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS_CLIENT) -o $@ -c $<

$(OBJS_SERVER): $(OUTPUT_DIR_SERVER)/%.o : $(SOURCE_DIR_SERVER)/%.$(SOURCES_EXT)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS_SERVER) -o $@ -c $<

cleanres:
ifneq ($(RES_CLIENT),)
	erase $(subst /,\,$(OBJ_RES_CLIENT))
endif
ifneq ($(RES_SERVER),)
	erase $(subst /,\,$(OBJ_RES_SERVER))
endif

clean: cleanres
	rmdir $(subst /,\,$(OUTPUT_DIR_BASE)) /S /Q
	rmdir $(subst /,\,$(OUTPUT_DIR_CLIENT)) /S /Q
	rmdir $(subst /,\,$(OUTPUT_DIR_SERVER)) /S /Q

#Il faut remplacer les / par des \ pour que la commande fonctionne sur windows
cleanall: clean
	erase $(subst /,\,$(CLIENT))
	erase $(subst /,\,$(SERVER))

rebuild: cleanall all
