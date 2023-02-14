PROJECT = knnimdenoiser
CC = gcc
CFLAGS = -Wall
LDFLAGS = -lpng -lm
OMPFLAGS = -fopenmp -DOMP_USE
RM = rm -f

all: $(PROJECT) $(PROJECT)_omp

$(PROJECT): $(PROJECT).c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(PROJECT)_omp: $(PROJECT).c
	$(CC) $(CFLAGS) $(OMPFLAGS) $^ $(LDFLAGS) -o $@

clean:
	$(RM) $(PROJECT) $(PROJECT)_omp
