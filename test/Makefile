PROG = PluginTest

REQUIRES = gdal configpp jsoncpp

include $(shell echo $${PREFIX-/usr})/share/smartmet/devel/makefile.inc

CFLAGS = -DUNIX -O0 -g $(FLAGS) -Wno-unknown-pragmas

# Unfortunately otlv4.h forces us to define the full oracle include path
INCLUDES += \
	-I ../cross_section

LIBS += -L$(libdir) \
	$(REQUIRED_LIBS) \
	-lsmartmet-spine  \
	-lsmartmet-macgyver \
	-lsmartmet-newbase \
	-lboost_date_time \
	-lboost_thread \
	-lboost_filesystem \
	-lboost_iostreams \
	-lboost_system \
	-lpthread -ldl

all: $(PROG)
clean:
	rm -f $(PROG) *~

ifdef CI
test: fminames $(PROG)
	@sed -i -e 's|"../tmpl"|"/etc/smartmet/plugins/cross_section"|g' cnf/cross_section.conf
else
test: $(PROG)
endif
	@./PluginTest -c cnf/reactor.conf
	@test -f PluginTest.err && mv PluginTest.err sqlite.err || echo All tests passed
	@printf "\n"
	@test `find . -name \*.err | wc -l` = "0" || ( echo ; echo "The following tests have errors:" ; \
	   for i in *.err ; do echo `basename $$i .err`; done ; rm -f *.err ; false )

fminames:
	sed -i -e 's/"smartmet-test"/"localhost"/g' cnf/*.conf
	@/usr/share/smartmet/test/db/init-and-start.sh && /usr/share/smartmet/test/db/install-test-db.sh > /dev/null

$(PROG) : % : %.cpp Makefile
	$(CXX) $(CFLAGS) -o $@ $@.cpp $(INCLUDES) $(LIBS)

clean:
	rm -f $(PROG) *~

	@mkdir -p failures
	@echo Running tests:
	@for prog in $(PROG); do ./$$prog; done
	@rm -f cross-section-access-log
