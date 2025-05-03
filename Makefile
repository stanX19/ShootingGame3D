SRCDIR		= srcs
SRCS		:= $(shell find $(SRCDIR) -name '*.cpp')

OBJDIR		= objs
OBJDIRS		= $(sort $(dir $(OBJS)))
OBJS		= $(subst $(SRCDIR)/,$(OBJDIR)/,$(subst .cpp,.o,$(SRCS)))

CWD			:= $(shell pwd)
INCLUDE_DIR	= includes/raylib includes/entt
HEADER_DIR	= headers
HEADERS		:= $(shell find $(HEADER_DIR) -name '*.hpp')
HEADERS_INC	= $(addprefix -I,$(sort $(dir $(HEADERS))) $(INCLUDE_DIR))

IFLAGS		:= -I. $(HEADERS_INC)
LFLAGS		= -Lincludes/raylib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

CC			= g++
CFLAGS		= -std=c++20 -Wall -Wextra -Werror -g3 # -fsanitize=address
AR			= ar -rcs
RM			= rm -rf
UP			= \033[1A
FLUSH		= \033[2K

NAME		= shooting_game_3d
ARGV		= 

run: all
	./$(NAME) $(ARGV)

$(NAME): $(OBJDIRS) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(IFLAGS) $(LFLAGS) -o $(NAME)

all: $(NAME)

$(OBJDIRS):
	mkdir -p $@
	@echo "$(UP)$(FLUSH)$(UP)$(FLUSH)$(UP)"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(HEADERS)
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@
	@echo "$(UP)$(FLUSH)$(UP)$(FLUSH)$(UP)$(FLUSH)$(UP)"

clean:
	@$(RM) $(OBJS)

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

.PHONY: all clean fclean re bonus push