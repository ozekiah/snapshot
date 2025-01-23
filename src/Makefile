CC = gcc
CFLAGS = -Wall -g

TARGET = svd 
LIBS = -larchive -lyaml -lcrypto
SRCS = main.c snapshot.c config.c fs.c utils.c revision.c delta.c tree.c
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $(TARGET) -g

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)
