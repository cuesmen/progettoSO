roadMap:

- Utilizzato un file .ini "conf.ini" come storage per le variabili.
- Inserito la libreria inih per facilitarmi l'accesso al file.
- Implementato config.c per la lettura del file e la gestione delle variabili.
- Scelto i file ini essendo dei file di testo strutturati in sezioni e facilmente modificabili e accessibili.

- Creato la struttura per leggere il config.ini dentro config.h
- Creato la memoria condivisa dentro master da poter utilizzare fra i vari processi
- Creato l'infrastruttura di atomo

daFare:
- Rivedere commenti
- Eliminare include a caso
- MIN_N_ATOMICO = 1 LOOP
- Master loop uscita

cd /mnt/c/users/cosmi/onedrive/desktop/progetto_so
make clean
make
rm atomo
./master
