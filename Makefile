SRCDIR		= srcs
SRCS		:= $(shell find $(SRCDIR) -name '*.cpp')

OBJDIR		= objs
OBJS		= $(subst $(SRCDIR)/,$(OBJDIR)/,$(subst .cpp,.o,$(SRCS)))
OBJDIRS		= $(sort $(dir $(OBJS)))

MAINCPP		= main/main.cpp

TESTDIR		= tests
TESTS		:= $(shell find $(TESTDIR) -name '*.cpp')
TESTBINDIR	= objs/test_bin
TESTOBJS    = $(patsubst $(TESTDIR)/%.cpp,$(TESTBINDIR)/%,$(TESTS))

CWD			:= $(shell pwd)
INCLUDE_DIR	= includes/raylib includes/entt
HEADER_DIR	= headers
HEADERS		:= $(shell find $(HEADER_DIR) -name '*.hpp')
HEADERS_INC	= $(addprefix -I,$(sort $(dir $(HEADERS))) $(INCLUDE_DIR))

IFLAGS		:= -I. $(HEADERS_INC)
LFLAGS		= -Lincludes/raylib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

CC			= g++
CFLAGS		= -std=c++20 -Wall -Wextra -Werror -MMD -MP #-g3 -fsanitize=address
AR			= ar -rcs
RM			= rm -rf
UP			= \033[1A
FLUSH		= \033[2K

NAME		= shooting_game_3d
ARGV		= 

run: all
	./$(NAME) $(ARGV)

$(NAME): $(OBJDIRS) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(MAINCPP) $(IFLAGS) $(LFLAGS) -o $(NAME)

all: $(NAME)

test: $(TESTBINDIR) $(OBJS) $(TESTOBJS)
	@for test_exec in $(TESTOBJS); do \
		echo "Running $$test_exec..."; \
		./$$test_exec || exit 1; \
	done
	

$(OBJDIRS):
	mkdir -p $@
	@echo "$(UP)$(FLUSH)$(UP)$(FLUSH)$(UP)"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIRS)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(TESTBINDIR)/%: $(TESTDIR)/%.cpp $(OBJS) $(TESTBINDIR)
	$(CC) $(CFLAGS) $(IFLAGS) $< $(OBJS) $(LFLAGS) -o $@

$(TESTBINDIR):
	mkdir -p $(TESTBINDIR)

clean:
	@$(RM) $(OBJS) $(OBJS:.o=.d)

fclean:	clean
	@$(RM) $(NAME)
	@$(RM) $(TESTDIR)
	@$(RM) $(OBJDIRS)
	@$(RM) ./a.out

re: fclean $(NAME)

push:
	make -C $(LIBFT_DIR) push
	@echo -n "Commit name: "; read name; make fclean;\
	git add .; git commit -m "$$name"; git push;

code:
	find $(SRCDIR) $(HEADER_DIR) -type f \( -name "*.hpp" -o -name "*.cpp" \) -exec cat {} + > ../code.txt

.PHONY: all clean fclean re bonus push
-include $(OBJS:.o=.d)