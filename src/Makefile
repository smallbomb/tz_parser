PROGS = tz_parser
OBJS = main.o tzdump.o

$(PROGS): $(OBJS)
	$(CC) -Wall $^ -o $@

%.o: %.c
	$(CC) -c $<

clean:
	rm -f $(OBJS)
