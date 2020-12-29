SUBNAME = cross_section
SPEC = smartmet-plugin-$(SUBNAME)
INCDIR = smartmet/plugins/$(SUBNAME)

REQUIRES = gdal jsoncpp ctpp2

include $(shell echo $${PREFIX-/usr})/share/smartmet/devel/makefile.inc

sysconfdir ?= /etc
tmpldir = $(sysconfdir)/smartmet/plugins/$(SUBNAME)

DEFINES = -DUNIX -D_REENTRANT

LIBS += -L$(libdir) \
	$(REQUIRED_LIBS) \
	-lsmartmet-spine \
	-lsmartmet-gis \
	-lboost_date_time \
	-lboost_thread \
	-lboost_iostreams \
	-lboost_system \
	-lbz2 -lz

# Templates

TEMPLATES = $(wildcard tmpl/*.tmpl)
BYTECODES = $(TEMPLATES:%.tmpl=%.c2t)

# What to install

LIBFILE = $(SUBNAME).so

# Compilation directories

vpath %.cpp $(SUBNAME)
vpath %.h $(SUBNAME)

# The files to be compiled

SRCS = $(wildcard $(SUBNAME)/*.cpp)
HDRS = $(wildcard $(SUBNAME)/*.h)
OBJS = $(patsubst %.cpp, obj/%.o, $(notdir $(SRCS)))

INCLUDES := -I$(SUBNAME) $(INCLUDES)

.PHONY: test rpm

# The rules

all: objdir $(LIBFILE) $(BYTECODES)
debug: all
release: all
profile: all

configtest:
	@if [ -x "$$(command -v cfgvalidate)" ]; then cfgvalidate -v test/cnf/cross_section.conf; fi

$(LIBFILE): $(OBJS)
	$(CC) $(LDFLAGS) -shared -rdynamic -o $(LIBFILE) $(OBJS) $(LIBS)

clean:
	rm -f $(LIBFILE) *~ $(SUBNAME)/*~
	rm -rf obj
	rm tmpl/*.c2t
	$(MAKE) -C test $@

format:
	clang-format -i -style=file $(SUBNAME)/*.h $(SUBNAME)/*.cpp test/*.cpp

install:
	@mkdir -p $(plugindir)
	$(INSTALL_PROG) $(LIBFILE) $(plugindir)/$(LIBFILE)
	@mkdir -p $(tmpldir)
	@list=`ls -1 tmpl/*.c2t`; \
	echo $(INSTALL_DATA) $$list $(tmpldir)/; \
	$(INSTALL_DATA) $$list $(tmpldir)/

test:
	cd test && make test

objdir:
	@mkdir -p $(objdir)

rpm: clean $(SPEC).spec
	rm -f $(SPEC).tar.gz # Clean a possible leftover from previous attempt
	tar -czvf $(SPEC).tar.gz --exclude test --exclude-vcs --transform "s,^,$(SPEC)/," *
	rpmbuild -tb $(SPEC).tar.gz
	rm -f $(SPEC).tar.gz

.SUFFIXES: $(SUFFIXES) .cpp

obj/%.o: %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.c2t:  %.tmpl
	ctpp2c $< $@

-include $(wildcard obj/*.d)
