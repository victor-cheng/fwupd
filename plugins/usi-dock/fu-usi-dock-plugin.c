/*
 * Copyright (C) 2021 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2021 Victor Cheng <victor_cheng@usiglobal.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"

#include <libusb.h>

#include "fu-usi-dock-dmc-device.h"
#include "fu-usi-dock-mcu-device.h"
#include "fu-usi-dock-plugin.h"

#define USI_DOCK_TBT_INSTANCE_ID "THUNDERBOLT\\VEN_0108&DEV_2031"

struct _FuUsiDockPlugin {
	FuPlugin parent_instance;
};

G_DEFINE_TYPE(FuUsiDockPlugin, fu_usi_dock_plugin, FU_TYPE_PLUGIN)

static void
fu_usi_dock_plugin_dmc_registered(FuPlugin *plugin, FuDevice *device)
{
	/* usb device from thunderbolt plugin */
	if (g_strcmp0(fu_device_get_plugin(device), "thunderbolt") == 0 &&
	    fu_device_has_guid(device, USI_DOCK_TBT_INSTANCE_ID)) {
		g_autofree gchar *msg = NULL;
		msg = g_strdup_printf("firmware update inhibited by [%s] plugin",
				      fu_plugin_get_name(plugin));
		fu_device_inhibit(device, "usb-blocked", msg);
	}
}

static void
fu_usi_dock_plugin_init(FuUsiDockPlugin *self)
{
}

static void
fu_usi_dock_plugin_constructed(GObject *obj)
{
	FuPlugin *plugin = FU_PLUGIN(obj);
	fu_plugin_add_device_gtype(plugin, FU_TYPE_USI_DOCK_MCU_DEVICE);
	fu_plugin_add_device_gtype(plugin, FU_TYPE_USI_DOCK_DMC_DEVICE);
}

static void
fu_usi_dock_plugin_class_init(FuUsiDockPluginClass *klass)
{
	FuPluginClass *plugin_class = FU_PLUGIN_CLASS(klass);
	plugin_class->constructed = fu_usi_dock_plugin_constructed;
	plugin_class->device_registered = fu_usi_dock_plugin_dmc_registered;
}

gboolean
fu_usi_dock_plugin_reset_usb(guint16 vid, guint16 pid)
{
	libusb_device_handle *handle = NULL;
	int ret;

	if (libusb_init(NULL) < 0) {
		g_info("libusb init failed");
		return FALSE;
	}

	handle = libusb_open_device_with_vid_pid(NULL, vid, pid);
	if (handle == NULL) {
		g_info("Device %04x:%04x not found", vid, pid);
		libusb_exit(NULL);
		return FALSE;
	}

	ret = libusb_reset_device(handle);
	if (ret != 0) {
		g_info("Failed to reset %04x:%04x — %s", vid, pid, libusb_error_name(ret));
		libusb_close(handle);
		libusb_exit(NULL);
		return FALSE;
	}

	g_info("Successfully reset device %04x:%04x", vid, pid);
	libusb_close(handle);
	libusb_exit(NULL);
	return TRUE;
}
