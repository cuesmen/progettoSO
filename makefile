# Compilatore
CC=gcc
CFLAGS=-Wall -Wvla -Wextra -Werror -D_GNU_SOURCE

# Directory degli oggetti e degli eseguibili
OBJDIR=objects
BINDIR=bin

# Directory dei file sorgenti e header
SRCDIR=src
MODULEDIR=$(SRCDIR)/modules
INCDIR=$(MODULEDIR)/include
LIBDIR=libs
GUIDIR=gui

# File di output
TARGET_MASTER=$(BINDIR)/master
TARGET_CONFIG=$(BINDIR)/config
TARGET_ATOMO=$(BINDIR)/atomo
TARGET_ATTIVATORE=$(BINDIR)/attivatore
TARGET_ALIMENTAZIONE=$(BINDIR)/alimentazione
TARGET_GUI=$(BINDIR)/gui_program
TARGET_INIBITORE=$(BINDIR)/inibitore

# File sorgenti
SRCS_MASTER=$(MODULEDIR)/master/master.c $(LIBDIR)/ini.c $(SRCDIR)/semaphore_utils.c $(SRCDIR)/log_utils.c
SRCS_CONFIG=$(MODULEDIR)/config/config.c $(LIBDIR)/ini.c
SRCS_ATOMO=$(MODULEDIR)/atomo/atomo.c $(SRCDIR)/semaphore_utils.c $(SRCDIR)/log_utils.c
SRCS_ATTIVATORE=$(MODULEDIR)/attivatore/attivatore.c $(SRCDIR)/semaphore_utils.c
SRCS_ALIMENTAZIONE=$(MODULEDIR)/alimentazione/alimentazione.c
SRCS_GUI=$(GUIDIR)/gui.c $(SRCDIR)/semaphore_utils.c
SRCS_INIBITORE=$(MODULEDIR)/inibitore/inibitore.c $(SRCDIR)/log_utils.c $(SRCDIR)/semaphore_utils.c

# File oggetto
OBJS_MASTER=$(OBJDIR)/master.o $(OBJDIR)/ini.o $(OBJDIR)/semaphore_utils.o $(OBJDIR)/log_utils.o
OBJS_CONFIG=$(OBJDIR)/config.o $(OBJDIR)/ini.o
OBJS_ATOMO=$(OBJDIR)/atomo.o $(OBJDIR)/semaphore_utils.o $(OBJDIR)/log_utils.o
OBJS_ATTIVATORE=$(OBJDIR)/attivatore.o $(OBJDIR)/semaphore_utils.o
OBJS_ALIMENTAZIONE=$(OBJDIR)/alimentazione.o
OBJS_INIBITORE=$(OBJDIR)/inibitore.o $(OBJDIR)/log_utils.o $(OBJDIR)/semaphore_utils.o

# Target principale
all: $(BINDIR) $(TARGET_MASTER) $(TARGET_CONFIG) $(TARGET_ATOMO) $(TARGET_ATTIVATORE) $(TARGET_ALIMENTAZIONE) $(TARGET_GUI) $(TARGET_INIBITORE)

# Crea la directory bin se non esiste
$(BINDIR):
	mkdir -p $(BINDIR)

# Compilazione del master
$(TARGET_MASTER): $(OBJS_MASTER)
	$(CC) $(CFLAGS) -o $(TARGET_MASTER) $(OBJS_MASTER)

# Compilazione del config
$(TARGET_CONFIG): $(OBJS_CONFIG)
	$(CC) $(CFLAGS) -o $(TARGET_CONFIG) $(OBJS_CONFIG)

# Compilazione di atomo
$(TARGET_ATOMO): $(OBJS_ATOMO)
	$(CC) $(CFLAGS) -o $(TARGET_ATOMO) $(OBJS_ATOMO)

# Compilazione di attivatore
$(TARGET_ATTIVATORE): $(OBJS_ATTIVATORE)
	$(CC) $(CFLAGS) -o $(TARGET_ATTIVATORE) $(OBJS_ATTIVATORE)

# Compilazione di alimentazione
$(TARGET_ALIMENTAZIONE): $(OBJS_ALIMENTAZIONE)
	$(CC) $(CFLAGS) -o $(TARGET_ALIMENTAZIONE) $(OBJS_ALIMENTAZIONE)

# Compilazione della GUI
$(TARGET_GUI): $(SRCS_GUI) | $(BINDIR)
	$(CC) $(SRCS_GUI) -o $(TARGET_GUI) `pkg-config --cflags --libs gtk+-3.0`

# Compilazione di inibitore
$(TARGET_INIBITORE): $(OBJS_INIBITORE)
	$(CC) $(CFLAGS) -o $(TARGET_INIBITORE) $(OBJS_INIBITORE)

# Regola generale per compilare i file .c in un file .o per i moduli
$(OBJDIR)/%.o: $(MODULEDIR)/*/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Regola per compilare i file direttamente nella directory src
$(OBJDIR)/semaphore_utils.o: $(SRCDIR)/semaphore_utils.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

$(OBJDIR)/log_utils.o: $(SRCDIR)/log_utils.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Regola per compilare i file nella directory libs
$(OBJDIR)/%.o: $(LIBDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Pulizia dei file generati
clean:
	rm -rf $(OBJDIR)
	find $(BINDIR) -maxdepth 1 -type f ! -name 'config.ini' -exec rm -f {} +

# Esegui il master
run: all
	cd $(BINDIR) && ./master

# Compila ed esegui la GUI e il master
runGui: all
	cd $(BINDIR) && ./$(notdir $(TARGET_GUI)) & \
	cd $(BINDIR) && ./$(notdir $(TARGET_MASTER))
