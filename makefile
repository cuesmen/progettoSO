# Compilatore
CC=gcc

# Flag del compilatore
CFLAGS=-Wall -Wvla -Wextra -Werror

# Directory degli oggetti
OBJDIR=objects

# Directory dei file sorgenti e header
SRCDIR=src/c
INCDIR=src/h

# File di output
TARGET=main

# File sorgenti (aggiungi qui i nuovi file sorgenti)
SRCS=$(SRCDIR)/master.c $(SRCDIR)/config.c $(SRCDIR)/ini.c

# File oggetto (nella directory degli oggetti)
OBJS=$(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Target principale
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Regola per creare la directory degli oggetti
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Compilare i file .c in file .o nella cartella objects
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Pulire i file generati
clean:
	rm -rf $(OBJS) $(TARGET)
