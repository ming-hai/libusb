/* -*- Mode: C; indent-tabs-mode:t ; c-basic-offset:8 -*- */
/*
 * libusb example program for hotplug API
 * Copyright © 2012-2013 Nathan Hjelm <hjelmn@mac.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdlib.h>
#include <stdio.h>

#include "libusb.h"

int done_attach = 0;
int done_detach = 0;
libusb_device_handle *handle = NULL;

static int LIBUSB_CALL hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data)
{
	struct libusb_device_descriptor desc;
	libusb_device_handle *new_handle;
	int rc;

	(void)ctx;
	(void)dev;
	(void)event;
	(void)user_data;

	rc = libusb_get_device_descriptor(dev, &desc);
	if (LIBUSB_SUCCESS == rc) {
		printf ("Device attached: %04x:%04x\n", desc.idVendor, desc.idProduct);
	} else {
		printf ("Device attached\n");
		fprintf (stderr, "Error getting device descriptor: %s\n",
			 libusb_strerror((enum libusb_error)rc));
	}

	rc = libusb_open (dev, &new_handle);
	if (LIBUSB_SUCCESS == rc) {
		if (handle) {
			libusb_close (handle);
		}
		handle = new_handle;
	} else {
		fprintf (stderr, "No access to device: %s\n",
			 libusb_strerror((enum libusb_error)rc));
	}

	done_attach++;

	return 0;
}

static int LIBUSB_CALL hotplug_callback_detach(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data)
{
	struct libusb_device_descriptor desc;
	int rc;

	(void)ctx;
	(void)dev;
	(void)event;
	(void)user_data;

	rc = libusb_get_device_descriptor(dev, &desc);
	if (LIBUSB_SUCCESS == rc) {
		printf ("Device detached: %04x:%04x\n", desc.idVendor, desc.idProduct);
	} else {
		printf ("Device detached\n");
		fprintf (stderr, "Error getting device descriptor: %s\n",
			 libusb_strerror((enum libusb_error)rc));
	}

	if (handle) {
		libusb_close (handle);
		handle = NULL;
	}

	done_detach++;

	return 0;
}

int main(int argc, char *argv[])
{
	libusb_hotplug_callback_handle hp[2];
	int product_id, vendor_id, class_id;
	int rc;

	vendor_id  = (argc > 1) ? (int)strtol (argv[1], NULL, 0) : LIBUSB_HOTPLUG_MATCH_ANY;
	product_id = (argc > 2) ? (int)strtol (argv[2], NULL, 0) : LIBUSB_HOTPLUG_MATCH_ANY;
	class_id   = (argc > 3) ? (int)strtol (argv[3], NULL, 0) : LIBUSB_HOTPLUG_MATCH_ANY;

	rc = libusb_init_context(/*ctx=*/NULL, /*options=*/NULL, /*num_options=*/0);
	if (LIBUSB_SUCCESS != rc)
	{
		printf ("failed to initialise libusb: %s\n",
			libusb_strerror((enum libusb_error)rc));
		return EXIT_FAILURE;
	}

	if (!libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG)) {
		printf ("Hotplug capabilities are not supported on this platform\n");
		libusb_exit (NULL);
		return EXIT_FAILURE;
	}

	rc = libusb_hotplug_register_callback (NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0, vendor_id,
		product_id, class_id, hotplug_callback, NULL, &hp[0]);
	if (LIBUSB_SUCCESS != rc) {
		fprintf (stderr, "Error registering callback 0\n");
		libusb_exit (NULL);
		return EXIT_FAILURE;
	}

	rc = libusb_hotplug_register_callback (NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0, vendor_id,
		product_id,class_id, hotplug_callback_detach, NULL, &hp[1]);
	if (LIBUSB_SUCCESS != rc) {
		fprintf (stderr, "Error registering callback 1\n");
		libusb_exit (NULL);
		return EXIT_FAILURE;
	}

	while (done_detach < done_attach || done_attach == 0) {
		rc = libusb_handle_events (NULL);
		if (LIBUSB_SUCCESS != rc)
			printf ("libusb_handle_events() failed: %s\n",
				libusb_strerror((enum libusb_error)rc));
	}

	if (handle) {
		printf ("Warning: Closing left-over open handle\n");
		libusb_close (handle);
	}

	libusb_exit (NULL);

	return EXIT_SUCCESS;
}
