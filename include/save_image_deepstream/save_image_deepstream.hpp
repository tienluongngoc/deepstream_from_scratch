#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include <cuda_runtime_api.h>
#include "gstnvdsmeta.h"

static GstPadProbeReturn osd_sink_pad_buffer_probe (GstPad * pad, GstPadProbeInfo * info, gpointer u_data);
static gboolean bus_call (GstBus * bus, GstMessage * msg, gpointer data);
int save_image_deepstream (int argc, char *argv[]);