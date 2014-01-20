
# Trabalho 3 de redes, Alunos: Giancarlo Klemm Camilo, Renan Greca.
# UFPR 2012/2

    PROG = poker

    # AQUI VAI A LISTA DOS MODULOS
    OBJS   = $(PROG).o udp_socket.o

    # Aqui vai o comando do compilador, com opcoes aplicaveis 
    # `a compilacao de qualquer modulo e `a geracao do codigo executavel
    CC     = gcc -g

%.o: %.c
	$(CC) -c $<

$(PROG):  $(OBJS)
	$(CC) -o $@ $^

clean:
	@rm -f *~ *.bak *.o

#distclean:   clean
#	@rm -f  $(PROG) *.o core a.out
