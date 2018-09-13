#
#
#

export LC_ALL=C
export BEEF_ROOT=$(CURDIR)
export BEEF_BUILD=build
export BEEF_OUTPUT=$(BEEF_ROOT)/$(BEEF_BUILD)

export RTE_SDK=$(BEEF_ROOT)/.dpdk
export RTE_TARGET=build
export RTE_OUTPUT=$(BEEF_OUTPUT)

include $(RTE_SDK)/mk/rte.vars.mk

BEEF_JX ?= -j3
export BEEF_JX

DIRS-y += core
include $(RTE_SDK)/mk/rte.extsubdir.mk
