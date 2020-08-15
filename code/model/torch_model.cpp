#include "torch_model.h"

const int img_width = 448;
const int img_height = 448;

torch_model* torch_model::instance() {
    static torch_model model;
    return &model;
}

void get_idx_to_label_(const char* idx_to_label_path) {
    std::vector<std::string> word;
    std::fstream fin(idx_to_label_path);
    std::string json = "";
    std::vector<int> pos;
    fin >> json;
    for(int i = 0; i < json.length(); i++) {
        if(json[i] == '"') {
            pos.push_back(i);
        }
    }
    assert(pos.size() % 4 == 0);
    idx_to_label_ = std::vector<std::string>(pos.size() / 4);
    for(int i = 0; i < pos.size(); i += 4) {
        int idx = atoi(json.c_str() + pos[i]);
        idx_to_label_[idx] = json.substr(pos[i + 2] + 1, pos[i + 3] - pos[i + 2] - 1);
    }
}

void torch_model::init(const char* model_path, const char* idx_to_label_path) {
    model_ = torch::jit::load(model_path);
    get_idx_to_label_(idx_to_label_path);
}

std::vector<std::pair<std::string, float> classify(const std::string& img_str) {
    std::vector<std::pair<std::string, float> res;
    cv::Mat img = cv::imdecode(data, IMREAD_UNCHANGED);
    auto img_tensor = torch::from_blob(img.data, {1, img_height, img_width, 3});
    img_tensor = img_tensor.permute({0, 3, 1, 2});
    img_tensor[0][0] = img_tensor[0][0].sub_(0.485).div_(0.229);
    img_tensor[0][1] = img_tensor[0][1].sub_(0.456).div_(0.224);
    img_tensor[0][2] = img_tensor[0][2].sub_(0.406).div_(0.225);
    torch::Tensor out_tensor = model_.forward({img_tensor}).toTensor();

    auto results = out_tensor.sort(-1, true);
    auto softmaxs = std::get<0>(results)[0].softmax(0);
    auto indexs = std::get<1>(results)[0];

    for (int i = 0; i < 3; i++) {
        auto idx = indexs[i].item<int>();
        res.emplace_back(idx_to_label_[idx], softmaxs[i].item<float>() * 100.0f);
    }
    return res;
}