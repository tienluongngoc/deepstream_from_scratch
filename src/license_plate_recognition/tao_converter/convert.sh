/deepstream_from_scratch/src/license_plate_recognition/tao_converter/tao-converter \
    -k nvidia_tlt -p image_input,1x3x48x96,4x3x48x96,16x3x48x96 \
    /deepstream_from_scratch/deepstream_lpr_app/models/LP/LPR/us_lprnet_baseline18_deployable.etlt -t fp16 -e \
    /deepstream_from_scratch/deepstream_lpr_app/models/LP/LPR/lpr_us_onnx_b16.engine