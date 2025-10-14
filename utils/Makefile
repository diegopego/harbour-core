ROOT := ../

ifneq ($(HB_BUILD_PARTS),lib)

DIRS := \
   hbmk2 \
   hbast \

ifneq ($(HB_BUILD_PARTS),compiler)

   DIRS := \
      hbtest \
      hbast \
      hbmk2 \
      hbi18n \
      $(__HB_UTIL_ADDONS)

endif

endif

ifneq ($(DIRS),)
   include $(ROOT)config/dir.mk
else
   include $(ROOT)config/none.mk
endif
