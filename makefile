# Compilatore
CC=gcc
CFLAGS=-Wall -Wvla -Wextra -Werror

# Directory degli oggetti
OBJDIR=objects

# Directory dei file sorgenti e header
MODULEDIR=src/modules
INCDIR=$(MODULEDIR)/include
LIBDIR=libs

# File di output
TARGET_MASTER=master
TARGET_CONFIG=config
TARGET_ATOMO=atomo
TARGET_ATTIVATORE=attivatore

# File sorgenti
SRCS_MASTER=$(MODULEDIR)/master/master.c $(LIBDIR)/ini.c $(MODULEDIR)/semaphore_utils.c
SRCS_CONFIG=$(MODULEDIR)/config/config.c $(LIBDIR)/ini.c
SRCS_ATOMO=$(MODULEDIR)/atomo/atomo.c $(MODULEDIR)/semaphore_utils.c
SRCS_ATTIVATORE=$(MODULEDIR)/attivatore/attivatore.c $(MODULEDIR)/semaphore_utils.c

# File oggetto
OBJS_MASTER=$(patsubst %.c,$(OBJDIR)/%.o,$(notdir $(SRCS_MASTER)))
OBJS_CONFIG=$(patsubst %.c,$(OBJDIR)/%.o,$(notdir $(SRCS_CONFIG)))
OBJS_ATOMO=$(patsubst %.c,$(OBJDIR)/%.o,$(notdir $(SRCS_ATOMO)))
OBJS_ATTIVATORE=$(patsubst %.c,$(OBJDIR)/%.o,$(notdir $(SRCS_ATTIVATORE)))

# Target principale
all: $(TARGET_MASTER) $(TARGET_CONFIG) $(TARGET_ATOMO) $(TARGET_ATTIVATORE)

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

# Regola generale per compilare qualsiasi file .c in un file .o
$(OBJDIR)/%.o: $(MODULEDIR)/master/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -I$(MODULEDIR)/atomo -c $< -o $@

$(OBJDIR)/%.o: $(MODULEDIR)/atomo/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -I$(MODULEDIR)/atomo -c $< -o $@

$(OBJDIR)/%.o: $(MODULEDIR)/config/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -I$(MODULEDIR)/atomo -c $< -o $@

$(OBJDIR)/%.o: $(MODULEDIR)/attivatore/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -I$(MODULEDIR)/atomo -c $< -o $@

$(OBJDIR)/%.o: $(LIBDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -I$(MODULEDIR)/atomo -c $< -o $@

# Aggiungi una regola per compilare semaphore_utils.c
$(OBJDIR)/%.o: $(MODULEDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -I$(MODULEDIR)/atomo -c $< -o $@

# Pulizia dei file generati
clean:
	rm -rf $(OBJDIR) $(TARGET_MASTER) $(TARGET_CONFIG) $(TARGET_ATOMO) $(TARGET_ATTIVATORE)

run: all
	./$(TARGET_MASTER)
