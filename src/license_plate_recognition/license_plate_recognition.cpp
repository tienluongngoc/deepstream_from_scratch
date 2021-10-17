#include "license_plate_recognition/license_plate_recognition.hpp"

#include <iostream>

gint frame_number = 0;
gint total_plate_number = 0;
gchar pgie_classes_str[4][32] = {"Vehicle", "TwoWheeler", "Person", "Roadsign"};

static GstPadProbeReturn osd_sink_pad_buffer_probe(GstPad *pad,
                                                   GstPadProbeInfo *info,
                                                   gpointer u_data) {
  GstBuffer *buf = (GstBuffer *)info->data;
  NvDsObjectMeta *obj_meta = NULL;
  guint vehicle_count = 0;
  guint person_count = 0;
  guint lp_count = 0;
  guint label_i = 0;
  NvDsMetaList *l_frame = NULL;
  NvDsMetaList *l_obj = NULL;
  NvDsMetaList *l_class = NULL;
  NvDsMetaList *l_label = NULL;
  NvDsDisplayMeta *display_meta = NULL;
  NvDsClassifierMeta *class_meta = NULL;
  NvDsLabelInfo *label_info = NULL;

  NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);

  for (l_frame = batch_meta->frame_meta_list; l_frame != NULL;
       l_frame = l_frame->next) {
    NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(l_frame->data);
    if (!frame_meta) continue;
    for (l_obj = frame_meta->obj_meta_list; l_obj != NULL;
         l_obj = l_obj->next) {
      obj_meta = (NvDsObjectMeta *)(l_obj->data);
      if (!obj_meta) continue;

      if (obj_meta->unique_component_id == PRIMARY_DETECTOR_UID) {
        if (obj_meta->class_id == PGIE_CLASS_ID_VEHICLE) vehicle_count++;
        if (obj_meta->class_id == PGIE_CLASS_ID_PERSON) person_count++;
      }

      if (obj_meta->unique_component_id == SECONDARY_DETECTOR_UID) {
        if (obj_meta->class_id == SGIE_CLASS_ID_LPD) lp_count++;
      }

      for (l_class = obj_meta->classifier_meta_list; l_class != NULL;
           l_class = l_class->next) {
        class_meta = (NvDsClassifierMeta *)(l_class->data);
        if (!class_meta) continue;
        if (class_meta->unique_component_id == SECONDARY_CLASSIFIER_UID) {
          for (label_i = 0, l_label = class_meta->label_info_list;
               label_i < class_meta->num_labels && l_label;
               label_i++, l_label = l_label->next) {
            label_info = (NvDsLabelInfo *)(l_label->data);
            if (label_info) {
              if (label_info->label_id == 0 &&
                  label_info->result_class_id == 1) {
                std::cout << "Plate License: " << label_info->result_label
                          << std::endl;
              }
            }
          }
        }
      }
    }
  }
  std::cout << "Frame Number: " << frame_number
            << " Vehicle Count: " << vehicle_count
            << " Person Count: " << person_count
            << " License Plate Count: " << lp_count << std::endl;
  frame_number++;
  total_plate_number += lp_count;
  return GST_PAD_PROBE_OK;
}

