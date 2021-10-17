#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include <locale.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gmodule.h>
#include "gstnvdsmeta.h"
#include "gst-nvmessage.h"
#include "nvdsmeta.h"

#define MAX_DISPLAY_LEN 64
#define MEASURE_ENABLE 1
#define PGIE_CLASS_ID_VEHICLE 0
#define PGIE_CLASS_ID_PERSON 2
#define SGIE_CLASS_ID_LPD 0 
#define MUXER_OUTPUT_WIDTH 1280
#define MUXER_OUTPUT_HEIGHT 720
#define MUXER_BATCH_TIMEOUT_USEC 4000000
#define CONFIG_GROUP_TRACKER "tracker"
#define CONFIG_GROUP_TRACKER_WIDTH "tracker-width"
#define CONFIG_GROUP_TRACKER_HEIGHT "tracker-height"
#define CONFIG_GROUP_TRACKER_LL_CONFIG_FILE "ll-config-file"
#define CONFIG_GROUP_TRACKER_LL_LIB_FILE "ll-lib-file"
#define CONFIG_GROUP_TRACKER_ENABLE_BATCH_PROCESS "enable-batch-process"
#define CONFIG_GPU_ID "gpu-id"
#define PRIMARY_DETECTOR_UID 1
#define SECONDARY_DETECTOR_UID 2
#define SECONDARY_CLASSIFIER_UID 3


typedef struct _perf_measure{
  GstClockTime pre_time;
  GstClockTime total_time;
  guint count;
}perf_measure;


#define CHECK_ERROR(error) \
  if (error) { \
    g_printerr ("Error while parsing config file: %s\n", error->message); \
    goto done; \
  }


static gchar *get_absolute_file_path(gchar *cfg_file_path, gchar *file_path) {
  gchar abs_cfg_path[PATH_MAX + 1];
  gchar *abs_file_path;
  gchar *delim;

  if (file_path && file_path[0] == '/') {
    return file_path;
  }

  if (!realpath(cfg_file_path, abs_cfg_path)) {
    g_free(file_path);
    return NULL;
  }

  if (!file_path) {
    abs_file_path = g_strdup(abs_cfg_path);
    return abs_file_path;
  }

  delim = g_strrstr(abs_cfg_path, "/");
  *(delim + 1) = '\0';

  abs_file_path = g_strconcat(abs_cfg_path, file_path, NULL);
  g_free(file_path);

  return abs_file_path;
}


static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
  GMainLoop *loop = (GMainLoop *)data;
  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS:
      g_print("End of stream\n");
      g_main_loop_quit(loop);
      break;
    case GST_MESSAGE_ERROR: {
      gchar *debug;
      GError *error;
      gst_message_parse_error(msg, &error, &debug);
      g_printerr("ERROR from element %s: %s\n", GST_OBJECT_NAME(msg->src),
                 error->message);
      if (debug) g_printerr("Error details: %s\n", debug);
      g_free(debug);
      g_error_free(error);
      g_main_loop_quit(loop);
      break;
    }
    default:
      break;
  }
  return TRUE;
}


static void cb_new_pad(GstElement *element, GstPad *pad, GstElement *data) {
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;
  GstPadLinkReturn ret;

  GstPad *sink_pad = gst_element_get_static_pad(data, "sink");
  if (gst_pad_is_linked(sink_pad)) {
    g_print("h264parser already linked. Ignoring.\n");
    goto exit;
  }

  new_pad_caps = gst_pad_get_current_caps(pad);
  new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
  new_pad_type = gst_structure_get_name(new_pad_struct);
  g_print("qtdemux pad %s\n", new_pad_type);

  if (g_str_has_prefix(new_pad_type, "video/x-h264")) {
    ret = gst_pad_link(pad, sink_pad);
    if (GST_PAD_LINK_FAILED(ret))
      g_print("fail to link parser and mp4 demux.\n");
  } else {
    g_print("%s output, not 264 stream\n", new_pad_type);
  }

exit:
  gst_object_unref(sink_pad);
}


