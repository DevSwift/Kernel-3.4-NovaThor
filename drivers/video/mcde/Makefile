mcde-objs		+= mcde_mod.o
mcde-objs		+= mcde_hw.o
mcde-objs		+= mcde_dss.o
mcde-objs		+= mcde_display.o
mcde-objs		+= mcde_bus.o
mcde-objs		+= mcde_fb.o
mcde-objs		+= mcde_debugfs.o
obj-$(CONFIG_FB_MCDE)	+= mcde.o

obj-$(CONFIG_MCDE_DISPLAY_DSI)			+= dsilink.o
obj-$(CONFIG_MCDE_DISPLAY_DSI)			+= dsilink_v2.o
obj-$(CONFIG_MCDE_DISPLAY_DSI)			+= dsilink_v3.o
obj-$(CONFIG_MCDE_DISPLAY_GENERIC_DSI)		+= display-generic_dsi.o
obj-$(CONFIG_MCDE_DISPLAY_SAMSUNG_S6D16D0)	+= display-samsung_s6d16d0.o
obj-$(CONFIG_MCDE_DISPLAY_SONY_ACX424AKP_DSI)   += display-sony_acx424akp_dsi.o
obj-$(CONFIG_MCDE_DISPLAY_VUIB500_DPI)		+= display-vuib500-dpi.o
obj-$(CONFIG_MCDE_DISPLAY_AB8500_DENC)		+= display-ab8500.o
obj-$(CONFIG_MCDE_DISPLAY_AV8100)		+= display-av8100.o
obj-$(CONFIG_DISPLAY_FICTIVE)			+= display-fictive.o

obj-$(CONFIG_MCDE_DISPLAY_PANEL_DSI)				+= display-panel_dsi.o
obj-$(CONFIG_MCDE_DISPLAY_LOGO)					+= logo.o
obj-$(CONFIG_MCDE_DISPLAY_R61408_DSI_SONY_L5F31178)		+= display-r61408_dsi_sony_l5f31178.o

ifeq ($(CONFIG_DEBUG_FS),y)
	mcde-objs						+= display-panel_dsi_debugfs.o
endif

ifdef CONFIG_FB_MCDE_DEBUG
EXTRA_CFLAGS += -DDEBUG
endif
ifdef CONFIG_FB_MCDE_VDEBUG
EXTRA_CFLAGS += -DVERBOSE_DEBUG
endif
