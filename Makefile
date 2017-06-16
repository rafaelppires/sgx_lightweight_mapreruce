CXX            := g++-5
SHELL          := /bin/bash
SGX_SDK        := /opt/intel/sgxsdk
SGX_MODE       ?= HW
#SGX_MODE       ?= SIMULATION_MODE
SGX_PRERELEASE ?= 1

CXX                := g++
SGX_LIBRARY_PATH   := $(SGX_SDK)/lib64
SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
SGX_EDGER8R        := $(SGX_SDK)/bin/x64/sgx_edger8r

TARGETS      := client worker
BINDIR       := bin
OBJDIR       := obj
SRCDIR       := src
LUADIR       := ../enclaved_lua/lua-sgx
SCBRSRCDIR   := ../scbr/src
SGXCOMMONDIR := ../sgx_common
SCBROBJS     := $(addprefix $(OBJDIR)/, message.o communication_zmq.o)
SGXCOMOBJS   := $(addprefix $(OBJDIR)/, sgx_errlist.o sgx_initenclave.o crypto.o utils.o)

MRENCLAVE    := enclave_mapreduce
TRUSTEDOBJS  := $(addprefix $(OBJDIR)/, $(MRENCLAVE)_t.o $(MRENCLAVE).o enclaved_mapper.o enclaved_reducer.o worker_protocol.o)

App_IncDirs  := src $(SCBRSRCDIR) $(SGX_SDK)/include $(SGXCOMMONDIR)
App_Include  := $(addprefix -I, $(App_IncDirs))

Enclave_IncDirs := $(LUADIR)/src $(SGX_SDK)/include/tlibc \
                   $(SGX_SDK)/include $(SRCDIR) /usr/include/x86_64-linux-gnu/ \
                   $(SGXCOMMONDIR)
Enclave_Include := $(addprefix -I, $(Enclave_IncDirs))

SGX_COMMON_CFLAGS := -m64
App_C_Flags := $(SGX_COMMON_CFLAGS) -fPIC -Wno-attributes
ifeq ($(SGX_DEBUG), 1)
    SGX_COMMON_CFLAGS += -O0 -g
    App_C_Flags += -DDEBUG -UNDEBUG -UEDEBUG
else
    SGX_COMMON_CFLAGS += -O2
    ifeq ($(SGX_PRERELEASE), 1)
        App_C_Flags += -DNDEBUG -DEDEBUG -UDEBUG
    else
        App_C_Flags += -DNDEBUG -UEDEBUG -UDEBUG
    endif
endif

App_Cpp_Flags := $(App_C_Flags) -std=c++11
App_Link_Flags := $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -lzmq -lcryptopp -lboost_system -lboost_chrono
ifneq ($(SGX_MODE), HW)
    Trts_Library_Name := sgx_trts_sim
    Service_Library_Name := sgx_tservice_sim
    App_Link_Flags += -lsgx_uae_service_sim -lpthread
    Urts_Library_Name := sgx_urts_sim
else
    Trts_Library_Name := sgx_trts
    Service_Library_Name := sgx_tservice
    App_Link_Flags += -lsgx_uae_service
    Urts_Library_Name := sgx_urts
endif
App_Link_Flags += -l$(Urts_Library_Name)

Common_C_Cpp_Flags := $(SGX_COMMON_CFLAGS) -nostdinc -fvisibility=hidden \
                      -fpie -fstack-protector -fno-builtin-printf
Enclave_C_Flags   := -Wno-implicit-function-declaration -std=c11 \
                     $(Common_C_Cpp_Flags)
Enclave_Cpp_Flags := $(Common_C_Cpp_Flags) -I$(SGX_SDK)/include/stlport -std=c++11 -nostdinc++

Trusted_Link_Flags := $(SGX_COMMON_CFLAGS) -Wl,--no-undefined -nostdlib \
    -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_PATH) -L$(LUADIR)/bin \
    -Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
    -Wl,--start-group -lsgx_tstdc -lsgx_tstdcxx -lsgx_tcrypto \
    -l$(Service_Library_Name) -lsgx_tsetjmp -lluasgx -Wl,--end-group \
    -Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
    -Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
    -Wl,--defsym,__ImageBase=0 \
    -Wl,--version-script=$(SRCDIR)/$(MRENCLAVE).lds

all: $(TARGETS) $(BINDIR)/$(MRENCLAVE).signed.so

