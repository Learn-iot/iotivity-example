#! /usr/bin/make -f
# //******************************************************************
# //
# // Copyright 2014 Intel Corporation.
# // Copyright 2015 Eurogiciel <philippe.coval@eurogiciel.fr>
# // Copyright 2016 Samsung <philippe.coval@osg.samsung.com>
# //
# //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
# //
# // Licensed under the Apache License, Version 2.0 (the "License");
# // you may not use this file except in compliance with the License.
# // You may obtain a copy of the License at
# //
# //      http://www.apache.org/licenses/LICENSE-2.0
# //
# // Unless required by applicable law or agreed to in writing, software
# // distributed under the License is distributed on an "AS IS" BASIS,
# // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# // See the License for the specific language governing permissions and
# // limitations under the License.
# //
# //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

default: all

package?=iotivity-example


local_bindir?=bin
local_bindir?=opt
DEST_LIB_DIR?=${DESTDIR}${local_optdir}/${package}/
IOTIVITY_DIR?=$(PKG_CONFIG_SYSROOT_DIR)/usr/include/iotivity

CPPFLAGS+=$(shell pkg-config iotivity --cflags)
LIBS+=-loc -loc_logger -loctbstack

ifeq (${PLATFORM}, TIZEN)
CPPFLAGS+=-D__TIZEN__=1 
CPPFLAGS+=$(shell pkg-config dlog --cflags)
LIBS+=$(shell pkg-config dlog --libs)
else
CPPFLAGS+=-I${IOTIVITY_DIR}/resource/stack
CPPFLAGS+=-I${IOTIVITY_DIR}/resource/c_common
endif

client?=${local_bindir}/client
server_objs?=
server?=${local_bindir}/server
client_objs?=

all+=${server}
all+=${client}
all+=${observer}

${local_bindir}/server: src/server.o ${server_objs} ${objs}
	@-mkdir -p ${@D}
	${CC} -o ${@} $^ ${LDFLAGS} ${LIBS}

${local_bindir}/client: src/client.o ${client_objs} ${objs}
	@-mkdir -p ${@D}
	${CC} -o ${@} $^ ${LDFLAGS} ${LIBS}

all: ${all}

${local_bindir}/%: %.o ${objs}
	@-mkdir -p ${@D}
	${CC} -o ${@} $^ ${LDFLAGS} ${LIBS}

run/%: bin/%
	${<D}/${<F}

xterm/% : bin/%
	xterm -e ${MAKE} run/${@F} &
	sleep 1

run: xterm/server xterm/client

clean:
	-rm -f *.o *~ ${objs} */*.o
	find . -iname "*~" -exec rm '{}' \;
	find . -iname "*.o" -exec rm -v '{}' \;

distclean: clean
	rm -rf bin
	rm -f ${client} ${server}

install: ${all}
	install -d ${DEST_LIB_DIR}
	install $^ ${DEST_LIB_DIR}
