#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

const std::string SOURCE_URL = "https://cs.stanford.edu/group/mlgroup/MURA-v1.1.zip";
const std::string SOURCE_PAGE = "https://stanfordmlgroup.github.io/competitions/mura/";
const unsigned int SEED = 20260909u;

struct Config {
    std::string archive = "data/cache/MURA-v1.1.zip";
    std::string output = "data/mura";
    std::string model = "models/mura_cnn.weights";
    std::string trainer = "./build/spine_cli";
    int image_count = 600;
    int epochs = 3;
    int batch_size = 16;
    bool full = false;
    bool force_download = false;
    bool verbose = true;
    bool prepare_only = false;
};

struct Sample {
    std::string archive_path;
    int label;
};

std::string shell_quote(const std::string& value) {
    std::string quoted = "'";
    for (char character : value) {
        if (character == '\'') quoted += "'\\''";
        else quoted += character;
    }
    quoted += "'";
    return quoted;
}

bool file_exists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode);
}

bool directory_exists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

void run(const std::string& command, const std::string& description) {
    std::cout << "[MURA] " << description << "\n";
    int result = std::system(command.c_str());
    if (result != 0) {
        throw std::runtime_error(description + " failed with status " + std::to_string(result));
    }
}

std::vector<std::string> command_lines(const std::string& command) {
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) throw std::runtime_error("Cannot execute command: " + command);
    std::vector<std::string> lines;
    char buffer[8192];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string line(buffer);
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
        if (!line.empty()) lines.push_back(line);
    }
    int result = pclose(pipe);
    if (result != 0) throw std::runtime_error("Command failed: " + command);
    return lines;
}

std::string option_value(int argc, char** argv, const std::string& option,
                         const std::string& fallback) {
    for (int index = 1; index + 1 < argc; ++index) {
        if (argv[index] == option) return argv[index + 1];
    }
    return fallback;
}

bool has_option(int argc, char** argv, const std::string& option) {
    return std::find(argv + 1, argv + argc, option) != argv + argc;
}

int option_int(int argc, char** argv, const std::string& option, int fallback) {
    return std::max(1, std::atoi(option_value(argc, argv, option,
                                              std::to_string(fallback)).c_str()));
}

void print_help() {
    std::cout
        << "Karnix C++ MURA pipeline\n\n"
        << "Usage: ./build/mura_pipeline [options]\n\n"
        << "  --archive PATH       Official MURA ZIP cache path\n"
        << "  --output PATH        Prepared dataset directory\n"
        << "  --model PATH         Output custom SpineCNN model\n"
        << "  --trainer PATH       Built spine_cli executable\n"
        << "  --count N            Prepared image count, default 600\n"
        << "  --epochs N           Training epochs, default 3\n"
        << "  --batch-size N       Loader batch size, default 16\n"
        << "  --full               Use every public train/valid image\n"
        << "  --force-download     Replace cached archive\n"
        << "  --prepare-only       Stop after manifests and PGM images\n"
        << "  --quiet              Disable per-image training telemetry\n"
        << "  --help               Show this message\n\n"
        << "This trains the custom 20,034-parameter SpineCNN on MURA.\n"
        << "It does not implement or export ResNet50/ONNX.\n";
}

Config parse_config(int argc, char** argv) {
    Config config;
    config.archive = option_value(argc, argv, "--archive", config.archive);
    config.output = option_value(argc, argv, "--output", config.output);
    config.model = option_value(argc, argv, "--model", config.model);
    config.trainer = option_value(argc, argv, "--trainer", config.trainer);
    config.image_count = option_int(argc, argv, "--count", config.image_count);
    config.epochs = option_int(argc, argv, "--epochs", config.epochs);
    config.batch_size = option_int(argc, argv, "--batch-size", config.batch_size);
    config.full = has_option(argc, argv, "--full");
    config.force_download = has_option(argc, argv, "--force-download");
    config.prepare_only = has_option(argc, argv, "--prepare-only");
    config.verbose = !has_option(argc, argv, "--quiet");
    if (!config.full && (config.image_count < 100 || config.image_count % 10 != 0)) {
        throw std::runtime_error("--count must be at least 100 and divisible by 10");
    }
    return config;
}

void ensure_parent_directory(const std::string& path) {
    std::size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) return;
    run("/bin/mkdir -p " + shell_quote(path.substr(0, slash)), "Creating parent directory");
}

void download_archive(const Config& config) {
    ensure_parent_directory(config.archive);
    if (file_exists(config.archive) && !config.force_download) {
        std::cout << "[MURA] Using cached archive: " << config.archive << "\n";
        return;
    }
    std::string temporary = config.archive + ".part";
    std::remove(temporary.c_str());
    run("/usr/bin/curl --fail --location --retry 3 --progress-bar " +
        shell_quote(SOURCE_URL) + " --output " + shell_quote(temporary),
        "Downloading official Stanford MURA archive (3.38 GB)");
    if (std::rename(temporary.c_str(), config.archive.c_str()) != 0) {
        throw std::runtime_error("Cannot finalize downloaded archive");
    }
}

