#include <experimental/filesystem>
#include <gflags/gflags.h>
#include <regex>
#include <swiftpose/swiftpose.hpp>

// Model flags
DEFINE_string(model_file, "../data/models/hao28-600000-256x384.uff",
              "Path to uff model.");
DEFINE_int32(input_height, 256, "Height of input image.");
DEFINE_int32(input_width, 384, "Width of input image.");
DEFINE_string(input_folder, "../data/media", "Folder of images to inference.");
DEFINE_string(output_foler, ".", "Folder to save outputs.");

int main()
{
    namespace fs = std::experimental::filesystem;
    constexpr auto log = []() -> std::ostream & {
        std::cout << "[SwiftPose::EXAMPLE] ";
        return std::cout;
    };

    // * Collect data into batch.
    log() << "Your current path: " << fs::current_path() << '\n';
    std::regex image_regex{R"((.*)\.(jpeg|jpg|png))"};
    std::vector<cv::Mat> batch;
    for (auto &&file : fs::directory_iterator(FLAGS_input_folder)) {
        auto file_name = file.path().string();
        if (std::regex_match(file_name, image_regex)) {
            log() << "Add file: " << file_name << " into batch.\n";
            batch.push_back(cv::imread(file_name));
        }
    }

    if (batch.empty()) {
        log() << "No input images got. Exiting.\n";
        exit(-1);
    }

    log() << "Batch shape: [" << batch.size() << ", 3, " << FLAGS_input_height
          << ", " << FLAGS_input_width << "]\n";

    // * Create TensorRT engine.
    namespace sp = swiftpose;
    sp::dnn::tensorrt engine(FLAGS_model_file,
                             {FLAGS_input_width, FLAGS_input_height}, "image",
                             {"outputs/conf", "outputs/paf"}, batch.size(),
                             nvinfer1::DataType::kHALF);
    sp::parser::paf parser({FLAGS_input_width / 8, FLAGS_input_height / 8},
                           {FLAGS_input_width, FLAGS_input_height});

    auto feature_map_packets = engine.inference(batch);
    for (const auto &packet : feature_map_packets)
        for (const auto &feature_map : packet)
            log() << feature_map << std::endl;

    // * Paf.
    for (auto &&packet : feature_map_packets) {
        auto human_vec = parser.process(packet[0], packet[1]);
        for (auto &&h : human_vec) h.print();
    }
}