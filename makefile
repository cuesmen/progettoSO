# Compilatore
CC=gcc

# Flag del compilatore
CFLAGS=-Wall -Wvla -Wextra -Werror

# Directory degli oggetti
OBJDIR=objects

# Directory dei file sorgenti e header
SRCDIR=src/c
INCDIR=src/h
LIBDIR=libs

# File di output
TARGET=main

# Trova tutti i file sorgenti nelle directory specificate
SRCS=$(wildcard $(SRCDIR)/*.c) $(wildcard $(LIBDIR)/*.c)

# Genera la lista dei file oggetto corrispondenti
OBJS=$(patsubst %.c,$(OBJDIR)/%.o,$(notdir $(SRCS)))

# Target principale
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Regola per creare la directory degli oggetti
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Regola generale per compilare qualsiasi file .c in un file .o da SRCDIR
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Regola generale per compilare qualsiasi file .c in un file .o da LIBDIR
$(OBJDIR)/%.o: $(LIBDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Pulire i file generati
clean:
	rm -rf $(OBJDIR) $(TARGET)
