NPROC		:= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
MAKEFLAGS	+= -j$(NPROC)

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
INCLUDE_DIR	= includes/raylib includes/entt includes
HEADER_DIR	= headers
HEADERS		:= $(shell find $(HEADER_DIR) -name '*.hpp')
HEADERS_INC	= $(addprefix -I,$(sort $(dir $(HEADERS))) $(INCLUDE_DIR))

IFLAGS		:= -I. $(HEADERS_INC)

ifneq ($(shell command -v mold 2>/dev/null),)
    LINKER := -fuse-ld=mold
else ifneq ($(shell command -v lld 2>/dev/null),)
    LINKER := -fuse-ld=lld
else
    LINKER := 
endif

CC_BASE		= g++
ifneq ($(shell command -v ccache 2>/dev/null),)
CC			= ccache $(CC_BASE)
else
CC			= $(CC_BASE)
endif

LFLAGS		= $(LINKER) -Lincludes/raylib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
CFLAGS		= -std=c++20 -Wall -Wextra -Werror -MMD -MP -fmax-errors=3# -g3 -fsanitize=address
AR			= ar -rcs
RM			= rm -rf
UP			= \033[1A
FLUSH		= \033[2K

PCH_HEADER	= headers/includes.hpp
PCH			= headers/includes.hpp.gch
PCH_FLAG	= -include $(PCH_HEADER)
PCH_DEPS	= $(PCH:.gch=.d)

NAME		= shooting_game_3d
ARGV		= 

run: all
	./$(NAME) $(ARGV)

$(NAME): $(OBJDIRS) $(PCH) $(OBJS) $(MAINCPP)
	$(CC) $(CFLAGS) $(PCH_FLAG) $(OBJS) $(MAINCPP) $(IFLAGS) $(LFLAGS) -o $(NAME)

all: $(NAME)

LATEST_TEST_EXEC := $(TESTBINDIR)/$(basename $(notdir $(shell ls -t $(TESTDIR)/*.cpp | head -1)))

test: $(LATEST_TEST_EXEC)
	@echo "Running $(LATEST_TEST_EXEC)..."
	@./$(LATEST_TEST_EXEC) || exit 1

all_test: $(TESTBINDIR) $(OBJS) $(TESTOBJS)
	@for test_exec in $(TESTOBJS); do \
		echo "Running $$test_exec..."; \
		./$$test_exec || exit 1; \
	done

testbin: $(TESTBINDIR) $(OBJS) $(TESTOBJS)
	
$(OBJDIRS) $(TESTBINDIR):
	mkdir -p $@
	@echo "$(UP)$(FLUSH)$(UP)$(FLUSH)$(UP)"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(PCH) | $(OBJDIRS)
	$(CC) $(CFLAGS) $(IFLAGS) $(PCH_FLAG) -c $< -o $@

$(TESTBINDIR)/%: $(TESTDIR)/%.cpp $(OBJS) $(PCH) | $(TESTBINDIR)
	$(CC) $(CFLAGS) $(IFLAGS) $(PCH_FLAG) $< $(OBJS) $(LFLAGS) -o $@

$(PCH): $(PCH_HEADER) | $(OBJDIRS)
	$(CC) $(CFLAGS) $(IFLAGS) -x c++-header $< -o $@

clean:
	@$(RM) $(OBJS) $(OBJS:.o=.d) $(PCH) $(PCH_DEPS)

fclean:	clean
	@$(RM) $(NAME)
	@$(RM) $(OBJDIRS)
	@$(RM) ./a.out

re: fclean $(NAME)

push:
	make -C $(LIBFT_DIR) push
	@echo -n "Commit name: "; read name; make fclean;\
	git add .; git commit -m "$$name"; git push;

code:
	find $(SRCDIR) $(HEADER_DIR) -type f \( -name "*.hpp" -o -name "*.cpp" \) -exec cat {} + > ../code.txt

.PHONY: all clean fclean re bonus push test run_test test all_test
-include $(OBJS:.o=.d) $(PCH_DEPS)