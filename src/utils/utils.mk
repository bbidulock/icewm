icesame_TOP	:= $(TOP)/utils

icesame_SRCS	:= \
	$(icesame_TOP)/icesame.cc
        
icesame_OBJS 	:= $(icesame_SRCS:.cc=.o)

icesame_DEPS	:= $(icesame_SRCS:.cc=.d)

icesame_ALIBS	:= libbase.a

icesame: $(icesame_OBJS) $(icesame_ALIBS)

DEPS	+= $(icesame_DEPS)

#######################################################################
iceview_TOP	:= $(TOP)/utils

iceview_SRCS	:= \
	$(iceview_TOP)/iceview.cc
        
iceview_OBJS	:= $(iceview_SRCS:.cc=.o)

iceview_DEPS	:= $(iceview_SRCS:.cc=.d)

iceview_ALIBS	:= libbase.a

iceview: $(iceview_OBJS) $(iceview_ALIBS)

DEPS	+= $(iceview_DEPS)

#######################################################################
icelist_TOP	:= $(TOP)/utils

icelist_SRCS	:= \
	$(icelist_TOP)/icelist.cc
        
icelist_OBJS	:= $(icelist_SRCS:.cc=.o)

icelist_DEPS	:= $(icelist_SRCS:.cc=.d)

icelist_ALIBS	:= libbase.a

icelist: $(icelist_OBJS) $(icelist_ALIBS)

DEPS	+= $(icelist_DEPS)

#######################################################################
icewm_help_TOP	:= $(TOP)/utils

icewm_help_SRCS := \
	$(icewm_help_TOP)/icehelp.cc
        
icewm_help_OBJS	:= $(icewm_help_SRCS:.cc=.o)

icewm_help_DEPS := $(icewm_help_SRCS:.cc=.d)

icewm_help_ALIBS := libbase.a

icewm_help: $(icewm_help_OBJS) $(icewm_help_ALIBS)

DEPS	+= $(icewm_help_DEPS)

#######################################################################
iceicon_TOP	:= $(TOP)/utils

iceicon_SRCS	:= \
	$(iceicon_TOP)/iceicon.cc
        
iceicon_OBJS	:= $(iceicon_SRCS:.cc=.o)

iceicon_DEPS	:= $(iceicon_SRCS:.cc=.d)

iceicon_ALIBS	:= libbase.a

iceicon: $(iceicon_OBJS) $(iceicon_ALIBS)

DEPS	+= $(iceicon_DEPS)

#######################################################################
test_dnd_TOP	:= $(TOP)/utils

test_dnd_SRCS	:= \
	$(test_dnd_TOP)/dndtest.cc
        
test_dnd_OBJS	:= $(test_dnd_SRCS:.cc=.o)

test_dnd_DEPS	:= $(test_dnd_SRCS:.cc=.d)

test_dnd_ALIBS	:= libbase.a

test_dnd: $(test_dnd_OBJS) $(test_dnd_ALIBS)

DEPS	+= $(test_dnd_DEPS)

#######################################################################
test_nop_TOP	:= $(TOP)/utils

test_nop_SRCS	:= \
	$(test_nop_TOP)/nop.cc
        
test_nop_OBJS	:= $(test_nop_SRCS:.cc=.o)

test_nop_DEPS	:= $(test_nop_SRCS:.cc=.d)

test_nop_ALIBS=libbase.a

test_nop: $(test_nop_OBJS) $(test_nop_ALIBS)

DEPS	+= $(test_nop_DEPS)

#######################################################################
test_wmmark_TOP	:= $(TOP)/utils

test_wmmark_SRCS := \
	$(test_wmmark_TOP)/wmmark.cc
        
test_wmmark_OBJS := $(test_wmmark_SRCS:.cc=.o)

test_wmmark_DEPS := $(test_wmmark_SRCS:.cc=.d)

test_wmmark_ALIBS := libbase.a

test_wmmark: $(test_wmmark_OBJS) $(test_wmmark_ALIBS)

DEPS	+= $(test_wmmark_DEPS)

#######################################################################
test_skt_TOP	:= $(TOP)/utils

test_skt_SRCS	:= \
	$(test_skt_TOP)/iceskt.cc
        
test_skt_OBJS	:= $(test_skt_SRCS:.cc=.o)

test_skt_DEPS	:= $(test_skt_SRCS:.cc=.d)

test_skt_ALIBS	:= libbase.a

test_skt: $(test_skt_OBJS) $(test_skt_ALIBS)

DEPS	+= $(test_skt_DEPS)

#######################################################################
icewm_hint_TOP := $(TOP)/utils

icewm_hint_SRCS := \
	$(icewm_hint_TOP)/icewmhint.cc
        
icewm_hint_OBJS := $(icewm_hint_SRCS:.cc=.o)

icewm_hint_DEPS := $(icewm_hint_SRCS:.cc=.d)

icewm_hint_ALIBS=libbase.a

icewm_hint: $(icewm_hint_OBJS) $(icewm_hint_ALIBS)

DEPS	+= $(icewm_hint_DEPS)

#######################################################################
icewm_bg_TOP	:= $(TOP)/utils

icewm_bg_SRCS := \
	$(icewm_bg_TOP)/icewmbg.cc
        
icewm_bg_OBJS := $(icewm_bg_SRCS:.cc=.o)

icewm_bg_DEPS := $(icewm_bg_SRCS:.cc=.d)

icewm_bg_ALIBS := libbase.a

icewm_bg: $(icewm_bg_OBJS) $(icewm_bg_ALIBS)

DEPS	+= $(icewm_bg_DEPS)

#######################################################################
icewm_sound_TOP=$(TOP)/utils

icewm_sound_SRCS= \
	$(icewm_sound_TOP)/icesound.cc
        
icewm_sound_OBJS=$(icewm_sound_SRCS:.cc=.o)

icewm_sound_DEPS=$(icewm_sound_SRCS:.cc=.d)

icewm_sound_ALIBS=libbase.a

icewm_sound: $(icewm_sound_OBJS) $(icewm_sound_ALIBS)

DEPS	+= $(icewm_sound_DEPS)

#######################################################################
test_gravity_TOP=$(TOP)/utils

test_gravity_SRCS= \
	$(test_gravity_TOP)/test_gravity.cc
        
test_gravity_OBJS=$(test_gravity_SRCS:.cc=.o)

test_gravity_DEPS=$(test_gravity_SRCS:.cc=.d)

test_gravity_ALIBS=libbase.a

test_gravity: $(test_gravity_OBJS) $(test_gravity_ALIBS)

DEPS	+= $(test_gravity_DEPS)

#######################################################################
test_mwm_hints_TOP	:= $(TOP)/utils

test_mwm_hints_SRCS	:= \
	$(test_mwm_hints_TOP)/testmwmhints.cc
        
test_mwm_hints_OBJS	:= $(test_mwm_hints_SRCS:.cc=.o)

test_mwm_hints_DEPS	:= $(test_mwm_hints_SRCS:.cc=.d)

test_mwm_hints_ALIBS	:= libbase.a

test_mwm_hints: $(test_mwm_hints_OBJS) $(test_mwm_hints_ALIBS)

DEPS	+= $(test_dnd_DEPS)
