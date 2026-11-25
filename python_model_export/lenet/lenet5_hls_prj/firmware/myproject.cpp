#include <iostream>

#include "myproject.h"
#include "parameters.h"


void myproject(
    hls::stream<input_t> &input_layer,
    hls::stream<result_t> &layer13_out
) {

    // hls-fpga-machine-learning insert IO
    #pragma HLS INTERFACE axis port=input_layer,layer13_out 
    #pragma HLS DATAFLOW

    // hls-fpga-machine-learning insert load weights
#ifndef __SYNTHESIS__
    static bool loaded_weights = false;
    if (!loaded_weights) {
        nnet::load_weights_from_txt<conv1_weight_t, 150>(w2, "w2.txt");
        nnet::load_weights_from_txt<conv1_bias_t, 6>(b2, "b2.txt");
        nnet::load_weights_from_txt<conv2_weight_t, 2400>(w5, "w5.txt");
        nnet::load_weights_from_txt<conv2_bias_t, 16>(b5, "b5.txt");
        nnet::load_weights_from_txt<fc1_weight_t, 30720>(w9, "w9.txt");
        nnet::load_weights_from_txt<fc1_bias_t, 120>(b9, "b9.txt");
        nnet::load_weights_from_txt<fc2_weight_t, 10080>(w11, "w11.txt");
        nnet::load_weights_from_txt<fc2_bias_t, 84>(b11, "b11.txt");
        nnet::load_weights_from_txt<output_weight_t, 840>(w13, "w13.txt");
        nnet::load_weights_from_txt<output_bias_t, 10>(b13, "b13.txt");
        loaded_weights = true;    }
#endif
    // ****************************************
    // NETWORK INSTANTIATION
    // ****************************************

    // hls-fpga-machine-learning insert layers

    hls::stream<layer2_t> layer2_out("layer2_out");
    #pragma HLS STREAM variable=layer2_out depth=576

    hls::stream<layer3_t> layer3_out("layer3_out");
    #pragma HLS STREAM variable=layer3_out depth=576

    hls::stream<layer4_t> layer4_out("layer4_out");
    #pragma HLS STREAM variable=layer4_out depth=144

    hls::stream<layer5_t> layer5_out("layer5_out");
    #pragma HLS STREAM variable=layer5_out depth=64

    hls::stream<layer6_t> layer6_out("layer6_out");
    #pragma HLS STREAM variable=layer6_out depth=64

    hls::stream<layer7_t> layer7_out("layer7_out");
    #pragma HLS STREAM variable=layer7_out depth=16

    auto& layer8_out = layer7_out;
    hls::stream<layer9_t> layer9_out("layer9_out");
    #pragma HLS STREAM variable=layer9_out depth=1

    hls::stream<layer10_t> layer10_out("layer10_out");
    #pragma HLS STREAM variable=layer10_out depth=1

    hls::stream<layer11_t> layer11_out("layer11_out");
    #pragma HLS STREAM variable=layer11_out depth=1

    hls::stream<layer12_t> layer12_out("layer12_out");
    #pragma HLS STREAM variable=layer12_out depth=1

    nnet::conv_2d_cl<input_t, layer2_t, config2>(input_layer, layer2_out, w2, b2); // conv1

    nnet::relu<layer2_t, layer3_t, relu_config3>(layer2_out, layer3_out); // conv1_relu

    nnet::pooling2d_cl<layer3_t, layer4_t, config4>(layer3_out, layer4_out); // pool1

    nnet::conv_2d_cl<layer4_t, layer5_t, config5>(layer4_out, layer5_out, w5, b5); // conv2

    nnet::relu<layer5_t, layer6_t, relu_config6>(layer5_out, layer6_out); // conv2_relu

    nnet::pooling2d_cl<layer6_t, layer7_t, config7>(layer6_out, layer7_out); // pool2

    nnet::dense<layer7_t, layer9_t, config9>(layer8_out, layer9_out, w9, b9); // fc1

    nnet::relu<layer9_t, layer10_t, relu_config10>(layer9_out, layer10_out); // fc1_relu

    nnet::dense<layer10_t, layer11_t, config11>(layer10_out, layer11_out, w11, b11); // fc2

    nnet::relu<layer11_t, layer12_t, relu_config12>(layer11_out, layer12_out); // fc2_relu

    nnet::dense<layer12_t, result_t, config13>(layer12_out, layer13_out, w13, b13); // output

}

