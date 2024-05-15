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
	-lboost_thread \
	-lboost_filesystem \
	-lboost_iostreams \
	-lboost_system \
	-lpthread -ldl

all: $(PROG)

clean:
	rm -f $(PROG) *~

TEST_DB_DIR := $(shell pwd)/tmp-geonames-db

TEST_PREPARE_TARGETS := templates cnf/geonames.conf wgs84mode
TEST_FINISH_TARGETS := .dummy wgs84mode_end
ifdef CI
GEONAMES_HOST_EDIT := sed -e 's|"smartmet-test"|"$(TEST_DB_DIR)"|g'
TEST_PREPARE_TARGETS += start-geonames-db
TEST_FINISH_TARGETS += stop-geonames-db
else
GEONAMES_HOST_EDIT := cat
endif

TESTER_PARAM := --handler=/csection --reactor-config=cnf/reactor.conf -e tmp/

test: $(TEST_PREPARE_TARGETS)
	@rm -f failures/*
	@echo Running tests:
	ok=true; $(TEST_RUNNER) smartmet-plugin-test $(TESTER_PARAM) || ok=false; $(MAKE) $(TEST_FINISH_TARGETS); $$ok

cnf/geonames.conf: cnf/geonames.conf.in .dummy
	$(GEONAMES_HOST_EDIT) $< >$@

geonames-database:
	@-$(MAKE) stop-geonames-db
	rm -rf tmp-geonames-db
	if ! /usr/share/smartmet/test/db/create-local-db.sh $(TEST_DB_DIR) >tmp-geonames-db.log 2>&1 ; then \
	    cat tmp-geonames-db.log; \
	    false; \
	fi

wgs84mode:
	rm -rf tmp && mkdir tmp && cp output/* tmp/
	if grep --quiet "#define WGS84 1" /usr/include/smartmet/newbase/NFmiGlobals.h; then for f in tmp/*.wgs84; do mv $$f tmp/$$(basename $$f .wgs84); done; fi

start-geonames-db: geonames-database
	/usr/share/smartmet/test/db/test-db-ctl.sh $(TEST_DB_DIR) start -w

stop-geonames-db:
	-/usr/share/smartmet/test/db/test-db-ctl.sh $(TEST_DB_DIR) stop -w

templates:
	$(MAKE) -C .. all-templates

.dummy:
	true

.PHONY: meta.conf geonames.conf