client : $(addprefix $(OBJDIR)/, client_protocol.o crypto.o utils.o)
worker : $(SGXCOMOBJS) $(OBJDIR)/$(MRENCLAVE)_u.o $(OBJDIR)/ocalls.o
$(TARGETS): $(SCBROBJS) 
$(TARGETS): % : $(OBJDIR)/%.o | $(BINDIR)
	@$(CXX) $^ -o $(BINDIR)/$@ $(App_Link_Flags)
	@echo -e "LINK\t\t=>\t$@"

$(OBJDIR)/worker.o : $(SRCDIR)/$(MRENCLAVE)_u.c $(SGXCOMOBJS)
$(SCBROBJS) : $(OBJDIR)/%.o : $(SCBRSRCDIR)/%.cpp | $(OBJDIR)
	@$(CXX) -c $< -o $@ $(App_Include) $(App_Cpp_Flags)
	@echo -e "CXX\t\t<=\t$<"

$(SGXCOMOBJS) : $(OBJDIR)/%.o : $(SGXCOMMONDIR)/%.cpp
	@$(CXX) -c $< -o $@ $(App_Include) $(App_Cpp_Flags)
	@echo -e "CXX\t\t<=\t$<"

$(OBJDIR)/$(MRENCLAVE)_u.o : $(SRCDIR)/$(MRENCLAVE)_u.c
$(SRCDIR)/$(MRENCLAVE)_u.c : %_u.c : %.edl $(SGX_EDGER8R)
	@cd $(dir $@) && $(SGX_EDGER8R) --untrusted ../$< --search-path ../$(dir $<) --search-path $(SGX_SDK)/include
	@echo -e "EDGER (App)\t=>\t$@"

$(OBJDIR)/$(MRENCLAVE)_t.o : $(SRCDIR)/$(MRENCLAVE)_t.c
$(SRCDIR)/$(MRENCLAVE)_t.c : $(SRCDIR)/$(MRENCLAVE).edl $(SGX_EDGER8R)
	@cd $(dir $@) && $(SGX_EDGER8R) --trusted ../$< --search-path ../$(dir $<) --search-path $(SGX_SDK)/include
	@echo -e "EDGER (Enclave)\t=>\t$@" 

$(OBJDIR)/%.o : $(SRCDIR)/%.cpp | $(OBJDIR)
	@$(CXX) -c $< -o $@ $(App_Include) $(App_Cpp_Flags)
	@echo -e "CXX\t\t<=\t$<"

$(OBJDIR)/%.o : $(SRCDIR)/%.c | $(OBJDIR)
	@$(CC) -c $< -o $@ $(App_Include) $(App_C_Flags)
	@echo -e "CC\t\t<=\t$<"

############## TRUSTED #########################################################
$(filter-out %_t.o, $(TRUSTEDOBJS)) : $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@$(CXX) -c $< -o $@ $(Enclave_Include) $(Enclave_Cpp_Flags)
	@echo -e "CXX (Enclave)\t<=\t" $<

$(OBJDIR)/$(MRENCLAVE)_t.o : $(SRCDIR)/$(MRENCLAVE)_t.c
	@$(CC) -c $< -o $@ $(Enclave_Include) $(Enclave_C_Flags)
	@echo -e "CC (Enclave)\t<=\t$<"

$(BINDIR)/$(MRENCLAVE).signed.so : %.signed.so : %.so
	@$(SGX_ENCLAVE_SIGNER) sign -enclave $< -config $(SRCDIR)/$(MRENCLAVE).config.xml -out $@ -key $(SRCDIR)/private_key.pem
	@echo -e "SIGN (Enclave)\t=>\t$@"

$(BINDIR)/$(MRENCLAVE).so : $(TRUSTEDOBJS) $(LUADIR)/bin/libluasgx.a
	@$(CXX) $(filter-out %.a,$^) -o $@ $(Trusted_Link_Flags)
	@echo -e "LINK (Enclave)\t=>\t$(filter-out %.a,$^)"

$(LUADIR)/bin/libluasgx.a :
	@$(MAKE) -C $(LUADIR)
################################################################################

$(BINDIR):
	@mkdir $(BINDIR)

$(OBJDIR):
	@mkdir $(OBJDIR)

.PHONY: all clean

clean:
	@rm -rf $(BINDIR) $(OBJDIR) $(SRCDIR)/$(MRENCLAVE)_{t,u}.{c,h}
	@echo -e "Cleaned $(BINDIR) $(OBJDIR)"
