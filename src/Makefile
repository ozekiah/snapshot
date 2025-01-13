CC = gcc
CFLAGS = -Wall -g

TARGET = snapshot
LIBS = -larchive -lyaml
SRCS = main.c snapshot.c config.c fs.c
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $(TARGET) -g

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)