static gboolean set_tracker_properties(GstElement *nvtracker, char *config_file_name) {
  gboolean ret = FALSE;
  GError *error = NULL;
  gchar **keys = NULL;
  gchar **key = NULL;
  GKeyFile *key_file = g_key_file_new();

  if (!g_key_file_load_from_file(key_file, config_file_name, G_KEY_FILE_NONE,
                                 &error)) {
    g_printerr("Failed to load config file: %s\n", error->message);
    return FALSE;
  }

  keys = g_key_file_get_keys(key_file, CONFIG_GROUP_TRACKER, NULL, &error);
  CHECK_ERROR(error);

  for (key = keys; *key; key++) {
    if (!g_strcmp0(*key, CONFIG_GROUP_TRACKER_WIDTH)) {
      gint width = g_key_file_get_integer(key_file, CONFIG_GROUP_TRACKER,
                                          CONFIG_GROUP_TRACKER_WIDTH, &error);
      CHECK_ERROR(error);
      g_object_set(G_OBJECT(nvtracker), "tracker-width", width, NULL);
    } else if (!g_strcmp0(*key, CONFIG_GROUP_TRACKER_HEIGHT)) {
      gint height = g_key_file_get_integer(key_file, CONFIG_GROUP_TRACKER,
                                           CONFIG_GROUP_TRACKER_HEIGHT, &error);
      CHECK_ERROR(error);
      g_object_set(G_OBJECT(nvtracker), "tracker-height", height, NULL);
    } else if (!g_strcmp0(*key, CONFIG_GPU_ID)) {
      guint gpu_id = g_key_file_get_integer(key_file, CONFIG_GROUP_TRACKER,
                                            CONFIG_GPU_ID, &error);
      CHECK_ERROR(error);
      g_object_set(G_OBJECT(nvtracker), "gpu_id", gpu_id, NULL);
    } else if (!g_strcmp0(*key, CONFIG_GROUP_TRACKER_LL_CONFIG_FILE)) {
      char *ll_config_file = get_absolute_file_path(
          config_file_name,
          g_key_file_get_string(key_file, CONFIG_GROUP_TRACKER,
                                CONFIG_GROUP_TRACKER_LL_CONFIG_FILE, &error));
      CHECK_ERROR(error);
      g_object_set(G_OBJECT(nvtracker), "ll-config-file", ll_config_file, NULL);
    } else if (!g_strcmp0(*key, CONFIG_GROUP_TRACKER_LL_LIB_FILE)) {
      char *ll_lib_file = get_absolute_file_path(
          config_file_name,
          g_key_file_get_string(key_file, CONFIG_GROUP_TRACKER,
                                CONFIG_GROUP_TRACKER_LL_LIB_FILE, &error));
      CHECK_ERROR(error);
      g_object_set(G_OBJECT(nvtracker), "ll-lib-file", ll_lib_file, NULL);
    } else if (!g_strcmp0(*key, CONFIG_GROUP_TRACKER_ENABLE_BATCH_PROCESS)) {
      gboolean enable_batch_process = g_key_file_get_integer(
          key_file, CONFIG_GROUP_TRACKER,
          CONFIG_GROUP_TRACKER_ENABLE_BATCH_PROCESS, &error);
      CHECK_ERROR(error);
      g_object_set(G_OBJECT(nvtracker), "enable_batch_process",
                   enable_batch_process, NULL);
    } else {
      g_printerr("Unknown key '%s' for group [%s]", *key, CONFIG_GROUP_TRACKER);
    }
  }

  ret = TRUE;
done:
  if (error) {
    g_error_free(error);
  }
  if (keys) {
    g_strfreev(keys);
  }
  if (!ret) {
    g_printerr("%s failed", __func__);
  }
  return ret;
}


static GstPadProbeReturn osd_sink_pad_buffer_probe (GstPad * pad, GstPadProbeInfo * info, gpointer u_data);


int lpr (int argc, char *argv[]);
