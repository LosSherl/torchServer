#ifndef TORCH_MODEL_H
#define TORCH_DODEL_H

#include <string>
#include <vector>
#include <fstream>
#include <torch/script.h>

// headers for opencv
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>

class torch_model{
public:
    static torch_model* instance();
    void init(const char* model_path, const char* idx_to_label_path);
    std::vector< std::pair<std::string, float> > classify(const std::string& img_str);
private:
    torch_model() = default;
    ~torch_model() = default;
    void get_idx_to_label_(const char* get_idx_to_label_path);

    torch::jit::script::Module model_;
    std::vector<std::string> idx_to_label_;
};

#endif