std::vector<std::string> read_archive_csv(const std::string& archive,
                                          const std::string& member) {
    return command_lines("/usr/bin/unzip -p " + shell_quote(archive) + " " +
                         shell_quote(member));
}

std::vector<Sample> parse_image_paths(const std::vector<std::string>& lines) {
    std::vector<Sample> samples;
    for (const std::string& line : lines) {
        if (line.find("_positive/") != std::string::npos) samples.push_back({line, 1});
        else if (line.find("_negative/") != std::string::npos) samples.push_back({line, 0});
        else throw std::runtime_error("Cannot derive MURA label from path: " + line);
    }
    return samples;
}

std::vector<Sample> balanced_subset(const std::vector<Sample>& all, int requested,
                                    unsigned int seed) {
    std::vector<Sample> negative;
    std::vector<Sample> positive;
    for (const Sample& sample : all) {
        (sample.label == 0 ? negative : positive).push_back(sample);
    }
    std::mt19937 generator(seed);
    std::shuffle(negative.begin(), negative.end(), generator);
    std::shuffle(positive.begin(), positive.end(), generator);
    int per_class = requested / 2;
    if (static_cast<int>(negative.size()) < per_class ||
        static_cast<int>(positive.size()) < per_class) {
        throw std::runtime_error("MURA split does not contain enough images per class");
    }
    std::vector<Sample> selected(negative.begin(), negative.begin() + per_class);
    selected.insert(selected.end(), positive.begin(), positive.begin() + per_class);
    std::shuffle(selected.begin(), selected.end(), generator);
    return selected;
}

std::string output_filename(const Sample& sample, int index) {
    std::ostringstream name;
    name << (sample.label ? "abnormal_" : "normal_")
            << std::setw(5) << std::setfill('0') << index << ".bmp";
    return name.str();
}

void extract_and_convert(const Config& config, const std::string& split,
                         const std::vector<Sample>& samples,
                         std::vector<std::string>& prepared_paths) {
    std::string split_directory = config.output + "/" + split;
    run("/bin/mkdir -p " + shell_quote(split_directory),
        "Creating " + split + " output directory");
    prepared_paths.clear();
    prepared_paths.reserve(samples.size());
    for (std::size_t index = 0; index < samples.size(); ++index) {
        std::string output = split_directory + "/" +
                             output_filename(samples[index], static_cast<int>(index));
        prepared_paths.push_back(output);
        if (file_exists(output)) continue;
        std::string temporary = "/tmp/karnix_mura_" + std::to_string(getpid()) +
                                "_" + std::to_string(index) + ".png";
        run("/usr/bin/unzip -p " + shell_quote(config.archive) + " " +
            shell_quote(samples[index].archive_path) + " > " + shell_quote(temporary),
            "Extracting " + split + " image " + std::to_string(index + 1) +
            "/" + std::to_string(samples.size()));
        run("/usr/bin/sips -s format bmp -z 64 64 " + shell_quote(temporary) +
            " --out " + shell_quote(output) + " >/dev/null 2>&1",
            "Preprocessing " + split + " image " + std::to_string(index + 1) +
            "/" + std::to_string(samples.size()));
        std::remove(temporary.c_str());
    }
}

void write_manifest(const std::string& path, const std::vector<Sample>& samples,
                    const std::vector<std::string>& prepared_paths) {
    std::ofstream output(path);
    if (!output.is_open()) throw std::runtime_error("Cannot write manifest: " + path);
    output << "path,label,source_member\n";
    for (std::size_t index = 0; index < samples.size(); ++index) {
        output << prepared_paths[index] << ',' << samples[index].label << ','
               << samples[index].archive_path << '\n';
    }
}

void write_metadata(const Config& config, std::size_t train_count,
                    std::size_t valid_count, double prepare_seconds) {
    std::ofstream output(config.output + "/SOURCE.txt");
    output << "MURA-v1.1 musculoskeletal radiographs\n"
           << "Official project page: " << SOURCE_PAGE << "\n"
           << "Archive URL used: " << SOURCE_URL << "\n"
           << "Archive Last-Modified: 20 Aug 2018\n"
           << "Prepared by: Karnix C++ mura_pipeline\n"
           << "Selection seed: " << SEED << "\n"
           << "Mode: " << (config.full ? "all public train/valid images" : "balanced subset") << "\n"
           << "Prepared train images: " << train_count << "\n"
           << "Prepared validation images: " << valid_count << "\n"
           << "Preprocessing: 64x64 BMP, converted to grayscale by the C++ loader\n"
           << "Prepare duration seconds: " << std::fixed << std::setprecision(2)
           << prepare_seconds << "\n"
           << "Model: custom Karnix SpineCNN, not ResNet50\n";
}

