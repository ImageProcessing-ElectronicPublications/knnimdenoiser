PROJECT = knnimdenoiser
CC = gcc
CFLAGS = -Wall -Isrc
LDFLAGS = -lpng -lm
OMPFLAGS = -fopenmp -DOMP_USE
RM = rm -f

all: $(PROJECT) $(PROJECT)_omp

$(PROJECT): src/$(PROJECT).c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(PROJECT)_omp: src/$(PROJECT).c
	$(CC) $(CFLAGS) $(OMPFLAGS) $^ $(LDFLAGS) -o $@

clean:
	$(RM) $(PROJECT) $(PROJECT)_omp