int lpr(int argc, char *argv[]) {
  GstElement *h264parser[128];
  GstElement *source[128];
  GstElement *decoder[128];
  GstElement *mp4demux[128];
  GstElement *parsequeue[128];

  GstBus *bus = NULL;
  guint bus_watch_id;
  GstPad *osd_sink_pad = NULL;
  GstCaps *caps = NULL;
  GstCapsFeatures *feature = NULL;
  static guint src_cnt = 0;
  perf_measure perf_measure;

  gchar ele_name[64];
  GstPad *sinkpad, *srcpad;
  gchar pad_name_sink[16] = "sink_0";
  gchar pad_name_src[16] = "src";

  gst_init(&argc, &argv);
  auto loop = g_main_loop_new(NULL, FALSE);

  perf_measure.pre_time = GST_CLOCK_TIME_NONE;
  perf_measure.total_time = GST_CLOCK_TIME_NONE;
  perf_measure.count = 0;

  auto pipeline = gst_pipeline_new("pipeline");
  auto streammux = gst_element_factory_make("nvstreammux", "stream-muxer");

  gst_bin_add(GST_BIN(pipeline), streammux);

  for (src_cnt = 0; src_cnt < (int)argc - 1; src_cnt++) {
    g_snprintf(ele_name, 64, "file_src_%d", src_cnt);
    source[src_cnt] = gst_element_factory_make("filesrc", ele_name);

    g_snprintf(ele_name, 64, "mp4demux_%d", src_cnt);
    mp4demux[src_cnt] = gst_element_factory_make("qtdemux", ele_name);

    g_snprintf(ele_name, 64, "h264parser_%d", src_cnt);
    h264parser[src_cnt] = gst_element_factory_make("h264parse", ele_name);

    g_snprintf(ele_name, 64, "parsequeue_%d", src_cnt);
    parsequeue[src_cnt] = gst_element_factory_make("queue", ele_name);

    g_snprintf(ele_name, 64, "decoder_%d", src_cnt);
    decoder[src_cnt] = gst_element_factory_make("nvv4l2decoder", ele_name);

    gst_bin_add_many(GST_BIN(pipeline), source[src_cnt], mp4demux[src_cnt],
                     h264parser[src_cnt], parsequeue[src_cnt], decoder[src_cnt], NULL);

    g_snprintf(pad_name_sink, 64, "sink_%d", src_cnt);
    sinkpad = gst_element_get_request_pad(streammux, pad_name_sink);
    srcpad = gst_element_get_static_pad(decoder[src_cnt], pad_name_src);

    gst_pad_link(srcpad, sinkpad);

    gst_element_link_pads(source[src_cnt], "src", mp4demux[src_cnt], "sink");
    g_signal_connect(mp4demux[src_cnt], "pad-added", G_CALLBACK(cb_new_pad),
                     h264parser[src_cnt]);
    gst_element_link_many(h264parser[src_cnt], parsequeue[src_cnt],
                          decoder[src_cnt], NULL);

    g_object_set(G_OBJECT(source[src_cnt]), "location", argv[1 + src_cnt],
                 NULL);

    gst_object_unref(sinkpad);
    gst_object_unref(srcpad);
  }

  auto vehicle_detection =
      gst_element_factory_make("nvinfer", "vehicle_detection");
  auto vehicle_tracking =
      gst_element_factory_make("nvtracker", "vehicle_tracking");
  auto plate_detection = gst_element_factory_make("nvinfer", "plate_detection");
  auto text_recognition =
      gst_element_factory_make("nvinfer", "text_recognition");

  auto queue1 = gst_element_factory_make("queue", "queue1");
  auto queue2 = gst_element_factory_make("queue", "queue2");
  auto queue3 = gst_element_factory_make("queue", "queue3");
  auto queue4 = gst_element_factory_make("queue", "queue4");

  auto sink = gst_element_factory_make("fakesink", "fake-renderer");

  g_object_set(G_OBJECT(streammux), "width", MUXER_OUTPUT_WIDTH, NULL);
  g_object_set(G_OBJECT(streammux), "height", MUXER_OUTPUT_HEIGHT, NULL);
  g_object_set(G_OBJECT(streammux), "batch-size", src_cnt, NULL);
  g_object_set(G_OBJECT(streammux), "batched-push-timeout",
               MUXER_BATCH_TIMEOUT_USEC, NULL);

  g_object_set(G_OBJECT(vehicle_detection), "config-file-path",
               "apps/license_plate_recognition/configs/vehicle_detection.txt",
               NULL);
  g_object_set(G_OBJECT(vehicle_detection), "unique-id", PRIMARY_DETECTOR_UID,
               NULL);

  g_object_set(G_OBJECT(plate_detection), "config-file-path",
               "apps/license_plate_recognition/configs/plate_detection.txt",
               NULL);
  g_object_set(G_OBJECT(plate_detection), "unique-id", SECONDARY_DETECTOR_UID,
               NULL);
  g_object_set(G_OBJECT(plate_detection), "process-mode", 2, NULL);

  g_object_set(G_OBJECT(text_recognition), "config-file-path",
               "apps/license_plate_recognition/configs/text_recognition.txt",
               NULL);
  g_object_set(G_OBJECT(text_recognition), "unique-id",
               SECONDARY_CLASSIFIER_UID, NULL);
  g_object_set(G_OBJECT(text_recognition), "process-mode", 2, NULL);

  char name[300];
  snprintf(name, 300,
           "apps/license_plate_recognition/configs/vehicle_tracking.txt");
  set_tracker_properties(vehicle_tracking, name);

  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
  gst_object_unref(bus);

  gst_bin_add_many(GST_BIN(pipeline), vehicle_detection, plate_detection,
                   vehicle_tracking, queue1, queue2, queue3, queue4,
                   text_recognition, sink, NULL);

  gst_element_link_many(streammux, queue1, vehicle_detection, queue2,
                        vehicle_tracking, queue3, plate_detection, queue4,
                        text_recognition, sink, NULL);

  osd_sink_pad = gst_element_get_static_pad(sink, "sink");
  gst_pad_add_probe(osd_sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
                    osd_sink_pad_buffer_probe, NULL, NULL);
  gst_object_unref(osd_sink_pad);

  gst_element_set_state(pipeline, GST_STATE_PLAYING);
  g_print("Running...\n");
  g_main_loop_run(loop);
  gst_element_set_state(pipeline, GST_STATE_NULL);

  gst_object_unref(GST_OBJECT(pipeline));
  g_source_remove(bus_watch_id);
  g_main_loop_unref(loop);
  return 0;
}