void prepare_dataset(const Config& config) {
    auto started = std::chrono::steady_clock::now();
    std::vector<Sample> train = parse_image_paths(
        read_archive_csv(config.archive, "MURA-v1.1/train_image_paths.csv"));
    std::vector<Sample> valid = parse_image_paths(
        read_archive_csv(config.archive, "MURA-v1.1/valid_image_paths.csv"));
    if (!config.full) {
        int train_count = config.image_count * 8 / 10;
        int valid_count = config.image_count - train_count;
        train = balanced_subset(train, train_count, SEED);
        valid = balanced_subset(valid, valid_count, SEED + 1);
    }
    if (directory_exists(config.output)) {
        run("/bin/rm -rf " + shell_quote(config.output), "Removing previous prepared MURA data");
    }
    run("/bin/mkdir -p " + shell_quote(config.output), "Creating MURA output directory");
    std::vector<std::string> train_paths;
    std::vector<std::string> valid_paths;
    extract_and_convert(config, "train", train, train_paths);
    extract_and_convert(config, "valid", valid, valid_paths);
    write_manifest(config.output + "/train.csv", train, train_paths);
    write_manifest(config.output + "/valid.csv", valid, valid_paths);
    double seconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - started).count();
    write_metadata(config, train.size(), valid.size(), seconds);
    std::cout << "[MURA] Prepared " << train.size() << " train and "
              << valid.size() << " validation images in " << seconds << " seconds\n";
}

void write_model_metadata(const Config& config, double pipeline_seconds) {
    std::ofstream output(config.model + ".meta.txt");
    output << "task=MURA_ABNORMALITY\n"
           << "model_type=Karnix_SpineCNN\n"
           << "architecture=1x64x64-conv4-conv8-conv16-dense32-softmax2\n"
           << "class_0=NORMAL\n"
           << "class_1=ABNORMAL\n"
           << "epochs=" << config.epochs << "\n"
           << "batch_loader_size=" << config.batch_size << "\n"
           << "optimizer=online_SGD\n"
           << "dataset_source=" << SOURCE_PAGE << "\n"
           << "pipeline_duration_seconds=" << std::fixed << std::setprecision(2)
           << pipeline_seconds << "\n"
           << "not_resnet50=true\n"
           << "not_onnx=true\n";
}

void train_and_evaluate(const Config& config) {
    if (!file_exists(config.trainer)) {
        throw std::runtime_error("Trainer not found: " + config.trainer +
                                 ". Build with cmake first.");
    }
    ensure_parent_directory(config.model);
    std::string checkpoint_directory = "models/checkpoints/mura";
    run("/bin/rm -rf " + shell_quote(checkpoint_directory) + " && /bin/mkdir -p " +
        shell_quote(checkpoint_directory), "Resetting MURA checkpoints");
    std::string verbose = config.verbose ? " --verbose" : "";
    run(shell_quote(config.trainer) + " --train " +
        shell_quote(config.output + "/train.csv") + " " + std::to_string(config.epochs) +
        " --model " + shell_quote(config.model) +
        " --task MURA_ABNORMALITY --negative NORMAL --positive ABNORMAL" +
        " --batch-size " + std::to_string(config.batch_size) +
        " --checkpoint-dir " + shell_quote(checkpoint_directory) + verbose +
        " | /usr/bin/tee models/mura_training_full.log",
        "Training custom C++ CNN on MURA");
    run(shell_quote(config.trainer) + " --evaluate " +
        shell_quote(config.output + "/valid.csv") +
        " --model " + shell_quote(config.model) +
        " --task MURA_ABNORMALITY --negative NORMAL --positive ABNORMAL" + verbose +
        " | /usr/bin/tee models/mura_evaluation_full.log",
        "Evaluating custom C++ CNN on MURA validation images");
}

} 

int main(int argc, char** argv) {
    try {
        if (has_option(argc, argv, "--help")) {
            print_help();
            return 0;
        }
        auto pipeline_started = std::chrono::steady_clock::now();
        Config config = parse_config(argc, argv);
        std::cout << "KARNIX C++ MURA PIPELINE\n"
                  << "Source: " << SOURCE_URL << "\n"
                  << "Mode: " << (config.full ? "FULL PUBLIC DATASET" : "BALANCED SUBSET") << "\n"
                  << "Prepared images: " << (config.full ? -1 : config.image_count) << "\n"
                  << "Model: custom SpineCNN (not ResNet50, not ONNX)\n\n";
        download_archive(config);
        prepare_dataset(config);
        if (!config.prepare_only) train_and_evaluate(config);
        double seconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - pipeline_started).count();
        if (!config.prepare_only) write_model_metadata(config, seconds);
        std::cout << "[MURA] PIPELINE COMPLETE in " << std::fixed << std::setprecision(2)
                  << seconds << " seconds\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[MURA ERROR] " << error.what() << "\n";
        return 1;
    }
}
